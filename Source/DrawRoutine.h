// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

#include <Dx11/Rendering/DxRenderingRoutine.h>
#include <Dx11/Rendering/VertexTypes.h>
#include <Utilities/Aligned.h>

#include "Voxel/VoxelLodOctree.h"
#include "Scene.h"

class Camera;
class AllocatorBase;

// Draws the polygonized surface. It's responsible for loading the grid 
// to the GPU and rendering the relevant blocks each frame.
class DrawRoutine : public DxRenderingRoutine, public Aligned<16>
{
public:
	DrawRoutine();
	virtual ~DrawRoutine();

	virtual bool Initialize(Renderer* renderer, Camera* camera, const DirectX::XMFLOAT4X4& projection, Scene* scene);

	virtual bool Render(float deltaTime);

	bool ReloadGrid();
	bool UpdateGrid();

	bool GetDrawSolid() const { return m_DrawSolid; }
	void SetDrawSolid(bool draw) { m_DrawSolid = draw; };

	bool GetDrawWireframe() const { return m_DrawWireframe; }
	void SetDrawWireframe(bool draw) { m_DrawWireframe = draw; };

	bool GetDrawTransitions() const { return m_DrawTransitions; }
	void SetDrawTransitions(bool draw) { m_DrawTransitions = draw; };

	bool GetDrawSurface() const { return m_DrawSurface; }
	void SetDrawSurface(bool draw) { m_DrawSurface = draw; };

	unsigned SetCurrentLodToDraw(unsigned id);
	unsigned GetCurrentLodToDraw() const;

	bool GetUseLodOctree() const { return m_UseLodOctree; }
	void SetUseLodOctree(bool use) { m_UseLodOctree = use; m_DrawTransitions = use; };

	bool GetLodUpdateEnabled() const { return m_LodUpdate; }
	void SetLodUpdateEnabled(bool enabled) { m_LodUpdate = enabled; };

private:
	void UpdateCulledObjects();

	Camera* m_Camera;
	DirectX::XMFLOAT4X4 m_Projection;

	Scene* m_Scene;

	bool m_DrawSolid;
	bool m_DrawWireframe;
	bool m_DrawTransitions;
	bool m_DrawSurface;
	bool m_UseLodOctree;
	bool m_LodUpdate;

	Voxels::VoxelLodOctree::VisibleBlocksVec m_BlockToDraw;
	
	ReleaseGuard<ID3D11Buffer> m_PerFrameBuffer;
	ReleaseGuard<ID3D11Buffer> m_PerBlockBuffer;	
	ReleaseGuard<ID3D11Buffer> m_TexturePropsBuffer;

	ReleaseGuard<ID3D11InputLayout> m_SurfaceVertexLayout;

	typedef std::vector<ReleaseGuard<ID3D11Buffer>> BuffersVec;
	
	// An entire LOD level data uploaded on the GPU
	struct LodLevel : boost::noncopyable
	{
		LodLevel() {}
		LodLevel(LodLevel&& rhs)
			: Blocks(std::move(rhs.Blocks))
		{}
		
		LodLevel& operator=(LodLevel&& rhs) {
			if(this != &rhs) {
				std::swap(Blocks, rhs.Blocks);
			}
			return *this;
		}

		// A block uploaded on the GPU
		struct BlockData
		{
			BlockData(){}

			BlockData(BlockData&& rhs)
				: Id(rhs.Id)
				, LODLevel(rhs.LODLevel)
				, VertexBuffer(std::move(rhs.VertexBuffer))
				, IndexBuffer(std::move(rhs.IndexBuffer))
				, IndicesSize(std::move(rhs.IndicesSize))
				, TransitionVertexBuffers(std::move(rhs.TransitionVertexBuffers))
				, TransitionIndexBuffers(std::move(rhs.TransitionIndexBuffers))
				, TransitionIndicesSizes(std::move(rhs.TransitionIndicesSizes))
			{}

			BlockData& operator=(BlockData&& rhs)
			{
				if(this != &rhs) {
					std::swap(Id, rhs.Id);
					std::swap(LODLevel, rhs.LODLevel);
					std::swap(VertexBuffer, rhs.VertexBuffer);
					std::swap(IndexBuffer, rhs.IndexBuffer);
					std::swap(IndicesSize, rhs.IndicesSize);
					std::swap(TransitionVertexBuffers, rhs.TransitionVertexBuffers);
					std::swap(TransitionIndexBuffers, rhs.TransitionIndexBuffers);
					std::swap(TransitionIndicesSizes, rhs.TransitionIndicesSizes);
				}
				return *this;
			}

			unsigned Id;
			unsigned LODLevel;
			ReleaseGuard<ID3D11Buffer> VertexBuffer;
			ReleaseGuard<ID3D11Buffer> IndexBuffer;
			unsigned IndicesSize;

			BuffersVec TransitionVertexBuffers;
			BuffersVec TransitionIndexBuffers;
			std::vector<unsigned> TransitionIndicesSizes;
		};

		typedef std::vector<BlockData> BlocksVec;
		BlocksVec Blocks;
	};
	bool CreateBuffersForBlock(const Voxels::BlockPolygons* inBlock, LodLevel::BlockData& outputBlock);

	typedef std::vector<LodLevel> LodLevels;
	LodLevels m_LodLevels;
	unsigned m_CurrentLodToDraw;

	typedef std::map<unsigned, LodLevel::BlockData*> BlocksMap;
	BlocksMap m_BlocksMap;
	
	ReleaseGuard<ID3D11VertexShader> m_VS;
	ReleaseGuard<ID3D11VertexShader> m_VSTransition;
	ReleaseGuard<ID3D11PixelShader> m_PS;

	ReleaseGuard<ID3D11RasterizerState> m_RasterizerState;
	ReleaseGuard<ID3D11RasterizerState> m_WireframeRasterizerState;

	ReleaseGuard<ID3D11SamplerState> m_SamplerState;
	TexturePtr m_DiffuseTextures;
	TexturePtr m_NormalTextures;
};