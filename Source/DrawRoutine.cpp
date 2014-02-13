// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#include "stdafx.h"

#include "DrawRoutine.h"
#include "ConstBufferTypes.h"
#include "VertexTypes.h"
#include "Scene.h"

#include <Dx11/Rendering/Camera.h>
#include <Dx11/Rendering/ShaderManager.h>
#include <Dx11/Rendering/FrustumCuller.h>

using namespace DirectX;

DrawRoutine::DrawRoutine()
	: m_DrawSolid(true)
	, m_DrawSurface(true)
	, m_DrawWireframe(false)
	, m_DrawTransitions(true)
	, m_CurrentLodToDraw(0)
	, m_UseLodOctree(true)
	, m_LodUpdate(true)
{}

DrawRoutine::~DrawRoutine()
{}

bool DrawRoutine::Initialize(Renderer* renderer, Camera* camera, const XMFLOAT4X4& projection, Scene* scene)
{
	DxRenderingRoutine::Initialize(renderer);

	m_Camera = camera;
	m_Projection = projection;
	m_Scene = scene;

	ShaderManager shaderManager(m_Renderer->GetDevice());
	ShaderManager::CompilationOutput compilationResult;
	// Create shaders
	if(!shaderManager.CompileShaderDuo("..\\Shaders\\DrawSurface.fx"
									, "VS"
									, "vs_4_0"
									, "PS"
									, "ps_4_0"
									, compilationResult))
	{
		compilationResult.ReleaseAll();
		return false;
	}

	ReleaseGuard<ID3DBlob> vsGuard(compilationResult.vsBlob);
	ReleaseGuard<ID3DBlob> psGuard(compilationResult.psBlob);
	m_VS.Set(compilationResult.vertexShader);
	m_PS.Set(compilationResult.pixelShader);
	
	m_VSTransition.Set(shaderManager.CompileVertexShader("..\\Shaders\\DrawSurface.fx", "VSTransition", "vs_4_0"));
	if(!m_VSTransition.Get()) {
		SLOG(Sev_Error, Fac_Rendering, "Unable to compile transition VS");
		return false;
	}
	
	// Create per frame buffer
	if(!shaderManager.CreateEasyConstantBuffer<PerFrameBuffer>(m_PerFrameBuffer.Receive()))
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to create per frame buffer");
		return false;
	}

	// Create per-subset properties buffer
	if(!shaderManager.CreateEasyConstantBuffer<PerBlockBuffer>(m_PerBlockBuffer.Receive()))
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to create per-block buffer");
		return false;
	}

	// Create texture props buffer
	if(!shaderManager.CreateEasyConstantBuffer<TexturePropertiesBuffer>(m_TexturePropsBuffer.Receive()))
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to create texture properties buffer");
		return false;
	}
	
	UINT numElements = ARRAYSIZE(SurfaceVertexLayout);
	HRESULT hr = S_OK;
    // Create the input layout
	hr = m_Renderer->GetDevice()->CreateInputLayout(SurfaceVertexLayout, numElements, vsGuard.Get()->GetBufferPointer(),
													vsGuard.Get()->GetBufferSize(), m_SurfaceVertexLayout.Receive());
	if(FAILED(hr))
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to create input layout");
		return false;
	}
	
	CD3D11_RASTERIZER_DESC rsDesc((CD3D11_DEFAULT()));
	rsDesc.FrontCounterClockwise = TRUE;
	//rsDesc.CullMode = D3D11_CULL_NONE;
	hr = m_Renderer->GetDevice()->CreateRasterizerState(&rsDesc, m_RasterizerState.Receive());
	if(FAILED(hr))
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to create rasterizer state for surface");
		return false;
	}
	rsDesc.FillMode = D3D11_FILL_WIREFRAME;
	hr = m_Renderer->GetDevice()->CreateRasterizerState(&rsDesc, m_WireframeRasterizerState.Receive());
	if(FAILED(hr))
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to create wireframe rasterizer state for surface");
		return false;
	}

	auto texManager = m_Renderer->GetTextureManager();
	
	// Create sampler
	m_SamplerState.Set(texManager.MakeSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP));

	// Load textures
	const auto& diffuseFiles = m_Scene->GetMaterials().GetDiffuseTextureList();
	m_DiffuseTextures = texManager.LoadTexture2DArray(diffuseFiles, "../media/textures", true);
	if(!m_DiffuseTextures)
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to load diffuse terrain textures");
		return false;
	}

	// load normal maps
	MaterialTable::TextureNames normalFiles;
	normalFiles.reserve(diffuseFiles.size());
	std::transform(diffuseFiles.cbegin(), diffuseFiles.cend(), std::back_inserter(normalFiles), [](const std::string& filename) {
		std::string name(filename.begin(), filename.begin() + filename.find_first_of('.'));
		return name + "_normal.dds";
	});

	m_NormalTextures = texManager.LoadTexture2DArray(normalFiles, "../media/textures", false);
	if (!m_NormalTextures)
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to load normal terrain textures");
		return false;
	}

	if(!ReloadGrid())
		return false;

	return true;
}

bool DrawRoutine::CreateBuffersForBlock(const Voxels::BlockPolygons* inBlock, LodLevel::BlockData& outputBlock) {
	unsigned indicesCnt = 0;
	auto indices = inBlock->GetIndices(&indicesCnt);
	if (!indicesCnt) {
		return true;
	}
	unsigned verticesCnt = 0;
	auto vertices = inBlock->GetVertices(&verticesCnt);

	D3D11_BUFFER_DESC vbDesc;
	::ZeroMemory(&vbDesc, sizeof(vbDesc));
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.ByteWidth = sizeof(SurfaceVertex)* verticesCnt;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	::ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	
	HRESULT hr = m_Renderer->GetDevice()->CreateBuffer(&vbDesc, &InitData, outputBlock.VertexBuffer.Receive());
	if(FAILED(hr))
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to create vertex buffer for surface");
		return false;
	}
	
	D3D11_BUFFER_DESC ibDesc;
	::ZeroMemory(&ibDesc, sizeof(ibDesc));
	ibDesc.Usage = D3D11_USAGE_DEFAULT;
	ibDesc.ByteWidth = sizeof(int)* indicesCnt;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	::ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = indices;
	
	hr = m_Renderer->GetDevice()->CreateBuffer(&ibDesc, &InitData, outputBlock.IndexBuffer.Receive());
	if(FAILED(hr))
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to create index buffer for surface");
		return false;
	}
	outputBlock.IndicesSize = indicesCnt;
	
	// Create the transition buffers
	for (auto tr = 0u; tr < Voxels::BlockPolygons::Face_Count; ++tr) {
		unsigned transitionVerticesCnt = 0;
		auto transitionInVertices = inBlock->GetTransitionVertices(
			Voxels::BlockPolygons::TransitionFaceId(tr), &transitionVerticesCnt);
		unsigned transitionIndicesCnt = 0;
		auto transitionInIndices  = inBlock->GetTransitionIndices(
			Voxels::BlockPolygons::TransitionFaceId(tr), &transitionIndicesCnt);
	
		outputBlock.TransitionIndicesSizes.push_back(transitionIndicesCnt);
		outputBlock.TransitionVertexBuffers.push_back(ReleaseGuard<ID3D11Buffer>());
		outputBlock.TransitionIndexBuffers.push_back(ReleaseGuard<ID3D11Buffer>());
	
		if (!transitionIndicesCnt) {
			continue;
		}
	
		vbDesc.ByteWidth = sizeof(SurfaceVertex)* transitionVerticesCnt;
		InitData.pSysMem = transitionInVertices;
		hr = m_Renderer->GetDevice()->CreateBuffer(&vbDesc, &InitData, outputBlock.TransitionVertexBuffers[tr].Receive());
		if(FAILED(hr))
		{
			SLOG(Sev_Error, Fac_Rendering, "Unable to create transition vertex buffer for block!");
			return false;
		}
	
		ibDesc.ByteWidth = sizeof(int) * transitionIndicesCnt;
		InitData.pSysMem = transitionInIndices;
		hr = m_Renderer->GetDevice()->CreateBuffer(&ibDesc, &InitData, outputBlock.TransitionIndexBuffers[tr].Receive());
		if(FAILED(hr))
		{
			SLOG(Sev_Error, Fac_Rendering, "Unable to create transition index buffer for block!");
			return false;
		}
	}

	return true;
}

bool DrawRoutine::ReloadGrid()
{
	SLOG(Sev_Debug, Fac_Rendering, "Uploading grid polygons to GPU...");

	const auto surface = m_Scene->GetPolygonSurface();

	const auto levelsCount = surface->GetLevelsCount();

	m_LodLevels.clear();
	m_BlocksMap.clear();
	m_LodLevels.resize(levelsCount);

	for(auto lodLevel = 0u; lodLevel < levelsCount; ++lodLevel) {
		auto blocksCount = surface->GetBlocksForLevelCount(lodLevel);
		assert(blocksCount);
	
		auto& currentLodLevel = m_LodLevels[lodLevel];
		currentLodLevel.Blocks.resize(blocksCount);
																   
		for(auto blockId = 0u; blockId < blocksCount; ++blockId) {
			auto& currentOutputBlock = currentLodLevel.Blocks[blockId];
			auto inBlock = surface->GetBlockForLevel(lodLevel, blockId);

			const auto inputBlockId = inBlock->GetId();
			currentOutputBlock.Id = inputBlockId;
			currentOutputBlock.LODLevel = lodLevel;
			m_BlocksMap.insert(std::make_pair(inputBlockId, &currentOutputBlock));

			if(!CreateBuffersForBlock(inBlock, currentOutputBlock))
				return false;
		}
	}

	SLOG(Sev_Debug, Fac_Rendering, "Successfully uploaded grid polygons to GPU");

	UpdateCulledObjects();

	// Update the texture properties
	ID3D11DeviceContext* context = m_Renderer->GetImmediateContext();
	const auto& props = m_Scene->GetMaterials().GetDiffuseTextureProperties();
	TexturePropertiesBuffer tpb;
	::memcpy(&tpb, &props[0], sizeof(MaterialTable::TextureProperties) * props.size());
	context->UpdateSubresource(m_TexturePropsBuffer.Get(), 0, nullptr, &tpb, 0, 0);

	return true;
}

bool DrawRoutine::UpdateGrid() {
	SLOG(Sev_Debug, Fac_Rendering, "Uploading modified grid polygons to GPU..");

	m_BlocksMap.clear();

	const auto surface = m_Scene->GetPolygonSurface();
	const auto levelsCount = surface->GetLevelsCount();
	for(auto lodLevel = 0u; lodLevel < levelsCount; ++lodLevel) {
		auto blocksCnt = surface->GetBlocksForLevelCount(lodLevel);
		assert(blocksCnt);

		//TODO: the whole algo is somewhat unoptimal due to the many traversals of the
		// structs. However the element count is relatively low so optimizing it is not 
		// a priority for now

		auto& currentBlocksOut = m_LodLevels[lodLevel].Blocks;
		// find all blocks that have been modified and need deletion
		auto last = std::remove_if(currentBlocksOut.begin(),
				currentBlocksOut.end(), [&](LodLevel::BlockData& outBlock) {
			for (auto bl = 0; bl < blocksCnt; ++bl)
			{
				if (outBlock.Id == surface->GetBlockForLevel(lodLevel, bl)->GetId())
				{
					return true;
				}
			}
			return false;
		});

		// delete the old blocks
		currentBlocksOut.erase(last, currentBlocksOut.end());
		currentBlocksOut.reserve(blocksCnt);

		// create the buffers for all new blocks
		for (auto bl = 0; bl < blocksCnt; ++bl) {
			auto inBlock = surface->GetBlockForLevel(lodLevel, bl);
			auto found = std::find_if(currentBlocksOut.begin(), currentBlocksOut.end(), [&](LodLevel::BlockData& outBlock) {
				return outBlock.Id == inBlock->GetId();
			});

			if(found != currentBlocksOut.cend())
				continue;

			currentBlocksOut.push_back(LodLevel::BlockData());
			auto& outBlock = currentBlocksOut.back();
			outBlock.Id = inBlock->GetId();
			outBlock.LODLevel = lodLevel;
			if(!CreateBuffersForBlock(inBlock, outBlock))
				return false;
		}

		std::for_each(currentBlocksOut.begin(), currentBlocksOut.end(), [&](LodLevel::BlockData& outBlock) {
			m_BlocksMap.insert(std::make_pair(outBlock.Id, &outBlock));
		});
	}
	
	SLOG(Sev_Debug, Fac_Rendering, "Successfully uploaded modified grid polygons to GPU");

	// Update the culled objects here - otherwise if culling updates are disabled we might
	// remain with invalid block IDs in the block to draw collection
	UpdateCulledObjects();

	return true;
}

void DrawRoutine::UpdateCulledObjects() {
	// The drawn surface might have some transformation (in the world matrix).
	// The Cull & LOD class expects works with the un-transformed grid so we have
	// to transform our Camera and Frustum planes in the objects space of the 
	// gid.

	auto camPos = m_Camera->GetPos();
	XMFLOAT4 frustumPlanes[6];
	FrustumCuller::CalculateFrustumPlanes(m_Camera->GetViewMatrix(), m_Projection, frustumPlanes);
	
	const auto worldMat = XMLoadFloat4x4(&m_Scene->GetGridWorldMatrix());
	XMVECTOR det;
	const auto invWorld = XMMatrixInverse(&det, worldMat);
	// To transform the planes we need to transform by the inverse-transpose of the matrix
	// We want to go from world to grid space so we want to transform with the inv(world) matrix
	// So we have transp(inv(inv(world))) which is just transp(world)
	const auto transpWorld = XMMatrixTranspose(worldMat);

	// Convert to grid space
	for (int i = 0; i < 6; ++i)
	{
		XMVECTOR plane = XMLoadFloat4(&frustumPlanes[i]);
		plane = XMPlaneNormalize(XMPlaneTransform(plane, transpWorld));
		XMStoreFloat4(&frustumPlanes[i], plane);
	}
	XMVECTOR camVec = XMLoadFloat3(&camPos);
	camVec = XMVector3Transform(camVec, invWorld);
	XMStoreFloat3(&camPos, camVec);
	
	m_BlockToDraw = m_Scene->GetLodOctree().Cull(frustumPlanes, camPos);
}

bool DrawRoutine::Render(float deltaTime)
{
	ID3D11DeviceContext* context = m_Renderer->GetImmediateContext();

	ReleaseGuard<ID3D11RasterizerState> oldRsState;
	context->RSGetState(oldRsState.Receive());
	context->RSSetState(m_RasterizerState.Get());

	ReleaseGuard<ID3D11SamplerState> oldSamplerState;
	context->PSGetSamplers(0, 1, oldSamplerState.Receive());
	context->PSSetSamplers(0, 1, m_SamplerState.GetConstPP());

	context->PSSetShader(m_PS.Get(), nullptr, 0);
	context->IASetInputLayout(m_SurfaceVertexLayout.Get());
	UINT strides[] = { sizeof(Voxels::PolygonVertex) };
	UINT offsets[] = { 0 };
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	PerFrameBuffer pfb;
	pfb.Projection = XMMatrixTranspose(XMLoadFloat4x4(&m_Projection));
	pfb.View = XMMatrixTranspose(XMLoadFloat4x4(&m_Camera->GetViewMatrix()));
	context->UpdateSubresource(m_PerFrameBuffer.Get(), 0, nullptr, &pfb, 0, 0);

	PerBlockBuffer psb;
	XMMATRIX world = XMLoadFloat4x4(&m_Scene->GetGridWorldMatrix());
	psb.World = XMMatrixTranspose(world);
	XMVECTOR determinant;
	auto invWrold = XMMatrixInverse(&determinant, world);
	psb.InvTranspWorld = invWrold;

   	// Set shaders
	context->VSSetConstantBuffers(0, 1, m_PerFrameBuffer.GetConstPP());
	context->VSSetConstantBuffers(1, 1, m_PerBlockBuffer.GetConstPP());
	context->PSSetConstantBuffers(2, 1, m_TexturePropsBuffer.GetConstPP());

	// Set textures
	ID3D11ShaderResourceView* const psTextures[] = {m_DiffuseTextures->GetSHRV(), m_NormalTextures->GetSHRV()};
	context->PSSetShaderResources(0, 2, psTextures);

	if(m_UseLodOctree) {
		if(m_LodUpdate) {
			UpdateCulledObjects();
		}
	} else {
		m_BlockToDraw.clear();
		auto& currentLodLevel = m_LodLevels[m_CurrentLodToDraw];
		std::for_each(currentLodLevel.Blocks.begin(), currentLodLevel.Blocks.end(), 
			[&](LodLevel::BlockData& block) {
				auto outputBlock = Voxels::VoxelLodOctree::VisibleBlock(block.Id);
				std::fill(outputBlock.TransitionFaces, outputBlock.TransitionFaces + 6, true);
				m_BlockToDraw.push_back(outputBlock);
		});
	}

	const auto blocksCount = m_BlockToDraw.size();
	for(auto id = 0u; id < blocksCount; ++id) {
		auto blockIt = m_BlocksMap.find(m_BlockToDraw[id].Id);
		assert(blockIt != m_BlocksMap.end());
		auto& currentBlock = blockIt->second;

		if(!currentBlock->IndexBuffer.Get())
			continue;

		int transitionFlags = 0;
		for(auto tr = 0u; tr < 6; ++tr) {
			if(!currentBlock->TransitionIndicesSizes[tr] || !m_BlockToDraw[id].TransitionFaces[tr])
				continue;
			transitionFlags |= (1 << tr);
		}
		psb.Properties = XMVectorSetInt(transitionFlags, currentBlock->LODLevel, 0, 0);
		context->UpdateSubresource(m_PerBlockBuffer.Get(), 0, nullptr, &psb, 0, 0);

		if(m_DrawSurface) {
			context->VSSetShader(m_VS.Get(), nullptr, 0); 

			context->IASetIndexBuffer(currentBlock->IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
			context->IASetVertexBuffers(0, 1, currentBlock->VertexBuffer.GetConstPP(), strides, offsets);
	
			// Draw
			if(m_DrawSolid)
			{
				context->DrawIndexed(currentBlock->IndicesSize, 0, 0);
			}
	
			if(m_DrawWireframe)
			{
				context->RSSetState(m_WireframeRasterizerState.Get());
				context->DrawIndexed(currentBlock->IndicesSize, 0, 0);
				context->RSSetState(oldRsState.Get());
			}
		} 
		if(m_DrawTransitions) {
			context->VSSetShader(m_VSTransition.Get(), nullptr, 0); 
			for(auto tr = 0u; tr < 6; ++tr) {
				if(!currentBlock->TransitionIndicesSizes[tr] || !m_BlockToDraw[id].TransitionFaces[tr])
					continue;
	
				context->IASetIndexBuffer(currentBlock->TransitionIndexBuffers[tr].Get(), DXGI_FORMAT_R32_UINT, 0);
				context->IASetVertexBuffers(0, 1, currentBlock->TransitionVertexBuffers[tr].GetConstPP(), strides, offsets);
				if(m_DrawSolid)
				{
					context->DrawIndexed(currentBlock->TransitionIndicesSizes[tr], 0, 0);
				}
				if(m_DrawWireframe)
				{
					context->RSSetState(m_WireframeRasterizerState.Get());
					context->DrawIndexed(currentBlock->TransitionIndicesSizes[tr], 0, 0);
					context->RSSetState(oldRsState.Get());
				}
			}
		}
	}
	
	context->PSSetSamplers(0, 1, oldSamplerState.GetConstPP());

	return true;
}

unsigned DrawRoutine::SetCurrentLodToDraw(unsigned id)
{
	m_CurrentLodToDraw = std::min(std::max(0u, id), m_Scene->GetPolygonSurface()->GetLevelsCount() - 1);
	return m_CurrentLodToDraw;
}

unsigned DrawRoutine::GetCurrentLodToDraw() const
{
	return m_CurrentLodToDraw;
}
