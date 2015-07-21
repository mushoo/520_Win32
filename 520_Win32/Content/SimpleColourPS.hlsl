Texture2D difTexture;
SamplerState difSampler;

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 norm : NORMAL;
	float3 lightPos : TEXCOORD1;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	//float3 ambientColor = float3(0.1, 0.1, 0.3);
	float3 diffuseColor = difTexture.Sample(difSampler, input.texcoord);
	float3 ambientColor = diffuseColor * 0.2;
	//return float4(input.norm, 1.0f);

	float3 color = ambientColor;
	float lightIntensity = saturate(dot(input.norm, input.lightPos));
	if (lightIntensity > 0.0f)
	{
		color += (diffuseColor * lightIntensity);
		color = saturate(color);
	}
	return float4(color, 1.0f);
}