Texture2D<float4> input : register(t0);
RWTexture2D<uint> output : register(u0);

cbuffer ConstantBuffer : register(b0)
{
	float denominator;
};

[numthreads(32, 32, 1)]
void main( 
	uint3 Gid : SV_GroupID,
	uint3 DTid : SV_DispatchThreadID 
	)
{
	float depth = input[DTid.xy].w;
	uint clusterX = Gid.x;
	uint clusterY = Gid.y;
	uint clusterZ = floor(log(-depth / 0.01f) * denominator);
	output[DTid.xy] = (clusterX << 16) | (clusterY << 8) | clusterZ;
}