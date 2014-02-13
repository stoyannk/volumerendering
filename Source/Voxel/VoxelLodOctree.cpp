// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#include "stdafx.h"
#include "VoxelLodOctree.h"

using namespace DirectX;

namespace Voxels
{

// if the distance between the camera and the center of the node is
// >= than the coeff than we settle for this block, otherwise we examine it's children
// NB: setting a number < 1.6875 might lead to incorrect results
static const float LOD_SETTLE_COEFF = 2.0f;

inline DirectX::XMVECTOR XMLoadFloat3(const float3* fl)
{
	static_assert(sizeof(float3) == sizeof(DirectX::XMFLOAT3), "Float types incompatible!");
	return XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(fl));
}

inline DirectX::XMVECTOR XMLoadFloat4(const float4* fl)
{
	static_assert(sizeof(float4) == sizeof(DirectX::XMFLOAT4), "Float types incompatible!");
	return XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(fl));
}

inline void XMStoreFloat3(float3* pDestination, DirectX::FXMVECTOR V)
{
	XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(pDestination), V);
}

inline void XMStoreFloat4(float4* pDestination, DirectX::FXMVECTOR V)
{
	XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(pDestination), V);
}

struct VoxelLodOctree::Node
{
	static NodePtr Create(unsigned level, const XMFLOAT3& min, const XMFLOAT3& max)
	{
		return NodePtr(new Node(level, min, max));
	}

	unsigned Id;
	unsigned Level;
	XMFLOAT3 MinCorner;
	XMFLOAT3 MaxCorner;

	unsigned ChildCount;
	NodePtr Children[8];

private:
	Node(unsigned level, const XMFLOAT3& min, const XMFLOAT3& max)
		: Id(PolygonSurface::INVALID_ID)
		, Level(level)
		, MinCorner(min)
		, MaxCorner(max)
		, ChildCount(0)
	{}
};

VoxelLodOctree::VoxelLodOctree()
	: m_LodLevels(0)
	, m_NonEmptyNodes(0)
{}

VoxelLodOctree::~VoxelLodOctree()
{}

bool VoxelLodOctree::PruneNode(NodePtr& node) {
	for(auto i = 0; i < 8 ; ++i) {
		if(node->Children[i]) {
			// this child is empty
			if(!PruneNode(node->Children[i])) {
				node->Children[i].reset();
			} else {
				++node->ChildCount;
			}
		}
	}

	return !!node->ChildCount || node->Id != PolygonSurface::INVALID_ID;
}

bool VoxelLodOctree::Build(const PolygonSurface& map)
{
	auto levelExtents = map.GetExtents();
	m_LodLevels = map.GetLevelsCount();

	m_Root = Node::Create(m_LodLevels - 1,
		XMFLOAT3(0.f, 0.f, 0.f),
		XMFLOAT3(levelExtents.x, levelExtents.y, levelExtents.z));

	NodesQueue currentLevelNodes;
	NodesQueue nextLevelNodes;

	currentLevelNodes.push(&m_Root);
	for(int lodLevel = m_LodLevels - 1; lodLevel >= 0; --lodLevel) {
		const auto blocksCnt = map.GetBlocksForLevelCount(lodLevel);
		while(!currentLevelNodes.empty()) {
			auto& currentNode = *currentLevelNodes.front();

			// look for a polygon block to assign to this node
			const auto currentNodeMin = XMLoadFloat3(&currentNode->MinCorner);
			for (auto bit = 0; bit < blocksCnt; ++bit) {
				auto block = map.GetBlockForLevel(lodLevel, bit);

				const auto minCorner = block->GetMinimalCorner();
				const auto blockMin = XMLoadFloat3(&minCorner);
				UINT areEqual;
				XMVectorEqualIntR(&areEqual, currentNodeMin, blockMin);
				if(XMComparisonAllTrue(areEqual)) {
					currentNode->Id = block->GetId();
					++m_NonEmptyNodes;

					#ifdef _DEBUG
					// assert that the max corners are the same
					const auto currentNodeMax = XMLoadFloat3(&currentNode->MaxCorner);
					const auto maxCorner = block->GetMaximalCorner();
					const auto blockMax = XMLoadFloat3(&maxCorner);
					XMVectorEqualIntR(&areEqual, currentNodeMax, blockMax);
					assert(XMComparisonAllTrue(areEqual));
					#endif
				}
			}

			if(lodLevel > 0) {
				// create all the children
				auto nextLevelNodeExtents = XMFLOAT3(levelExtents.x / 2, levelExtents.y / 2, levelExtents.z / 2);

				auto childId = 0u;

				for(auto x = 0; x < 2; ++x)
				for(auto y = 0; y < 2; ++y)
				for(auto z = 0; z < 2; ++z)
				{
					const auto minCorner = XMFLOAT3(currentNode->MinCorner.x + (x * nextLevelNodeExtents.x)
												  , currentNode->MinCorner.y + (y * nextLevelNodeExtents.y)
												  , currentNode->MinCorner.z + (z * nextLevelNodeExtents.z));

					const auto maxCorner = XMFLOAT3(currentNode->MinCorner.x + ((x + 1) * nextLevelNodeExtents.x)
												  , currentNode->MinCorner.y + ((y + 1) * nextLevelNodeExtents.y)
												  , currentNode->MinCorner.z + ((z + 1) * nextLevelNodeExtents.z));
					
					currentNode->Children[childId] = Node::Create(lodLevel - 1, minCorner, maxCorner);
					
					nextLevelNodes.push(&currentNode->Children[childId]);
					
					++childId;
				}
			}

			currentLevelNodes.pop();
		}
		levelExtents.x /= 2;
		levelExtents.y /= 2;
		levelExtents.z /= 2;

		std::swap(currentLevelNodes, nextLevelNodes);
		assert(nextLevelNodes.empty());
	}

	#ifdef _DEBUG
	// assert that all blocks are in the octree
	auto blockTotalCnt = 0u;
	for(auto lodLevel = 0u; lodLevel < m_LodLevels; ++lodLevel) {
		blockTotalCnt += map.GetBlocksForLevelCount(lodLevel);
	}
	assert(blockTotalCnt == m_NonEmptyNodes);
	#endif

	// Prune all empty nodes
	PruneNode(m_Root);

	return true;
}

VoxelLodOctree::VisibleBlocksVec VoxelLodOctree::Cull(const XMFLOAT4 frustumPlanes[6], const XMFLOAT3& cameraPosition) {
	VisibleBlocksVec output;

	typedef std::vector<NodePtrVec> LodNodesVec;
	LodNodesVec nodesToDraw;
	nodesToDraw.resize(m_LodLevels);

	NodesQueue unvisitedNodes;
	unvisitedNodes.push(&m_Root);

	XMVECTOR camPos = XMLoadFloat3(&cameraPosition);

	while(!unvisitedNodes.empty()) {
		auto& node = *unvisitedNodes.front();

		if(IsCubeVisible(frustumPlanes, node->MinCorner, node->MaxCorner)) {
			bool hasToPushChildren = true;
			if(node->Id != PolygonSurface::INVALID_ID) {
				if(node->ChildCount == 0) {
					// push this because it has no children anyway - a leaf
					nodesToDraw[node->Level].push_back(&node);
					hasToPushChildren = false;
				} else {
					// check the distance
					XMVECTOR minCorner = XMLoadFloat3(&node->MinCorner);
					XMVECTOR maxCorner = XMLoadFloat3(&node->MaxCorner);

					XMVECTOR nodeCenter = (maxCorner + minCorner) / 2;
					XMFLOAT3A distanceVec;
					XMStoreFloat3A(&distanceVec, XMVectorAbs(nodeCenter - camPos));
					auto chebishevDistance = std::max(std::max(distanceVec.x, distanceVec.y), distanceVec.z);

					XMVECTOR nodeExtent = maxCorner - minCorner; // it's a cube - all axes should have the same extent

					if (chebishevDistance >= XMVectorGetX(nodeExtent) * LOD_SETTLE_COEFF) {
						// settle for this node	- it is far away
						nodesToDraw[node->Level].push_back(&node);
						hasToPushChildren = false;
					}
				}
			}

			if(hasToPushChildren) {
				for(auto child = 0u; child < 8; ++child) {
					auto& theChild = (*node).Children[child];
					if(theChild) {
						unvisitedNodes.push(&theChild);
					}
				}
			}
		}

		unvisitedNodes.pop();
	}
	
	for(int level = m_LodLevels - 1; level >= 0; --level) {
		std::for_each(nodesToDraw[level].cbegin(), nodesToDraw[level].cend(), [&](NodePtr* node_p) {
			auto& node = (*node_p);
			output.emplace_back(VisibleBlock(node->Id));

			// we skip the transition face calculation for the nowest level nodes - they never draw transitions
			if(level == 0) return;

			// check the higher-res nodes for potential neighbours
			std::for_each(nodesToDraw[level - 1].cbegin(), nodesToDraw[level - 1].cend(), [&](NodePtr* highnode_p) {
				CheckForNeighbour(node, *highnode_p, output.back());
			});
		});
	}
	
	return std::move(output);
}

void VoxelLodOctree::CheckForNeighbour(const NodePtr& lowResNode, const NodePtr& highResNode, VisibleBlock& output)
{
#ifdef _DEBUG
	const XMFLOAT3& lMin = lowResNode->MinCorner;
	const XMFLOAT3& lMax = lowResNode->MaxCorner;

	const XMFLOAT3& hMin = highResNode->MinCorner;
	const XMFLOAT3& hMax = highResNode->MaxCorner;
	// check that no two nodes overlap!
	if(((lMin.x < hMax.x) && (hMin.x < lMax.x))
	&& ((lMin.y < hMax.y) && (hMin.y < lMax.y))
	&& ((lMin.z < hMax.z) && (hMin.z < lMax.z))) {
		assert(false && "Detected overlapping blocks for drawing!");
	}
#endif

	XMVECTOR lowMin = XMLoadFloat3(&lowResNode->MinCorner);
	XMVECTOR lowMax = XMLoadFloat3(&lowResNode->MaxCorner);

	XMVECTOR highMin = XMLoadFloat3(&highResNode->MinCorner);
	XMVECTOR highMax = XMLoadFloat3(&highResNode->MaxCorner);

	// check if it's a direct neighbour
	auto XPosPlane = XMPlaneFromPointNormal(lowMax, XMVectorSet(1, 0, 0, 1));
	if(std::abs(XMVectorGetX(XMPlaneDotCoord(XPosPlane, highMin))) < std::numeric_limits<float>::epsilon()
		&& floor(lowResNode->MaxCorner.y) > floor(highResNode->MinCorner.y)
		&& floor(lowResNode->MaxCorner.z) > floor(highResNode->MinCorner.z)
		&& floor(lowResNode->MinCorner.y) <= floor(highResNode->MinCorner.y)
		&& floor(lowResNode->MinCorner.z) <= floor(highResNode->MinCorner.z)) {
		output.TransitionFaces[BlockPolygons::XPos] = true;
			return;
	}
	auto XNegPlane = XMPlaneFromPointNormal(lowMin, XMVectorSet(-1, 0, 0, 1));
	if(std::abs(XMVectorGetX(XMPlaneDotCoord(XNegPlane, highMax))) < std::numeric_limits<float>::epsilon()
		&& floor(lowResNode->MinCorner.y) < floor(highResNode->MaxCorner.y)
		&& floor(lowResNode->MinCorner.z) < floor(highResNode->MaxCorner.z)
		&& floor(lowResNode->MaxCorner.y) >= floor(highResNode->MaxCorner.y)
		&& floor(lowResNode->MaxCorner.z) >= floor(highResNode->MaxCorner.z)) {
		output.TransitionFaces[BlockPolygons::XNeg] = true;
			return;
	}

	auto YPosPlane = XMPlaneFromPointNormal(lowMax, XMVectorSet(0, 1, 0, 1));
	if(std::abs(XMVectorGetX(XMPlaneDotCoord(YPosPlane, highMin))) < std::numeric_limits<float>::epsilon()
		&& floor(lowResNode->MaxCorner.x) > floor(highResNode->MinCorner.x)
		&& floor(lowResNode->MaxCorner.z) > floor(highResNode->MinCorner.z)
		&& floor(lowResNode->MinCorner.x) <= floor(highResNode->MinCorner.x)
		&& floor(lowResNode->MinCorner.z) <= floor(highResNode->MinCorner.z)) {
		output.TransitionFaces[BlockPolygons::YPos] = true;
			return;
	}
	auto YNegPlane = XMPlaneFromPointNormal(lowMin, XMVectorSet(0, -1, 0, 1));
	if(std::abs(XMVectorGetX(XMPlaneDotCoord(YNegPlane, highMax))) < std::numeric_limits<float>::epsilon()
		&& floor(lowResNode->MinCorner.x) < floor(highResNode->MaxCorner.x)
		&& floor(lowResNode->MinCorner.z) < floor(highResNode->MaxCorner.z)
		&& floor(lowResNode->MaxCorner.x) >= floor(highResNode->MaxCorner.x)
		&& floor(lowResNode->MaxCorner.z) >= floor(highResNode->MaxCorner.z)) {
		output.TransitionFaces[BlockPolygons::YNeg] = true;
			return;
	}

	auto ZPosPlane = XMPlaneFromPointNormal(lowMax, XMVectorSet(0, 0, 1, 1));
	if(std::abs(XMVectorGetX(XMPlaneDotCoord(ZPosPlane, highMin))) < std::numeric_limits<float>::epsilon()
		&& floor(lowResNode->MaxCorner.x) > floor(highResNode->MinCorner.x)
		&& floor(lowResNode->MaxCorner.y) > floor(highResNode->MinCorner.y)
		&& floor(lowResNode->MinCorner.x) <= floor(highResNode->MinCorner.x)
		&& floor(lowResNode->MinCorner.y) <= floor(highResNode->MinCorner.y)) {
		output.TransitionFaces[BlockPolygons::ZPos] = true;
			return;
	}

	auto ZNegPlane = XMPlaneFromPointNormal(lowMin, XMVectorSet(0, 0, -1, 1));
	if(std::abs(XMVectorGetX(XMPlaneDotCoord(ZNegPlane, highMax))) < std::numeric_limits<float>::epsilon()
		&& floor(lowResNode->MinCorner.x) < floor(highResNode->MaxCorner.x)
		&& floor(lowResNode->MinCorner.y) < floor(highResNode->MaxCorner.y)
		&& floor(lowResNode->MaxCorner.x) >= floor(highResNode->MaxCorner.x)
		&& floor(lowResNode->MaxCorner.y) >= floor(highResNode->MaxCorner.y)) {
		output.TransitionFaces[BlockPolygons::ZNeg] = true;
			return;
	}
}
 
bool VoxelLodOctree::IsCubeVisible(const XMFLOAT4 frustumPlanes[6], const XMFLOAT3& cubeMin, const XMFLOAT3& cubeMax) {
	XMFLOAT4 minExtreme;

	for(unsigned i = 0; i < 6; i++)
	{
		if (frustumPlanes[i].x <= 0)
		{
			minExtreme.x = cubeMin.x;
		}
		else
		{
			minExtreme.x = cubeMax.x;
		}

		if (frustumPlanes[i].y <= 0)
		{
			minExtreme.y = cubeMin.y;
		}
		else
		{
			minExtreme.y = cubeMax.y;
		}

		if (frustumPlanes[i].z <= 0)
		{
			minExtreme.z = cubeMin.z;
		}
		else
		{
			minExtreme.z = cubeMax.z;
		}

		if ((frustumPlanes[i].x * minExtreme.x + frustumPlanes[i].y * minExtreme.y + frustumPlanes[i].z * minExtreme.z + frustumPlanes[i].w) < 0.f)
		{
			return false;
		}
	}

	return true;
}

}
