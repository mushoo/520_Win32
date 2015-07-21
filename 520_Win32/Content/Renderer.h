#pragma once

#include "..\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\StepTimer.h"
#include "SimpleMesh.h"
#include "objLoader\tiny_obj_loader.h"
#include "Camera.h"

namespace _520
{
	struct TriangleIndices
	{
		unsigned int index[3];
	};

	struct Material
	{
		std::string m_name;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_diffuseView;
		mutable std::vector<unsigned int> m_meshIndices;
	};

	// This sample renderer instantiates a basic rendering pipeline.
	class Renderer
	{
	public:
		Renderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		//void CreateTiledResources();
		//void CreateCubes();
		void LoadModels();
		void LoadTexture(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> &view);
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		std::shared_ptr<Camera> GetCamera() { return m_camera; }

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::shared_ptr<Camera> m_camera = nullptr;

		// D3D resources.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>	m_sampler;
		std::vector<std::shared_ptr<_520::Material>>		m_materials;

		ModelViewProjectionConstantBuffer	m_constantBufferData;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;

		// Meshes and Materials.
		std::vector<SimpleMesh> m_meshes;
		std::vector<VertexPositionTextureNormal> m_vertices;
		std::vector<TriangleIndices> m_triIndices;
		int m_nVertices;
		int m_nTriIndices;

		// Sun light.
		XMFLOAT3 m_light;
		LightPositionBuffer m_lightCBufferData;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_lightCBuffer;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
	};
}

