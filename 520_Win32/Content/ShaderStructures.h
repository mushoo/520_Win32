#pragma once

namespace _520
{
	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};

	struct LightScreenDimBuffer
	{
		DirectX::XMFLOAT3 position;
		UINT windowWidth;
		UINT windowHeight;
		float padding[3]; // Required for 16-byte boundary reasons.
	};

	struct AssignClustersBuffer
	{
		float denominator;
		float padding[3]; // Required for 16-byte boundary reasons.
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionTextureNormal
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 texcoord;
		DirectX::XMFLOAT3 normal;
	};
}