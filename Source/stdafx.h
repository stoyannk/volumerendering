// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

#define CHECK_MEMORY 0

#if CHECK_MEMORY
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <Utilities/CoreIncludes.h>

#include <memory.h>

#include <DirectXMath.h>
#include <D3DX11tex.h>

#include <queue>

#ifdef PROFI_ENABLE
#include <profi_decls.h>
#include <profi.h>
#endif

#include "../Voxels/include/Voxels.h"
