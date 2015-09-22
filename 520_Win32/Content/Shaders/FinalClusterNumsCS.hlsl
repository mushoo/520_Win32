struct Cluster
{
	uint clusterNum;
	uint lightCount;
	uint lightOffset;
	uint padding;
};

RWTexture2D<uint> data : register(u0);
RWTexture2D<uint> clusterCounts : register(u1);
RWStructuredBuffer<Cluster> clusters : register(u2);

groupshared uint temp[32][32];

[numthreads(32, 32, 1)]
void main(
	uint3 Gid : SV_GroupID,
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID
	)
{
	uint value = data[DTid.xy];
	uint depth = (value >> 10) & 0x3FF;
	uint2 coord = uint2((value >> 5) & 0x1F, value & 0x1F);
	uint offset = (value >> 20) & 0x3FF;
	bool first = value >> 30;

	uint tmp = Gid.x + Gid.y * 32 - 1;
	uint2 prevIndex = uint2(tmp & 0x1F, tmp >> 5);
	temp[coord.y][coord.x] = offset + clusterCounts[prevIndex] - 1;
	
	Cluster c;
	c.clusterNum = (depth << 12) | (Gid.x << 6) | (Gid.y);
	c.lightCount = 0;
	c.lightOffset = 0;
	c.padding = 0;
	if (first)
		clusters[offset + clusterCounts[prevIndex] - 1] = c;
		
	GroupMemoryBarrierWithGroupSync();
	data[DTid.xy] = temp[GTid.y][GTid.x];
}