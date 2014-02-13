// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

#include <Dx11/AppGraphics/DxGraphicsApplication.h>
#include "../Voxels/include/Grid.h"

#ifdef _DEBUG
	#define COUNT_MEMORY 1
#else
	#define COUNT_MEMORY 0
#endif

class Scene;
class ClearRenderingRoutine;
class PresentRoutine;
class DrawRoutine;

class VolumeRenderingApplication : public DxGraphicsApplication
{
public:

	VolumeRenderingApplication (HINSTANCE instance);
	virtual ~VolumeRenderingApplication();
	bool Initiate(char* className, char* windowName, unsigned width, unsigned height, WNDPROC winProc, bool sRGBRT = true);
	virtual void Update(float delta) override;
	virtual void KeyDown(unsigned int key) override;
	virtual void MouseButtonDown(MouseBtn button, int x, int y) override;
	virtual void MouseButtonUp(MouseBtn button, int x, int y) override;
	virtual void MouseMove(int x, int y) override;

	void ModifyGrid(int mouseX, int mouseY);

private:
	void RecalculateGrid(const Voxels::float3pair* modified = nullptr);
	
	std::unique_ptr<Scene> m_Scene;

	std::unique_ptr<ClearRenderingRoutine> m_ClearRoutine;
	std::unique_ptr<PresentRoutine> m_PresentRoutine;
	std::unique_ptr<DrawRoutine> m_DrawRoutine;

	DirectX::XMFLOAT3 m_GridScale;

	enum ModificationType
	{
		MT_Inject = 0,
		MT_ChangeMaterial,

		MT_Count
	};
	ModificationType m_Modification;
	Voxels::InjectionType m_InjectionType;
	int m_MaterialId;
	unsigned m_MaterialCount;
	bool m_AddMaterialBlend;

	unsigned m_MaterialEditSize;
	unsigned m_VolumeEditSize;

	bool m_IsRightButtonDown;
	std::pair<int, int> m_LastMousePos;
};

