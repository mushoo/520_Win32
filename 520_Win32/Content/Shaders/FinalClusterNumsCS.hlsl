RWTexture2D<uint> data : register(u0);
RWTexture2D<uint> clusterCounts : register(u1);

groupshared uint temp[1024];

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
	uint sum = value >> 20;
	temp[32 * coord.y + coord.x] = sum + clusterCounts[Gid.xy] - 1;

	GroupMemoryBarrierWithGroupSync();
	data[DTid.xy] = temp[32 * GTid.y + GTid.x];
}