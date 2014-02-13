// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

#include <Dx11\Rendering\DxRenderingRoutine.h>

class ClearRenderingRoutine : public DxRenderingRoutine
{
public:
	virtual bool Render(float deltaTime);
};