// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main(uint id : SV_VERTEXID)
{
	// Generate fullscreen triangle.
	// From: http://www.slideshare.net/DevCentralAMD/vertex-shader-tricks-bill-bilodeau, slides 12--14.
	PixelShaderInput output;
	output.pos.x = (float)(id / 2) * 4.0 - 1.0;
	output.pos.y = (float)(id % 2) * 4.0 - 1.0;
	output.pos.z = 0.0;
	output.pos.w = 1.0;

	output.tex.x = (float)(id / 2) * 2.0;
	output.tex.y = 1.0 - (float)(id % 2) * 2.0;
	return output;
}
