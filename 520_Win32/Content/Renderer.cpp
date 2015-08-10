#include "pch.h"
#include "Renderer.h"
#include "DirectXHelper.h"
#include "DDSLoader\DDSTextureLoader.h"
#include "DirectXTex\DirectXTex.h"

#include <algorithm>
#include <map>
#include <set>
#include <locale>
#include <codecvt>
#include <string>

using namespace _520;

using namespace DirectX;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Renderer::Renderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_deviceResources(deviceResources),
	m_camera(std::make_shared<Camera>())
{
	ID3D11RasterizerState *rs;
	D3D11_RASTERIZER_DESC rsdesc;
	ZeroMemory(&rsdesc, sizeof(D3D11_RASTERIZER_DESC));
	rsdesc.FillMode = D3D11_FILL_SOLID;
	rsdesc.CullMode = D3D11_CULL_NONE;
	rsdesc.FrontCounterClockwise = true;
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateRasterizerState(&rsdesc, &rs)
		);
	m_deviceResources->GetD3DDeviceContext()->RSSetState(rs);

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
	m_light = { 0, 0, 10.0f };
}

void Renderer::LoadTexture(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> &view)
{
	Microsoft::WRL::ComPtr<ID3D11Resource> texture;

	std::string str = "Assets\\crytek-sponza\\" + name;
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(str);
	const wchar_t *filename = wide.c_str();

	DX::ThrowIfFailed(
		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), filename, &texture, &view)
		);
}

void Renderer::LoadModels()
{
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, "Assets/crytek-sponza/sponza.obj", "Assets/crytek-sponza/");
	if (err != "") exit(0);

	m_nVertices = 0;
	m_nTriIndices = 0;
	for (int i = 0; i < shapes.size(); i++)
	{
		int nVertices = shapes[i].mesh.positions.size() / 3;
		int nTriIndices = shapes[i].mesh.indices.size() / 3;
		m_meshes.push_back(SimpleMesh{ m_nVertices, nVertices, m_nTriIndices, nTriIndices });
		m_nVertices += nVertices;
		m_nTriIndices += nTriIndices;
	}
	m_vertices.resize(m_nVertices);
	m_triIndices.resize(m_nTriIndices);
	for (int i = 0; i < m_meshes.size(); i++)
	{
		SimpleMesh mesh = m_meshes[i];
		for (int j = 0; j < mesh.m_nVertices; j++)
		{
			tinyobj::mesh_t m = shapes[i].mesh;
			XMFLOAT3 position(m.positions[3 * j], m.positions[3 * j + 1], m.positions[3 * j + 2]);
			XMFLOAT2 texCoord(m.texcoords[2 * j], m.texcoords[2 * j + 1]);
			XMFLOAT3 normal(m.normals[3 * j], m.normals[3 * j + 1], m.normals[3 * j + 2]);
			m_vertices[j + mesh.m_vertexOffset].pos = position;
			m_vertices[j + mesh.m_vertexOffset].texcoord = texCoord;
			m_vertices[j + mesh.m_vertexOffset].normal = normal;
		}
		for (int j = 0; j < mesh.m_nTriIndices; j++)
		{
			m_triIndices[j + mesh.m_triIndexOffset].index[0] = m_meshes[i].m_vertexOffset + shapes[i].mesh.indices[3 * j];
			m_triIndices[j + mesh.m_triIndexOffset].index[1] = m_meshes[i].m_vertexOffset + shapes[i].mesh.indices[3 * j + 1];
			m_triIndices[j + mesh.m_triIndexOffset].index[2] = m_meshes[i].m_vertexOffset + shapes[i].mesh.indices[3 * j + 2];
		}
	}
	
	// Make a list of Materials, each one with a list of the meshes it is to be applied to. 
	//std::map<std::string, Microsoft::WRL::ComPtr<ID3D11Resource>> diffuses;
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> diffuseViews;
	auto comp = [](std::shared_ptr<Material> a, std::shared_ptr<Material> b)->bool{
		return a->m_name < b->m_name;
	};
	std::set <std::shared_ptr<Material>, std::function<bool(std::shared_ptr<Material>, std::shared_ptr<Material>)>> materialsCache(comp);
	for (int i = 0; i < m_meshes.size(); i++)
	{
		for (auto matId : shapes[i].mesh.material_ids)
		{
			std::string name = materials[matId].diffuse_texname;
			if (name.empty())
				continue;

			if (diffuseViews.find(name) == diffuseViews.end())
			{
				LoadTexture(name, diffuseViews[name]);
			}
			
			auto pMaterial = std::make_shared<Material>();
			pMaterial->m_name = name;
			pMaterial->m_diffuseView = diffuseViews[name];

			auto matIt = materialsCache.insert(pMaterial).first;
			if (std::find((*matIt)->m_meshIndices.begin(), (*matIt)->m_meshIndices.end(), i) ==
				(*matIt)->m_meshIndices.end())
			{
				(*matIt)->m_meshIndices.push_back(i);
			}
		}
	}
	m_materials.assign(materialsCache.begin(), materialsCache.end());
}

// Initializes view parameters when the window size changes.
void Renderer::CreateWindowSizeDependentResources()
{
	RECT windowSize = m_deviceResources->GetWindowSize();

	UINT width = windowSize.right - windowSize.left;
	UINT height = windowSize.bottom - windowSize.top;

	float aspectRatio = width / (float)height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMStoreFloat4x4(
		&m_constantBufferData.projection,
		XMMatrixTranspose(perspectiveMatrix)
		);

	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(m_camera->GetView()));
	XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixScaling(0.001f, 0.001f, 0.001f)));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Renderer::Update(DX::StepTimer const& timer)
{
	m_camera->Update(timer);
}

void Renderer::Render()
{
	if (!m_loadingComplete)
	{
		return;
	}

	RenderGBuffers();

	// Now we draw the full-screen quad.
	auto context = m_deviceResources->GetD3DDeviceContext();

	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Attach our vertex shader.
	context->VSSetShader(
		m_finalVS.Get(),
		nullptr,
		0
		);
	// Attach our pixel shader.
	context->PSSetShader(
		m_finalPS.Get(),
		nullptr,
		0
		);

	m_lightCBufferData.position = m_light;
	context->UpdateSubresource(
		m_lightCBuffer.Get(),
		0,
		NULL,
		&m_lightCBufferData,
		0,
		0
		);
	context->PSSetConstantBuffers(
		0,
		1,
		m_lightCBuffer.GetAddressOf()
		);

	context->PSSetSamplers(
		0,
		1,
		m_pointSampler.GetAddressOf()
		);

	// Set the texture G-bufferinos for the final shading.
	context->PSSetShaderResources(0, m_deviceResources->GBUFFNUM, &m_deviceResources->m_d3dGBufferResourceViews[0]);

	// Set index and vertex buffers to NULL.
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetIndexBuffer(nullptr, (DXGI_FORMAT)0, 0);
	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->Draw(3, 0);

	// Unset everything we used that's not going to be unset by the next draw.
	std::vector<ID3D11ShaderResourceView *> nullBuffer(m_deviceResources->GBUFFNUM, 0);
	context->PSSetShaderResources(0, m_deviceResources->GBUFFNUM, &nullBuffer[0]);
}

void Renderer::RenderGBuffers()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	for (ID3D11RenderTargetView *view : m_deviceResources->m_d3dGBufferTargetViews)
	{
		if (view != nullptr)
			context->ClearRenderTargetView(view, DirectX::Colors::CornflowerBlue);
	}

	// Set render targets.
	context->OMSetRenderTargets(
		m_deviceResources->GBUFFNUM,
		&m_deviceResources->m_d3dGBufferTargetViews[0],
		m_deviceResources->GetDepthStencilView()
		);

	// Attach our vertex shader.
	context->VSSetShader(
		m_deferredVS.Get(),
		nullptr,
		0
		);
	// Attach our pixel shader.
	context->PSSetShader(
		m_deferredPS.Get(),
		nullptr,
		0
		);

	// Send the constant buffer to the graphics device.
	XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(m_camera->GetView()));
	context->UpdateSubresource(
		m_constantBuffer.Get(),
		0,
		NULL,
		&m_constantBufferData,
		0,
		0
		);
	context->VSSetConstantBuffers(
		0,
		1,
		m_constantBuffer.GetAddressOf()
		);

	context->PSSetSamplers(
		0,
		1,
		m_linearSampler.GetAddressOf()
		);

	// Each vertex is one instance of the VertexPositionTextureNormal struct.
	context->IASetInputLayout(m_inputLayout.Get());
	UINT stride = sizeof(VertexPositionTextureNormal);
	UINT offset = 0;
	context->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
		);
	context->IASetIndexBuffer(
		m_indexBuffer.Get(),
		DXGI_FORMAT_R32_UINT, // Each index is one 32-bit unsigned integer (short).
		0
		);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto pMaterial : m_materials)
	{
		context->PSSetShaderResources(
			0,
			1,
			pMaterial->m_diffuseView.GetAddressOf()
			);

		for (int i : pMaterial->m_meshIndices)
		{
			context->DrawIndexed(
				m_meshes[i].m_nTriIndices * 3,
				m_meshes[i].m_triIndexOffset * 3,
				0
				);
		}
	}

	// Unset everything we used that's not going to be unset by the next draw.
	std::vector<ID3D11RenderTargetView *> nullRenderTargets(m_deviceResources->GBUFFNUM, 0);
	ID3D11DepthStencilView *nullDepthView = 0;
	context->OMSetRenderTargets(
		m_deviceResources->GBUFFNUM,
		&nullRenderTargets[0],
		nullDepthView
		);

	ID3D11Buffer *nullBuffer[1] = { 0 };
	context->VSSetConstantBuffers(
		0,
		1,
		nullBuffer
		);
}

void Renderer::CreateDeviceDependentResources()
{
	std::vector<byte> vsData = DX::ReadFile("DeferredVS.cso");
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateVertexShader(
		&vsData[0],
		vsData.size(),
		nullptr,
		&m_deferredVS
		)
		);

	static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateInputLayout(
		vertexDesc,
		ARRAYSIZE(vertexDesc),
		&vsData[0],
		vsData.size(),
		&m_inputLayout
		)
		);

	vsData = DX::ReadFile("FinalVS.cso");
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateVertexShader(
			&vsData[0],
			vsData.size(),
			nullptr,
			&m_finalVS
			)
		);

	std::vector<byte> psData = DX::ReadFile("FinalPS.cso");
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreatePixelShader(
			&psData[0],
			psData.size(),
			nullptr,
			&m_finalPS
			)
		);

	psData = DX::ReadFile("DeferredPS.cso");
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreatePixelShader(
		&psData[0],
		psData.size(),
		nullptr,
		&m_deferredPS
		)
		);

	LoadModels();

	// Once the model is loaded, create the buffers to store it.
	
	// Create vertex buffer.
	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = &m_vertices[0];
	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionTextureNormal) * m_nVertices, D3D11_BIND_VERTEX_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexBufferData,
			&m_vertexBuffer
			)
		);

	// Create index buffer.
	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	indexBufferData.pSysMem = &m_triIndices[0];
	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(TriangleIndices) * m_nTriIndices, D3D11_BIND_INDEX_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexBufferData,
			&m_indexBuffer
			)
		);

	// Create MVP constant buffer.
	CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&constantBufferDesc,
			nullptr,
			&m_constantBuffer
			)
		);

	// Create Light constant buffer.
	CD3D11_BUFFER_DESC lightCBufferDesc(sizeof(LightPositionBuffer), D3D11_BIND_CONSTANT_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&lightCBufferDesc,
			nullptr,
			&m_lightCBuffer
			)
		);
	
	
	// Once the texture view is created, create a sampler.This defines how the color
	// for a particular texture coordinate is determined using the relevant texture data.
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	// The sampler does not use anisotropic filtering, so this parameter is ignored.
	samplerDesc.MaxAnisotropy = 0;

	// Specify how texture coordinates outside of the range 0..1 are resolved.
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

	// Use no special MIP clamping or bias.
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	// Don't use a comparison function.
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateSamplerState(
			&samplerDesc,
			&m_linearSampler
			)
		);

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateSamplerState(
		&samplerDesc,
		&m_pointSampler
		)
		);

	// Once the cube is loaded, the object is ready to be rendered.
	
	m_loadingComplete = true;
}

/*void Renderer::CreateTiledResources()
{
	// Create the tiled texture.
	D3D11_TEXTURE2D_DESC textureDesc;
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = 1024;
	textureDesc.Height = 1024;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.MipLevels = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TILED;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateTexture2D(&textureDesc, nullptr, &m_texture));

	// Allocate the tiled resource pool.
	D3D11_BUFFER_DESC tilePoolDesc;
	ZeroMemory(&tilePoolDesc, sizeof(tilePoolDesc));
	const int tileSize = 128 * 128 * 4; //128*128 texels, 4 bytes per texel.
	const int numTiles = 2; //we want two tiles that we can reuse.
	tilePoolDesc.ByteWidth = tileSize * numTiles;
	tilePoolDesc.Usage = D3D11_USAGE_DEFAULT;
	tilePoolDesc.MiscFlags = D3D11_RESOURCE_MISC_TILE_POOL;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&tilePoolDesc, nullptr, &m_tilePool));

	// Create temporary resource so we can color the tiles.
	D3D11_BUFFER_DESC tempBufferDesc;
	ZeroMemory(&tempBufferDesc, sizeof(tempBufferDesc));
	tempBufferDesc.ByteWidth = tileSize;
	tempBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_TILED;
	tempBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	Microsoft::WRL::ComPtr<ID3D11Buffer> tempBuffer;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&tempBufferDesc, nullptr, &tempBuffer));

	// Map the resource to the first tile.
	D3D11_TILED_RESOURCE_COORDINATE startCoordinate;
	ZeroMemory(&startCoordinate, sizeof(startCoordinate));
	D3D11_TILE_REGION_SIZE regionSize;
	ZeroMemory(&regionSize, sizeof(regionSize));
	regionSize.NumTiles = 1;
	UINT rangeFlags = D3D11_TILE_RANGE_REUSE_SINGLE_TILE;
	UINT tileStartOffset = 0;
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDeviceContext()->UpdateTileMappings(
			m_texture.Get(),
			1,
			&startCoordinate,
			&regionSize,
			m_tilePool.Get(),
			1,
			&rangeFlags,
			&tileStartOffset,
			nullptr,
			0
			)
		);
	// Color the tile black.
	byte defaultTileData[tileSize];
	FillMemory(defaultTileData, tileSize, 0x7F);
	m_deviceResources->GetD3DDeviceContext()->UpdateTiles(m_texture.Get(), &startCoordinate, &regionSize, defaultTileData, 0);

	//Map the resource to the second tile.
	tileStartOffset = 1;
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDeviceContext()->UpdateTileMappings(
		m_texture.Get(),
		1,
		&startCoordinate,
		&regionSize,
		m_tilePool.Get(),
		1,
		&rangeFlags,
		&tileStartOffset,
		nullptr,
		0
		)
		);
	// Color the tile white.
	FillMemory(defaultTileData, tileSize, 0xCF);
	m_deviceResources->GetD3DDeviceContext()->UpdateTiles(m_texture.Get(), &startCoordinate, &regionSize, defaultTileData, 0);

	// Map the texture to the two tiles in a checkerboard pattern.
	for (int y = 0; y < 8; y++) {
		for (int x = 0; x < 8; x++) {
			//if ((x + y) % 4 == 3) continue;
			D3D11_TILED_RESOURCE_COORDINATE startCoordinate;
			ZeroMemory(&startCoordinate, sizeof(startCoordinate));
			startCoordinate.X = x;
			startCoordinate.Y = y;
			D3D11_TILE_REGION_SIZE regionSize;
			ZeroMemory(&regionSize, sizeof(regionSize));
			regionSize.NumTiles = 1;
			UINT rangeFlags = D3D11_TILE_RANGE_REUSE_SINGLE_TILE;
			UINT tileStartOffset = (x + y) % 2;
			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDeviceContext()->UpdateTileMappings(
				m_texture.Get(),
				1,
				&startCoordinate,
				&regionSize,
				m_tilePool.Get(),
				1,
				&rangeFlags,
				&tileStartOffset,
				nullptr,
				0
				)
				);

		}
	}
}*/

void Renderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_finalVS.Reset();
	m_inputLayout.Reset();
	m_finalPS.Reset();
	m_constantBuffer.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
	m_lightCBuffer.Reset();
	m_materials.clear();
}