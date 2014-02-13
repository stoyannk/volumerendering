// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#include "stdafx.h"

#include "VoxelPlane.h"

using namespace DirectX;

namespace Voxels
{

VoxelPlane::VoxelPlane(const XMFLOAT3& normal, float d)
	: m_Plane(XMVectorSet(normal.x, normal.y, normal.z, d))
{
	m_Plane = XMPlaneNormalize(m_Plane);
}
	
void VoxelPlane::GetSurface(float xStart, float xEnd, float xStep,
	float yStart, float yEnd, float yStep,
	float zStart, float zEnd, float zStep,
	float* output,
	unsigned char* materialid,
	unsigned char* blend)
{
	auto id = 0;
	for (auto z = zStart; z < zEnd; z += zStep)
	{
		for (auto y = yStart; y < yEnd; y += yStep)
		{
			for (auto x = xStart; x < xEnd; x += xStep)
			{
				const XMVECTOR point = XMVectorSet(x, y, z, 1);
				output[id] = XMVectorGetX(XMPlaneDot(m_Plane, point));
				if (materialid != nullptr)
				{
					materialid[id] = 0;
					blend[id] = 0;
				}
				++id;
			}
		}
	}
}

VoxelPlane::~VoxelPlane()
{}

}