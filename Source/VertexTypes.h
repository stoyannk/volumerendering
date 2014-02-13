// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

struct GridVertex
{
	float Distance;
};

//TODO: Split this in two declarations. One with and one without the secondary vertices
// Use the smaller definition for the blocks that don't have transitions
struct SurfaceVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT4 SecondaryPosition;
	DirectX::XMFLOAT3 Normal;
	unsigned TextureIndices[2];
};

extern D3D11_INPUT_ELEMENT_DESC GridVertexLayout[1];
extern D3D11_INPUT_ELEMENT_DESC SurfaceVertexLayout[4];
