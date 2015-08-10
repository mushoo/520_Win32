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
	float2 tex;
	tex.x = input.tex.x * 2.0 - (float)(input.tex.x >= 0.5);
	tex.y = input.tex.y * 2.0 - (float)(input.tex.y >= 0.5);
	uint section = (uint)(input.tex.x >= 0.5) + (uint)(input.tex.y >= 0.5) * 2.0;
	
	float4 diffuseColor = colorTexture.Sample(pointSampler, tex);
	float4 position = positionTexture.Sample(pointSampler, tex);
	float4 normal = normalTexture.Sample(pointSampler, tex);

	float3 ambientColor = diffuseColor * 0.2;

	float3 color = ambientColor;
	float lightIntensity = saturate(dot(normal.xyz, normalize(lightPos - position.xyz)));
	if (lightIntensity > 0.0f)
	{
		color += (diffuseColor * lightIntensity);
		color = saturate(color);
	}


	float4 zcolor;
	zcolor.x = abs(ddx(position.w));
	zcolor.y = abs(ddy(position.w));
	zcolor.z = 0.0;
	zcolor.w = 0.0;

	switch (section) {
	case 0: return float4(color, 1.0f);
	case 1: return 1000*zcolor;
	case 2: return normal;
	default: {
		float3 cluster;
		cluster.r = floor(log(-position.w / 0.01f) / log(1 + 2 * tan(1.2217304764) / (768.0f / 32.0f))) / 30.0f;
		cluster.g = floor(input.tex.x * 32) / 32.0f;
		cluster.b = floor(input.tex.y * 24) / 24.0f;
		return float4(cluster.rrr, 1.0);
	}
	}
}