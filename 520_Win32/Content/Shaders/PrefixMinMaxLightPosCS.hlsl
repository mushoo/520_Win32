struct AABBNode
{
	float3 max;
	float3 min;
	uint index;
	bool active;
};

struct Light
{
	float3 position;
	float3 color;
	float radius;
	bool active;
};

cbuffer ConstantBuffer : register(b0)
{
	uint depth;
	bool first;
};

cbuffer ModelViewProjectionConstantBuffer : register(b1)
{
	matrix model;
	matrix view;
	matrix projection;
};

RWStructuredBuffer<AABBNode> treeNodes : register(u0);
StructuredBuffer<Light> lightList: register(t0);

groupshared float3 temp[32][32];

uint parent(uint child) { return (child - 1) >> 5; }

float3 scan_warp_max(uint warpid, uint lane)
{
	if (lane >= 1) temp[warpid][lane] = max(temp[warpid][lane - 1], temp[warpid][lane]);
	if (lane >= 2) temp[warpid][lane] = max(temp[warpid][lane - 2], temp[warpid][lane]);
	if (lane >= 4) temp[warpid][lane] = max(temp[warpid][lane - 4], temp[warpid][lane]);
	if (lane >= 8) temp[warpid][lane] = max(temp[warpid][lane - 8], temp[warpid][lane]);
	if (lane >= 16) temp[warpid][lane] = max(temp[warpid][lane - 16], temp[warpid][lane]);
	return temp[warpid][lane];
}

float3 scan_warp_min(uint warpid, uint lane)
{
	if (lane >= 1) temp[warpid][lane] = min(temp[warpid][lane - 1], temp[warpid][lane]);
	if (lane >= 2) temp[warpid][lane] = min(temp[warpid][lane - 2], temp[warpid][lane]);
	if (lane >= 4) temp[warpid][lane] = min(temp[warpid][lane - 4], temp[warpid][lane]);
	if (lane >= 8) temp[warpid][lane] = min(temp[warpid][lane - 8], temp[warpid][lane]);
	if (lane >= 16) temp[warpid][lane] = min(temp[warpid][lane - 16], temp[warpid][lane]);
	return temp[warpid][lane];
}

[numthreads(32, 32, 1)]
void main(
	uint3 Gid : SV_GroupID,
	uint3 GTid : SV_GroupThreadID
	)
{
	uint lane = GTid.x;
	uint warpid = GTid.y;
	uint index = 1024 * Gid.x + 32 * warpid + lane;
	uint offset = ((1 << 5 * depth) - 1) / 31;
	float3 firstPos;

	// Do max.
	if (first)
	{
		float4 pos;
		if (lightList[index].active)
		{
			pos = float4(lightList[index].position, 1.0f);
			// Put into view space.
			pos = mul(pos, view);
			treeNodes[index + offset].max = pos.xyz;
			treeNodes[index + offset].min = lightList[index].radius.xxx;
			treeNodes[index + offset].index = index;
			treeNodes[index + offset].active = true;
		}
		else {
			pos = float4(lightList[0].position, 1.0f);
			pos = mul(pos, view);
		}
		temp[warpid][lane] = pos.xyz;
		firstPos = pos.xyz;
	}
	else if (index < (1 << 5 * depth) && treeNodes[index + offset].active)
		temp[warpid][lane] = treeNodes[index + offset].max;
	else temp[warpid][lane] = treeNodes[0 + offset].max;

	scan_warp_max(warpid, lane);
	GroupMemoryBarrierWithGroupSync();

	if (warpid == 0) {
		temp[0][lane] = temp[lane][31];
		scan_warp_max(warpid, lane);
	}

	if (warpid == 0 && lane == 0) {
		treeNodes[parent(index + offset)].max = temp[0][31];
		treeNodes[parent(parent(index + offset))].max = temp[0][31];
	}
	
	GroupMemoryBarrierWithGroupSync();

	// Do min.
	if (index < (1 << 5 * depth) && treeNodes[index + offset].active)
		temp[warpid][lane] = (first ? firstPos : treeNodes[index + offset].min);
	else temp[warpid][lane] = (first ? firstPos : treeNodes[0 + offset].min);

	scan_warp_min(warpid, lane);
	GroupMemoryBarrierWithGroupSync();

	if (warpid == 0) {
		temp[0][lane] = temp[lane][31];
		scan_warp_min(warpid, lane);
	}

	if (warpid == 0 && lane == 0){
		treeNodes[parent(index + offset)].min = temp[0][31];
		treeNodes[parent(parent(index + offset))].min = temp[0][31];
	}
}