// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#include "stdafx.h"

#include "Scene.h"
#include "VoxelPlane.h"
#include "VoxelBall.h"
#include "VoxelProc.h"
#include "VoxelBox.h"

#include <Utilities/SimpleAllocator.h>
#include "HeightMapLoader.h"

#include <DirectXCollision.h>

using namespace DirectX;
							  
Scene::Scene(const std::string& filename /*leave empty to generate*/
		, unsigned gridSize
		, const std::string& materialTable
		, const std::string& heightmap /*leave empty to generate*/
		, SeedSurface surfaceType
		, float scale
		, const XMFLOAT3& gridScale)
	: m_Scale(gridScale)
	, m_Grid(nullptr)
	, m_PolygonSurface(nullptr)
{
	const float start_x = -(gridSize / 8.f);
	const float start_y = -(gridSize / 8.f);
	const float start_z = -(gridSize / 8.f);
	const float step = 0.5f;

	m_Polygonizer.reset(new Voxels::Polygonizer);
	if(filename.empty() && heightmap.empty())
	{
		switch(surfaceType) {
		case SSU_Plane:
			m_Surface.reset(new Voxels::VoxelPlane(XMFLOAT3(0.0f, 0.0f, 1.0f), 5.0f));
			break;
		case SSU_Sphere:
			m_Surface.reset(new Voxels::VoxelBall(XMFLOAT3(5, 5, 5), gridSize / 8.f));
			break;
		case SSU_Proc:
			{
				Voxels::ProceduralParams params;
				params.DimensionFactor = 0.25f;
				params.ScaleFactor = 7.5f;
				m_Surface.reset(new Voxels::VoxelProc(XMFLOAT3(0.0f, 0.0f, 1.0f),
					5.0f,
					params));
			}
			break;
		case SSU_Box:
			m_Surface.reset(new Voxels::VoxelBox(XMFLOAT3(5.0f, 5.0f, 5.0f)));
			break;
		}
		
		m_Grid = SceneGridType::Create(gridSize, gridSize, gridSize, start_x, start_y, start_z, step, m_Surface.get());
	}
	else
	{
		if(!filename.empty()) {
			std::ifstream fin(filename.c_str(), std::ios::binary);
			if (!fin.is_open())
			{
				SLOG(Sev_Error, Fac_Rendering, "Unable to open voxel grid file!");
			}

			fin.seekg(0, std::ios::end);
			unsigned length = (unsigned)fin.tellg();
			fin.seekg(0, std::ios::beg);

			std::unique_ptr<char[]> data(new char[length]);
			fin.read(data.get(), length);

			m_Grid = SceneGridType::Load(data.get(), length);
		} else {
			unsigned int w;
			std::shared_ptr<char> heightValues;
			if (!HeightMapLoader::Load(heightmap, w, heightValues, scale)) {
				SLOG(Sev_Error, Fac_Rendering, "Unable generate grid from hightmap!");
			}
			m_Grid = SceneGridType::Create(w, heightValues.get());
		}
	}

	if (!m_Grid) {
		SLOG(Sev_Error, Fac_Rendering, "Unable to create voxel grid!");
		return;
	}

	if(!m_Materials.Load(materialTable)) {
		SLOG(Sev_Error, Fac_Rendering, "Unable to load the material table!");
	}

	RecalculateGrid();

	XMStoreFloat4x4(&m_GridWorld, XMMatrixScaling(gridScale.x, gridScale.y, gridScale.z));
}

bool Scene::SaveVoxelGrid(const std::string& filename)
{
	std::ofstream fout(filename.c_str(), std::ios::binary);

	if (!fout.is_open())
		return false;

	auto pack = m_Grid->PackForSave();

	fout.write(pack->GetData(), pack->GetSize());

	pack->Destroy();

	return true;
}

void Scene::RecalculateGrid(const Voxels::float3pair* modified)
{
	SLLOG(Sev_Info, Fac_Rendering, "Memory used for grid blocks: ", m_Grid->GetGridBlocksMemorySize());
	
	Voxels::Modification* modification = Voxels::Modification::Create();
	if(modified) {
		modification->Map = m_PolygonSurface;
		modification->MinCornerModified = modified->first;
		modification->MaxCornerModified = modified->second;
	}
	else {
		if (m_PolygonSurface)
		{
			m_PolygonSurface->Destroy();
			m_PolygonSurface = nullptr;
		}
	}

	SLOG(Sev_Debug, Fac_Rendering, "Recalculating grid...");

	m_PolygonSurface = std::move(decltype(m_PolygonSurface)(m_Polygonizer->Execute(*m_Grid, &m_Materials, modified ? modification : nullptr)));
	
	modification->Destroy();

	SLLOG(Sev_Info, Fac_Rendering, "Polygon data size: ", m_PolygonSurface->GetPolygonDataSizeBytes());
	SLLOG(Sev_Info, Fac_Rendering, "Material cache size: ", m_PolygonSurface->GetCacheSizeBytes());

	// Print stat data
	unsigned totalVertices = 0;
	unsigned totalIndices = 0;

	auto stats = m_PolygonSurface->GetStatistics();
	SLOG(Sev_Debug, Fac_Rendering, "TransVoxel results:");
	SLOG(Sev_Debug, Fac_Rendering, "Total blocks recalculated: ", stats->BlocksCalculated);
	for (auto level = 0u; level < m_PolygonSurface->GetLevelsCount(); ++level)
	{
		unsigned blocksCnt = m_PolygonSurface->GetBlocksForLevelCount(level);
		SLOG(Sev_Debug, Fac_Rendering, "Blocks produced ", blocksCnt);
		for (auto blockId = 0u; blockId < blocksCnt; ++blockId)
		{
			unsigned temp = 0;
			auto block = m_PolygonSurface->GetBlockForLevel(level, blockId);
			block->GetVertices(&temp);
			SLOG(Sev_Trace, Fac_Rendering, "Vertices produced ", temp);
			totalVertices += temp;
			block->GetIndices(&temp);
			totalIndices += temp;
			SLOG(Sev_Trace, Fac_Rendering, "Indices produced ", temp);
		}
	}
	SLOG(Sev_Debug, Fac_Rendering, "Total vertices produced: ", totalVertices);
	SLOG(Sev_Debug, Fac_Rendering, "Total indices produced: ", totalIndices);

	SLOG(Sev_Debug, Fac_Rendering, "Trivial cells ", stats->TrivialCells);
	SLOG(Sev_Debug, Fac_Rendering, "NonTrivial cells ", stats->NonTrivialCells);
	for (auto i = 0u; i < Voxels::PolygonizationStatistics::CASES_COUNT; ++i)
	{
		SLOG(Sev_Trace, Fac_Rendering, "Cells with Case[", i, "] ", stats->PerCaseCellsCount[i]);
	}

	// Rebuild the octree
	m_LodOctree.reset(new Voxels::VoxelLodOctree());
	if(!m_LodOctree->Build(*m_PolygonSurface)) {
		SLOG(Sev_Error, Fac_Rendering, "LOD octree building failed!");
	}

	SLLOG(Sev_Debug, Fac_Rendering, "LOD octree levels: ", m_LodOctree->GetLodLevelsCount());
	SLLOG(Sev_Debug, Fac_Rendering, "LOD octree non-empty cells: ", m_LodOctree->GetNonEmptyNodesCount());
}

bool Scene::Intersect(DirectX::FXMVECTOR start,
	DirectX::FXMVECTOR end,
	XMVECTOR& intersection) const
{
	using namespace DirectX;

	const auto recScale = XMVectorReciprocal(XMLoadFloat3(&m_Scale));
	const auto origin = XMVectorMultiply(start, recScale);
	const auto endPoint = XMVectorMultiply(end, recScale);

	unsigned blocksCnt = m_PolygonSurface->GetBlocksForLevelCount(0);
	auto direction = DirectX::XMVector3Normalize(end - start);

	typedef std::pair<float, const Voxels::BlockPolygons*> HitBlock;
	std::vector<HitBlock> hitBlocks;
	for (auto blockId = 0u; blockId < blocksCnt; ++blockId)
	{
		BoundingBox aabb;
		auto block = m_PolygonSurface->GetBlockForLevel(0, blockId);
		const auto minCorner = block->GetMinimalCorner();
		const auto maxCorner = block->GetMaximalCorner();
		aabb.Extents = XMFLOAT3((maxCorner.x - minCorner.x) / 2.f,
								(maxCorner.y - minCorner.y) / 2.f,
								(maxCorner.z - minCorner.z) / 2.f);

		aabb.Center = XMFLOAT3(minCorner.x + aabb.Extents.x,
								minCorner.y + aabb.Extents.y,
								minCorner.z + aabb.Extents.z);

		float distance = 0;
		if (aabb.Intersects(start, direction, distance))
		{
			hitBlocks.push_back(std::make_pair(distance, block));
		}
	}

	std::sort(hitBlocks.begin(), hitBlocks.end(), 
		[](HitBlock& block1, HitBlock& block2) {
		return std::abs(block1.first) < std::abs(block2.first);
	});

	float distance = 0;
	float nearest = std::numeric_limits<float>::max();
	bool found = false;
	XMVECTOR V0, V1, V2;
	for (auto block = hitBlocks.cbegin(); block != hitBlocks.cend(); ++block)
	{
		const auto blockRef = block->second;
		unsigned indicesCnt = 0;
		auto indices = blockRef->GetIndices(&indicesCnt);
		for (auto triangle = 0u; triangle < indicesCnt; triangle += 3)
		{
			auto vertices = blockRef->GetVertices(nullptr);
			V0 = XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&vertices[indices[triangle]].Position));
			V1 = XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&vertices[indices[triangle + 1]].Position));
			V2 = XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&vertices[indices[triangle + 2]].Position));
			if (TriangleTests::Intersects(start, direction, V0, V1, V2, distance))
			{
				nearest = std::min(nearest, distance);
				found = true;
			}
		}

		if (found)
			break;
	}

	intersection = start + nearest * direction;

	return found;
}

const MaterialTable& Scene::GetMaterials() const {
	return m_Materials;
}

void Scene::DestroySurface()
{
	if (m_PolygonSurface)
	{
		m_PolygonSurface->Destroy();
		m_PolygonSurface = nullptr;
	}
}

Scene::~Scene()
{
	if (m_Grid)
	{
		m_Grid->Destroy();
	}
	DestroySurface();
}
