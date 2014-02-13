// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

#include "../Voxels/include/Grid.h"
#include "../Voxels/include/Polygonizer.h"

#include "MaterialTable.h"
#include "Voxel/VoxelLodOctree.h"

class AllocatorBase;

class Scene
{
public:
	typedef char GridType;
	typedef Voxels::Grid SceneGridType;
	typedef Voxels::Polygonizer VoxelAlgorithm;

	enum SeedSurface
	{
		SSU_Plane,
		SSU_Sphere,
		SSU_Proc,
		SSU_Box
	};

	Scene(const std::string& filename /*leave empty to generate*/
		, unsigned gridSize
		, const std::string& materialTable
		, const std::string& heightmap /*leave empty to generate*/
		, SeedSurface surfaceType
		, float scale
		, const DirectX::XMFLOAT3& gridScale);
	~Scene();

	const MaterialTable& GetMaterials() const;

	const Voxels::PolygonSurface* GetPolygonSurface() const { return m_PolygonSurface; }
	SceneGridType* GetVoxelGrid() const { return m_Grid; }

	bool Intersect(DirectX::FXMVECTOR start,
		DirectX::FXMVECTOR end,
		DirectX::XMVECTOR& intersection) const;

	bool SaveVoxelGrid(const std::string& filename);

	const DirectX::XMFLOAT4X4& GetGridWorldMatrix() const { return m_GridWorld; }

	void RecalculateGrid(const Voxels::float3pair* modified = nullptr);

	Voxels::VoxelLodOctree& GetLodOctree() const { return *m_LodOctree; }

private:
	SceneGridType* m_Grid;
	std::unique_ptr<Voxels::VoxelSurface> m_Surface;
	std::unique_ptr<VoxelAlgorithm> m_Polygonizer;
	Voxels::PolygonSurface* m_PolygonSurface;
	std::unique_ptr<Voxels::VoxelLodOctree> m_LodOctree;
	MaterialTable m_Materials;

	DirectX::XMFLOAT3 m_Scale;

	DirectX::XMFLOAT4X4 m_GridWorld;
};