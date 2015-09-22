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
	uint indexLocal = GTid.x + GTid.y * 32;
	uint indexTotal = indexLocal + Gid.x * 1024;
	uint lane = GTid.x;
	uint warpid = Gid.x * 32 + GTid.y;
	uint addr = (uint)((1 << 5 * depth) - 1) / 31 + indexTotal;

	if (warpid >= 1 << 5 * (depth - 1)) return;

	temp[indexLocal] = treeNodes[addr];

	if (lane >= 1) temp[indexLocal] = AABBMinMax(temp[indexLocal - 1], temp[indexLocal]);
	if (lane >= 2) temp[indexLocal] = AABBMinMax(temp[indexLocal - 2], temp[indexLocal]);
	if (lane >= 4) temp[indexLocal] = AABBMinMax(temp[indexLocal - 4], temp[indexLocal]);
	if (lane >= 8) temp[indexLocal] = AABBMinMax(temp[indexLocal - 8], temp[indexLocal]);
	if (lane >= 16) temp[indexLocal] = AABBMinMax(temp[indexLocal - 16], temp[indexLocal]);

	if (lane == 0)
		treeNodes[addr >> 5] = temp[indexLocal + 31];
}