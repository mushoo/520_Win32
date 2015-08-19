#define N 1024

RWTexture2D<uint> data : register(u0);

groupshared uint temp[N];

uint scan_warp(uint index)
{
	uint lane = index & 31; // index of thread in warp (0..31)
	if (lane >= 1) temp[index] = max(temp[index - 1], temp[index]);
	if (lane >= 2) temp[index] = max(temp[index - 2], temp[index]);
	if (lane >= 4) temp[index] = max(temp[index - 4], temp[index]);
	if (lane >= 8) temp[index] = max(temp[index - 8], temp[index]);
	if (lane >= 16) temp[index] = max(temp[index - 16], temp[index]);
	return temp[index];
}

[numthreads(32, 32, 1)]
void main(
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID
	)
{
	uint lane = GTid.x;
	uint warpid = GTid.y;
	uint index = GTid.x + 32 * GTid.y;

	// Load the cluster number into shared memory
	temp[index] = data[DTid.xy];

	// Step 1: Intra - warp scan in each warp
	uint val = scan_warp(index);
	GroupMemoryBarrierWithGroupSync();

	// Step 2: Collect per - warp partial results
	if (lane == 31) temp[warpid] = temp[index];
	GroupMemoryBarrierWithGroupSync();

	// Step 3: Use 1 st warp to scan per - warp results
	if (warpid == 0) scan_warp(index);
	GroupMemoryBarrierWithGroupSync();

	// Step 4: Accumulate results from Steps 1 and 3
	if (warpid > 0) val = max(temp[warpid - 1], val);
	GroupMemoryBarrierWithGroupSync();

	// Step 5: Write and return the final result
	temp[index] = val;
	GroupMemoryBarrierWithGroupSync();

	if (index == 0 || temp[index - 1] < val)
		data[DTid.xy] = val;
	else data[DTid.xy] = 0;
}