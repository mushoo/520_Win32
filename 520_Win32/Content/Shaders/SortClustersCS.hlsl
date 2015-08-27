#define N 1024

RWTexture2D<uint> data : register(u0);

groupshared uint temp[N];

[numthreads(32, 32, 1)]
void main(
	uint3 Gid : SV_GroupID,
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID
	)
{
	uint res;

	uint index = GTid.x + 32 * GTid.y;

	// Load the cluster number into shared memory
	temp[index] = data[DTid.xy];

	GroupMemoryBarrierWithGroupSync();  // Wait for all threads to complete

	// Sorting in thread local memory
	for (uint b = 2; b <= N; b <<= 1)
	{
		for (uint d = (b >> 1); d >= 1; d >>= 1)
		{
			// Calculate the new value
			res = temp[(((((index & b) == 0) ^ ((index & d) == 0)) == (temp[index] < temp[index ^ d])) ? (index ^ d) : (index))];
			GroupMemoryBarrierWithGroupSync();

			// Write new value back to shared memory
			temp[index] = res;
			GroupMemoryBarrierWithGroupSync();
		}
	}

	// Put final sorted value in place in global memory
	data[DTid.xy] = temp[index];
}