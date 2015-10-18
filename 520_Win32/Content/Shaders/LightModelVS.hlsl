cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 norm : NORMAL;
	float3 worldPos : INSTANCEPOS;
	float3 color : INSTANCECOLOR;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 norm : NORMAL;
	float3 color : TEXCOORD0;
};

PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

	// Transform the vertex position into projected space.
	pos.xyz *= 0.0015;
	pos.xyz += input.worldPos;
	pos = mul(pos, view);
	pos = mul(pos, projection);
	output.pos = pos;

	output.norm = mul(input.norm, (float3x3)model);
	output.norm = normalize(output.norm);

	output.color = input.color;

	return output;
}