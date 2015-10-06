struct Cluster
{
	uint clusterNum;
	uint lightCount;
	uint lightOffset;
	bool active;
};

cbuffer ConstantBuffer : register(b0)
{
	uint stride;
	bool finish;
};

RWStructuredBuffer<Cluster> clusters : register(u0);

groupshared uint temp[32][32];

uint scan_warp_sum(uint warpid, uint lane)
{
	if (lane >= 1) temp[warpid][lane] = temp[warpid][lane - 1] + temp[warpid][lane];
	if (lane >= 2) temp[warpid][lane] = temp[warpid][lane - 2] + temp[warpid][lane];
	if (lane >= 4) temp[warpid][lane] = temp[warpid][lane - 4] + temp[warpid][lane];
	if (lane >= 8) temp[warpid][lane] = temp[warpid][lane - 8] + temp[warpid][lane];
	if (lane >= 16) temp[warpid][lane] = temp[warpid][lane - 16] + temp[warpid][lane];
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
	uint index = GTid.x + 32 * GTid.y + 1024 * Gid.x;
	index *= stride;
	//if (!clusters[1024 * stride * Gid.x].active) return;
	if (!finish)
	{
		uint first = clusters[index].lightOffset;
		temp[warpid][lane] = first;

		uint val = scan_warp_sum(warpid, lane);
		GroupMemoryBarrierWithGroupSync();

		if (warpid == 0) {
			temp[0][lane] = temp[lane][31];
			scan_warp_sum(warpid, lane);
		}
		GroupMemoryBarrierWithGroupSync();

		if (warpid > 0) val = temp[0][warpid - 1] + val;

		if (warpid == 0 && lane == 0)
			clusters[index].lightOffset = temp[0][31];
		else
			clusters[index].lightOffset = val - first;
	}
	else
	{
		if (Gid.x != 0 && (warpid != 0 || lane != 0))
			clusters[index].lightOffset += clusters[1024 * stride * Gid.x].lightOffset;
	}
}