#pragma once

#include "..\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\StepTimer.h"
#include "SimpleMesh.h"
#include "objLoader\tiny_obj_loader.h"
#include "Camera.h"


namespace _520
{
#define MAXCLUSTERS 100000
#define MAXPAIRS 100000

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

	struct Cluster
	{
		UINT32 clusterNum;
		UINT32 lightCount;
		UINT32 lightOffset;
		UINT32 padding;
	};

	struct Pair
	{
		UINT32 clusterIndex;
		UINT32 lightIndex;
		UINT32 lightOffset;
		UINT32 valid;
	};

	// This sample renderer instantiates a basic rendering pipeline.
	class Renderer
	{
	public:
		Renderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void CreateClusterResources();
		void CreateLightResources();
		//void CreateTiledResources();
		//void CreateCubes();
		void LoadModels();
		void LoadTexture(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> &view);
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		void ClearViews();
		void RenderGBuffers();
		void AssignClusters();
		void SortClusters();
		void CalcClusterNums();
		void ConstructLightTree();
		void TraverseTree();
		void SumClusterLightCounts();
		void MakeLightList();
		void RenderFinal();
		std::shared_ptr<Camera> GetCamera() { return m_camera; }

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		std::shared_ptr<Camera> m_camera = nullptr;

		// D3D resources.
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_finalVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_finalPS;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	m_assignClustersCS;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	m_sortClustersCS;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	m_compactClustersCS;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	m_compactClusters2CS;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	m_finalClusterNumsCS;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	m_AABBConstructTreeCS;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	m_traverseTreeCS;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	m_sumClusterLightCountsCS;
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>	m_makeLightListCS;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>	m_linearSampler;
		Microsoft::WRL::ComPtr<ID3D11SamplerState>	m_pointSampler;
		std::vector<std::shared_ptr<_520::Material>>m_materials;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_deferredVS;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_deferredPS;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		
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
		LightScreenDimBuffer m_lightScreenDimCBufferData;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_lightScreenDimCBuffer;

		// Cluster buffer.
		ID3D11Buffer *m_clusterBuffer;
		ID3D11UnorderedAccessView *m_clusterView;
		ID3D11ShaderResourceView *m_clusterResourceView;
		ID3D11UnorderedAccessView *m_pairListView;
		ID3D11ShaderResourceView *m_pairListResourceView;
		ID3D11UnorderedAccessView *m_clusterLightListsView;
		ID3D11ShaderResourceView *m_clusterLightListsResourceView;
		UINT32 m_numClusters;

		// Light system buffer.
		ID3D11Buffer *m_treeBuffer;
		ID3D11UnorderedAccessView *m_lightTreeView;
		ID3D11ShaderResourceView *m_lightTreeResourceView;
		ID3D11Buffer *m_lightTreeCBuffer;
		ID3D11Buffer *m_lightListBuffer;
		ID3D11ShaderResourceView *m_lightListResourceView;

		// Compute constant buffers.
		AssignClustersBuffer m_assignClustersCBufferData;
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_assignClustersCBuffer;
		ID3D11Texture2D *m_tempCopyResource;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float m_fovAngleY;
	};
}

