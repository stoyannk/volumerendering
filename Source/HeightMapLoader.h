// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

#include "../Voxels/include/Grid.h"

// Loads an image that can later be used as source for a voxel grid
class HeightMapLoader
{
public:
	typedef Voxels::VoxelGrid grid_t;
	static bool Load(const std::string& filename, unsigned& w, std::shared_ptr<char>& heightmap, float heightScale = 1.f);
};
