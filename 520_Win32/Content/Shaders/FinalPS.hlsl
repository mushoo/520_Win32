Texture2D colorTexture : register(t0);
Texture2D positionTexture : register(t1);
Texture2D normalTexture : register(t2);
Texture2D<uint> clusterAssignTexture : register(t3);

SamplerState pointSampler;

cbuffer LightPositionBuffer : register(b0)
{
	float3 lightPos;
	uint width;
	uint height;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

float rand(float2 co){
	return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

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
	case 2: {
		/*uint clusterNum = clusterAssignTexture[tex * uint2(width, height)];
		float color = (float)(clusterNum & 0xFF) / 30.0f;*/
		return float4(0, 0, 0, 1.0f);
	}
	default: {
		uint clusterNum = clusterAssignTexture[tex * uint2(width, height)];
		float randNum = rand(clusterNum);
		uint3 cluster;
		cluster.x = (clusterNum >> 16) & 0xFF;
		cluster.y = (clusterNum >> 8) & 0xFF;
		cluster.z = clusterNum & 0xFF;
		float3 color;
		color.r = rand(float2(cluster.x, cluster.y));
		color.g = rand(float2(cluster.y, cluster.z));
		color.b = rand(float2(cluster.z, cluster.x));
		return float4(color, 1.0f);
	}
	}
}