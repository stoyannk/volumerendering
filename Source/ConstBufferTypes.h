// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

struct PerFrameBuffer
{
	DirectX::XMMATRIX View;
	DirectX::XMMATRIX Projection;
};

struct PerSubsetBuffer
{
	DirectX::XMMATRIX World;
	DirectX::XMVECTOR MaterialParams;
};

struct PerBlockBuffer
{
	DirectX::XMMATRIX World;
	DirectX::XMMATRIX InvTranspWorld;
	DirectX::XMVECTOR Properties; // x - adjacency flags, y - block level(debug)
};

struct TexturePropertiesBuffer
{
	float UVScale[256];
};

struct GridProperties
{
	DirectX::XMVECTOR Properties;
};
