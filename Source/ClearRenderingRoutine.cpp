// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#include "stdafx.h"

#include "ClearRenderingRoutine.h"

bool ClearRenderingRoutine::Render(float deltaTime)
{
	ID3D11DeviceContext* context = m_Renderer->GetImmediateContext();
	
	// Set rt to back buffer
	ID3D11RenderTargetView* bbufferRTV = m_Renderer->GetBackBufferView();
	context->OMSetRenderTargets(1, &bbufferRTV, m_Renderer->GetBackDepthStencilView());

	// Clear the back buffer 
    float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	context->ClearRenderTargetView(m_Renderer->GetBackBufferView(), ClearColor);
	context->ClearDepthStencilView(m_Renderer->GetBackDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	return true;
}