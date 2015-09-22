#define N 1024

RWTexture2D<uint> data : register(u0);
RWTexture2D<uint> clusterCounts : register(u1);

groupshared uint temp[N];

uint scan_warp_sum(uint index)
{
	uint lane = index & 31; // index of thread in warp (0..31)
	if (lane >= 1) temp[index] = temp[index - 1] + temp[index];
	if (lane >= 2) temp[index] = temp[index - 2] + temp[index];
	if (lane >= 4) temp[index] = temp[index - 4] + temp[index];
	if (lane >= 8) temp[index] = temp[index - 8] + temp[index];
	if (lane >= 16) temp[index] = temp[index - 16] + temp[index];
	return temp[index];
}

[numthreads(32, 32, 1)]
void main(
	uint3 Gid : SV_GroupID,
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID
	)
{
	uint lane = GTid.x;
	uint warpid = GTid.y;
	uint index = GTid.x + 32 * GTid.y;

	// Load the cluster number into shared memory
	uint val = data[DTid.xy] >> 10;
	temp[index] = val;
	GroupMemoryBarrierWithGroupSync();
	
	uint prev = temp[index - 1];
	GroupMemoryBarrierWithGroupSync();

	// Initialize the counts.
	uint first = ((index == 0) || (prev < val));
	temp[index] = first;
	
	// Step 1: Intra-warp scan in each warp
	val = scan_warp_sum(index);
	GroupMemoryBarrierWithGroupSync();

	// Step 2: Collect per-warp partial results
	if (lane == 31) temp[warpid] = temp[index];
	GroupMemoryBarrierWithGroupSync();

	// Step 3: Use 1st warp to scan per-warp results
	if (warpid == 0) scan_warp_sum(index);
	GroupMemoryBarrierWithGroupSync();

	// Step 4: Accumulate results from Steps 1 and 3
	if (warpid > 0) val = temp[warpid - 1] + val;

	data[DTid.xy] |= (first << 30) | (val << 20);

	if (warpid == 31 && lane == 31)
		clusterCounts[Gid.xy] = val;
}