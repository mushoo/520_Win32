struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float3 color : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	return float4(input.color, 1.0f);
}