struct AABBNode
{
	float3 center;
	float3 radius;
	uint index;
	bool active;
};

struct Cluster
{
	uint clusterNum;
	uint lightCount;
	uint lightOffset;
	bool active;
};

struct Pair
{
	uint clusterIndex;
	uint lightIndex;
	uint lightOffset;
	bool valid;
};

cbuffer ConstantBuffer : register(b0)
{
	uint maxDepth;
	uint width;
	uint height;
	float denominator;
	float invFocalLenX;
	float invFocalLenY;
};

StructuredBuffer<AABBNode> treeNodes : register(t0);
RWStructuredBuffer<Cluster> clusters : register(u0);
AppendStructuredBuffer<Pair> pairAppendBuffer : register(u1);

groupshared bool branch[32][5][32]; // [warpid][depth][lane]
groupshared uint lightCounts[32][32]; // [warpid][lane] (accumulative)
groupshared uint totalCounts[32];

bool checkAABBintersection(AABBNode a, float3 center, float3 radius)
{
	bool x = abs(a.center.x - center.x) <= (a.radius.x + radius.x);
	bool y = abs(a.center.y - center.y) <= (a.radius.y + radius.y);
	bool z = abs(a.center.z - center.z) <= (a.radius.z + radius.z);

	return x && y && z;
}

float squared(float x){ return x * x; }

bool checkAABBsphere(float3 center, float3 radius, float3 S, float R)
{
	float3 C1 = center - radius;
	float3 C2 = center + radius;

	float dist_squared = R * R;
	/*
	dist_squared -= (S.x < C1.x) * squared(S.x - C1.x);
	dist_squared -= (S.x >= C1.x) * (S.x < C2.x) * squared(S.x - C2.x);

	dist_squared -= (S.y < C1.y) * squared(S.y - C1.y);
	dist_squared -= (S.y >= C1.y) * (S.y < C2.y) * squared(S.y - C2.y);

	dist_squared -= (S.z < C1.z) * squared(S.z - C1.z);
	dist_squared -= (S.z >= C1.z) * (S.z < C2.z) * squared(S.z - C2.z);

	return dist_squared > 0;*/
	float3 closestPointInAabb = min(max(S, C1), C2);
	float distance = length(closestPointInAabb - S);
	return distance < R;
}

void depthFirst(uint clusterId, uint lane, uint warpid, float3 center, float3 radius)
{
	uint depth = 1;
	uint address = 1;
	uint pos[5] = { 0, 0, 0, 0, 0 };
	Pair pair;
	uint index;
	bool result;
	uint i;
	AABBNode tempNode;

	branch[warpid][depth][lane] = treeNodes[address + lane].active &&
		checkAABBintersection(treeNodes[address + lane], center, radius);
	
	//if (lane == 4 && treeNodes[address + lane].active && checkAABBintersection(treeNodes[address + lane], center, radius))
	//	clusters[clusterId].lightCount = true;
	//return;
	
	//if (branch[warpid][depth][lane])
	//	clusters[clusterId].lightCount = true;
	//return;
	while (depth > 0)
	{
		i = pos[depth];
		for (; i < 32; i++)
		{
			if (branch[warpid][depth][i])
			{
				//clusters[clusterId].lightCount++;


				tempNode = treeNodes[(address << 5) + 1 + lane];
				if (depth == maxDepth - 2)
				{
					index = totalCounts[warpid];
					result = tempNode.active && checkAABBsphere(center, radius, tempNode.center, tempNode.radius.x);
						//checkAABBintersection(tempNode, center, radius);
					//if (result)
					//	clusters[clusterId].lightCount = true;
					lightCounts[warpid][lane] = result;


					if (lane >= 1) lightCounts[warpid][lane] = lightCounts[warpid][lane - 1] + lightCounts[warpid][lane];
					if (lane >= 2) lightCounts[warpid][lane] = lightCounts[warpid][lane - 2] + lightCounts[warpid][lane];
					if (lane >= 4) lightCounts[warpid][lane] = lightCounts[warpid][lane - 4] + lightCounts[warpid][lane];
					if (lane >= 8) lightCounts[warpid][lane] = lightCounts[warpid][lane - 8] + lightCounts[warpid][lane];
					if (lane >= 16) lightCounts[warpid][lane] = lightCounts[warpid][lane - 16] + lightCounts[warpid][lane];

					pair.clusterIndex = clusterId;
					pair.lightIndex = tempNode.index;
					pair.lightOffset = totalCounts[warpid] + lightCounts[warpid][lane] - 1;
					pair.valid = true;

					if (result)
						pairAppendBuffer.Append(pair);

					if (lane == 31) totalCounts[warpid] += lightCounts[warpid][31];
				}
				else
				{
					pos[depth] = i + 1;
					depth++;
					address = (address << 5) + 1;
					branch[warpid][depth][lane] = tempNode.active &&
						checkAABBintersection(tempNode, center, radius);
					break;
				}
			}
			address += 1;
		}
		if (i == 32)
		{
			pos[depth] = 0;
			depth--;
			address >>= 5;
		}
	}
}

[numthreads(32, 32, 1)]
void main( 
	uint3 Gid : SV_GroupID,
	uint3 GTid : SV_GroupThreadID
	)
{
	uint clusterId = GTid.y + Gid.x * 32;
	uint lane = GTid.x;
	uint warpid = GTid.y;
	uint clusterNum = clusters[clusterId].clusterNum;

	if (!clusters[clusterId].active) return;
	lightCounts[warpid][lane] = 0;
	if (lane == 0)
		totalCounts[warpid] = 0;

	float3 maximum = -float3(1000, 1000, 1000);
	float3 minimum = float3(1000, 1000, 1000);
	for (uint z = 0; z < 2; z++)
		for (uint x = 0; x < 2; x++)
			for (uint y = 0; y < 2; y++)
			{
				float clusterZ = ((clusterNum >> 12) & 0x3FF) + z;
				float eye_z = exp(clusterZ * denominator) * 0.01f;
				float2 screenSpaceCoord = float2((clusterNum >> 6) & 0x3F, (clusterNum & 0x3F)) + float2(x, y);
				screenSpaceCoord = 64 * screenSpaceCoord / float2(width, -(float)height) + float2(-1.0, 1.0);
				float3 corner = float3(screenSpaceCoord * float2(invFocalLenX, invFocalLenY) * eye_z, -eye_z);
				maximum = max(maximum, corner);
				minimum = min(minimum, corner);
			}
	
	float3 center = (minimum + maximum) * 0.5;
	float3 radius = abs(maximum - center);

	depthFirst(clusterId, lane, warpid, center, radius);

	if (lane == 0)
	{
		clusters[clusterId].lightCount = totalCounts[warpid];
		clusters[clusterId].lightOffset = totalCounts[warpid];
	}
}