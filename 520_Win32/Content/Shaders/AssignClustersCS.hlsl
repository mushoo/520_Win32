Texture2D<float4> input : register(t0);
RWTexture2D<uint> output : register(u0);

cbuffer ConstantBuffer : register(b0)
{
	float denominator;
};

[numthreads(32, 32, 1)]
void main( 
	uint3 Gid : SV_GroupID,
	uint3 GTid : SV_GroupThreadID,
	uint3 DTid : SV_DispatchThreadID 
	)
{
	float depth = input[DTid.xy].w;
	uint tileX = GTid.x & 0x1F; //5 bits
	uint tileY = GTid.y & 0x1F; //5 bits
	uint clusterZ = (uint)floor(log(-depth / 0.01f) * denominator) & 0x3FF; //10 bits
	output[DTid.xy] = (clusterZ << 10) | (tileX << 5) | tileY;
}