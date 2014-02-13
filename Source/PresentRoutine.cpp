// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information

#include "stdafx.h"

#include "PresentRoutine.h"

bool PresentRoutine::Render(float deltaTime)
{
	if(FAILED(m_Renderer->GetSwapChain()->Present(1, 0)))
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to present");
		return false;
	}

	return true;
}
