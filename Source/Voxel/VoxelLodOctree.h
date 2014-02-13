// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

#include "../../Voxels/include/Polygonizer.h"

class AllocatorBase;

namespace Voxels
{

// Culls surface blocks and decides wich LOD levels to use based on
// the position of the camera each frame
class VoxelLodOctree
{
public:
	struct VisibleBlock {
		VisibleBlock(unsigned id) 
			: Id(id)
		{
			std::fill(TransitionFaces, TransitionFaces + BlockPolygons::Face_Count, false);
		}
		unsigned Id;

		bool TransitionFaces[BlockPolygons::Face_Count];
	};
	typedef std::vector<VisibleBlock> VisibleBlocksVec;

	VoxelLodOctree();
	~VoxelLodOctree();

	bool Build(const PolygonSurface& map);

	// Culls blocks and decides which LOD levels to use
	// NB: Planes and camera position MUST be in un-transformed grid coordinates
	VisibleBlocksVec Cull(const DirectX::XMFLOAT4 frustumPlanes[6], const DirectX::XMFLOAT3& cameraPosition);

	unsigned GetLodLevelsCount() const { return m_LodLevels; }
	unsigned GetNonEmptyNodesCount() const { return m_NonEmptyNodes; }

private:
	struct Node;
	typedef std::unique_ptr<Node> NodePtr;
	typedef std::vector<NodePtr*> NodePtrVec;

	bool PruneNode(NodePtr& node);
	void CheckForNeighbour(const NodePtr& lowResNode, const NodePtr& highResNode, VisibleBlock& output);
	static bool IsCubeVisible(const DirectX::XMFLOAT4 frustumPlanes[6], const DirectX::XMFLOAT3& cubeMin, const DirectX::XMFLOAT3& cubeMax);

	std::shared_ptr<AllocatorBase> m_Allocator;

	NodePtr m_Root;

	typedef std::deque<NodePtr*> qcontainer;
	typedef std::queue<NodePtr*, qcontainer> NodesQueue;

	// statistical data
	unsigned m_LodLevels;
	unsigned m_NonEmptyNodes;
};

}