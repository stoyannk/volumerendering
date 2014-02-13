// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#include "stdafx.h"

#include "VoxelBox.h"

using namespace DirectX;

namespace Voxels
{

VoxelBox::VoxelBox(const XMFLOAT3& extents)
	: m_Extents(extents)
{}

void VoxelBox::GetSurface(float xStart, float xEnd, float xStep,
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
				XMFLOAT3 d(std::abs(x) - m_Extents.x, std::abs(y) - m_Extents.y, std::abs(z) - m_Extents.z);
				XMFLOAT3 dm(std::max(d.x, 0.f), std::max(d.y, 0.f), std::max(d.z, 0.f));
				auto dl = std::sqrtf(dm.x * dm.x + dm.y * dm.y + dm.z * dm.z);

				output[id] = std::min(std::max(d.x, std::max(d.y, d.z)), 0.f) + dl;
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

}