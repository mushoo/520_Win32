struct Cluster
{
	uint clusterNum;
	uint lightCount;
	uint lightOffset;
	uint padding;
};

struct Pair
{
	uint clusterIndex;
	uint lightIndex;
	uint lightOffset;
	bool valid;
};

RWStructuredBuffer<Cluster> clusters : register(u0);
RWBuffer<uint> clusterLightLists : register(u1);
StructuredBuffer<Pair> pairs : register(t0);

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint index = DTid.x;
	if (!pairs[index].valid) return;

	uint clusterIndex = pairs[index].clusterIndex;
	uint lightOffset = 0;
	if (clusterIndex != 0)
		lightOffset = clusters[clusterIndex].lightOffset;
	lightOffset += pairs[index].lightOffset;
	clusterLightLists[lightOffset] = pairs[index].lightIndex;
}