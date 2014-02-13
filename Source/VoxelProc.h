// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

#include <Utilities/Aligned.h>
#include "../Voxels/include/VoxelSurface.h"

namespace Voxels
{

struct ProceduralParams
{
	float DimensionFactor;
	float ScaleFactor;
};

__declspec(align(16))
class VoxelProc : public VoxelSurface, public Aligned<16>
{
public:
	VoxelProc(const DirectX::XMFLOAT3& normal, float d, const ProceduralParams& params);
	virtual ~VoxelProc();

	virtual void GetSurface(float xStart, float xEnd, float xStep,
		float yStart, float yEnd, float yStep,
		float zStart, float zEnd, float zStep,
		float* output,
		unsigned char* materialid,
		unsigned char* blend) override;
private:
	DirectX::XMVECTOR m_Plane;
	ProceduralParams m_Params;
};

}