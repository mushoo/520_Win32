Texture2D colorTexture : register(t0);
Texture2D positionTexture : register(t1);
Texture2D normalTexture : register(t2);
SamplerState pointSampler;

cbuffer LightPositionBuffer : register(b0)
{
	float3 lightPos;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	float4 diffuseColor = colorTexture.Sample(pointSampler, input.tex);
	float4 position = positionTexture.Sample(pointSampler, input.tex);
	float4 normal = normalTexture.Sample(pointSampler, input.tex);

	float3 ambientColor = diffuseColor * 0.2;

	float3 color = ambientColor;
	float lightIntensity = saturate(dot(normal.xyz, normalize(lightPos - position.xyz)));
	if (lightIntensity > 0.0f)
	{
		color += (diffuseColor * lightIntensity);
		color = saturate(color);
	}
	return float4(color, 1.0f);
}