// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information

#include "stdafx.h"

#include "VolumeRenderingApplication.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch(message)
    {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

#ifdef PROFI_ENABLE
#ifdef DEBUG
typedef profi::DebugMallocAllocator ProfiAllocator;
#else
typedef profi::DefaultMallocAllocator ProfiAllocator;
#endif
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	#ifdef PROFI_ENABLE
	ProfiAllocator allocator;
	profi::Initialize(&allocator);
	#endif

	Logging::Logger::Initialize();
	Logging::Logger::Get().AddTarget(new std::ofstream("VolumeRenderingApplication_App.log"));

	auto app = std::unique_ptr<VolumeRenderingApplication>(new VolumeRenderingApplication(hInstance));

	if(app->Initiate("VolumeRenderingApp", "VolumeRendering application", 1280, 720, &WndProc))
	{
		app->DoMessageLoop();
	}

	app.reset();
	Logging::Logger::Deinitialize();

	#ifdef PROFI_ENABLE
	// dump the perf
	profi::IReport* report(profi::GetReportJSON());
	
	std::ofstream fout("performance.json");
	fout.write((const char*)report->Data(), report->Size());

	report->Release();
	profi::Deinitialize();
	#endif

	return 0;
}