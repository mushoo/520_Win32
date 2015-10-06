#define N 1024

struct AABBNode
{
	float3 center;
	float3 radius;
	uint index;
	bool active;
};

RWStructuredBuffer<AABBNode> treeNodes : register(u0);

cbuffer ConstantBuffer : register(b0)
{
	uint depth;
	uint blocksize;
	bool first;
};

AABBNode AABBMinMax(AABBNode a, AABBNode b)
{
	AABBNode result;
	if (!a.active) a = b;
	if (!b.active) b = a;
	result.center = (min(a.center - a.radius, b.center - b.radius) + max(a.center + a.radius, b.center + b.radius)) / 2.0f;
	result.radius = max(a.center + a.radius, b.center + b.radius) - result.center;
	result.index = 0xFFFFFFFF;
	result.active = a.active || b.active;
	return result;
}

groupshared AABBNode temp[N];

[numthreads(32, 32, 1)]
void main(
	uint3 Gid : SV_GroupID,
	uint3 GTid : SV_GroupThreadID
	)
{
	uint lane = GTid.x;
	uint warpid = Gid.x * 32 + GTid.y;
	warpid *= blocksize;

	for (uint i = 0; i < blocksize; i++)
	{
		if (warpid >= 1 << 5 * depth) return;

		uint rootAddr = ((1 << 5 * depth) - 1) / 31 + warpid;
		uint childAddr = (rootAddr << 5) + 1 + GTid.x;
		uint sharedIndex = GTid.x + GTid.y * 32;

		temp[sharedIndex] = treeNodes[childAddr];

		if (first)
		{
			temp[sharedIndex].active = true;
			treeNodes[childAddr].active = true;
		}

		if (lane >= 1) temp[sharedIndex] = AABBMinMax(temp[sharedIndex - 1], temp[sharedIndex]);
		if (lane >= 2) temp[sharedIndex] = AABBMinMax(temp[sharedIndex - 2], temp[sharedIndex]);
		if (lane >= 4) temp[sharedIndex] = AABBMinMax(temp[sharedIndex - 4], temp[sharedIndex]);
		if (lane >= 8) temp[sharedIndex] = AABBMinMax(temp[sharedIndex - 8], temp[sharedIndex]);
		if (lane >= 16) temp[sharedIndex] = AABBMinMax(temp[sharedIndex - 16], temp[sharedIndex]);

		if (lane == 31)
			treeNodes[rootAddr] = temp[sharedIndex];

		warpid++;
	}
}