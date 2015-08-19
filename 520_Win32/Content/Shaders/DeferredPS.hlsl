Texture2D difTexture;
SamplerState difSampler;

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 norm : NORMAL;
	float3 worldPos : TEXCOORD1;
	float depth : TEXCOORD2;
};

struct PSoutput
{
	float4 color : SV_Target0;
	float4 position: SV_Target1;
	float4 normal : SV_Target2;
};

PSoutput main(PixelShaderInput input) : SV_TARGET
{
	PSoutput output;
	output.color = difTexture.Sample(difSampler, input.texcoord);
	output.position = float4(input.worldPos, input.depth);
	output.normal = float4(input.norm, 1.0f);
	return output;
}