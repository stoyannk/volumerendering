// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#include "stdafx.h"

#include "VoxelProc.h"

#include <ThirdParty/glm/glm/glm.hpp>
#include <ThirdParty/glm/glm/gtc/noise.hpp>

using namespace DirectX;

namespace Voxels
{

VoxelProc::VoxelProc(const XMFLOAT3& normal, float d, const ProceduralParams& params)
	: m_Plane(XMVectorSet(normal.x, normal.y, normal.z, d))
	, m_Params(params)
{
	m_Plane = XMPlaneNormalize(m_Plane);
}

VoxelProc::~VoxelProc()
{}

void VoxelProc::GetSurface(float xStart, float xEnd, float xStep,
	float yStart, float yEnd, float yStep,
	float zStart, float zEnd, float zStep,
	float* output,
	unsigned char* materialid,
	unsigned char* blend)
{
	auto id = 0;
	/*for (auto z = zStart; z < zEnd; z += zStep)
	{
		for (auto y = yStart; y < yEnd; y += yStep)
		{
			for (auto x = xStart; x < xEnd; x += xStep)
			{
				XMVECTOR point = XMVectorSet(x, y, z, 1);
				auto density = XMVectorGetX(XMPlaneDot(m_Plane, point));
				density += glm::perlin(glm::vec3(
					x * m_Params.DimensionFactor,
					y * m_Params.DimensionFactor,
					z * m_Params.DimensionFactor)) * m_Params.ScaleFactor;

				output[id] = density;
				if (materialid != nullptr)
				{
					materialid[id] = 0;
					blend[id] = 0;
				}
				++id;
			}
		}
	}*/

	/*for (auto z = zStart/2; z < zEnd/2; z += zStep/2)
	{
		for (auto y = yStart/2; y < yEnd/2; y += yStep/2)
		{
			for (auto x = xStart/2; x < xEnd/2; x += xStep/2)
			{
				XMVECTOR point = XMVectorSet(x, y, z, 1);
				auto density = XMVectorGetX(XMPlaneDot(m_Plane, point));
				density += glm::perlin(glm::vec3(
					x * m_Params.DimensionFactor,
					y * m_Params.DimensionFactor,
					z * m_Params.DimensionFactor)) * m_Params.ScaleFactor * 2;

				output[id] = density;
				if (materialid != nullptr)
				{
					materialid[id] = 0;
					blend[id] = 0;
				}
				++id;
			}
		}
	}*/

	static const int DFACT = 2;

	for (auto z = zStart / DFACT; z < zEnd / DFACT; z += zStep / DFACT)
	{
		for (auto y = yStart / DFACT; y < yEnd / DFACT; y += yStep / DFACT)
		{
			for (auto x = xStart / DFACT; x < xEnd / DFACT; x += xStep / DFACT)
			{
				XMVECTOR point = XMVectorSet(x, y, z, 1);
				auto density = XMVectorGetX(XMPlaneDot(m_Plane, point));
				density += glm::perlin(glm::vec3(
					x * m_Params.DimensionFactor,
					y * m_Params.DimensionFactor,
					z * m_Params.DimensionFactor)) * m_Params.ScaleFactor * 4;

				output[id] = density;
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