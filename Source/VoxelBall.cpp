// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#include "stdafx.h"

#include "VoxelBall.h"

using namespace DirectX;

namespace Voxels
{

VoxelBall::VoxelBall(const XMFLOAT3& position, float radius)
	: m_Ball(XMVectorSet(position.x, position.y, position.z, radius))
{}

void VoxelBall::GetSurface(float xStart, float xEnd, float xStep,
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
				const XMVECTOR vec = XMVectorSubtract(m_Ball, XMVectorSet(x, y, z, 0));
				const XMVECTOR dist = XMVector3Length(vec);

				output[id] = XMVectorGetW(XMVectorSubtract(dist, m_Ball));
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

VoxelBall::~VoxelBall()
{}

}