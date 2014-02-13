// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#include "stdafx.h"

#include "VolumeRenderingApplication.h"
#include "Scene.h"

#include "ClearRenderingRoutine.h"
#include "PresentRoutine.h"
#include "DrawRoutine.h"

#include "VoxelBall.h"

#include <boost/program_options.hpp>

using namespace DirectX;

namespace po = boost::program_options;

static const float MV_SPEED = 0.005f * 10;

void LogVoxelsMessage(Voxels::LogSeverity severity, const char* message)
{
	Logging::Severity sev;
	switch (severity)
	{
	case Voxels::LS_Trace:
		sev = Logging::Sev_Trace;
		break;
	case Voxels::LS_Debug:
		sev = Logging::Sev_Debug;
		break;
	case Voxels::LS_Info:
		sev = Logging::Sev_Info;
		break;
	case Voxels::LS_Warning:
		sev = Logging::Sev_Warning;
		break;
	case Voxels::LS_Error:
		sev = Logging::Sev_Error;
		break;
	case Voxels::LS_CriticalError:
		sev = Logging::Sev_Error;
		break;
	}

	STLOG(sev, Logging::Fac_Voxels, std::tie(message));
}

class AllocatorImpl
{
public:
	static void* Allocate(size_t size);
	static void Deallocate(void* ptr);

	static void* AllocateAligned(size_t size, size_t alignment);
	static void DeallocateAligned(void* ptr);
	static void ReportMemory();
private:
#if COUNT_MEMORY
	static size_t CurrentUsedMemory;
	static size_t MaxUsedMemory;
	static unsigned GetNextId();
	static unsigned NextId;

	struct AllocationInfo
	{
		unsigned Id;
		size_t Size;
	};
	static std::unordered_map<void*, AllocationInfo> Allocations;
#endif
};

#if COUNT_MEMORY
size_t AllocatorImpl::CurrentUsedMemory = 0;
size_t AllocatorImpl::MaxUsedMemory = 0;
unsigned AllocatorImpl::NextId = 0;
std::unordered_map<void*, AllocatorImpl::AllocationInfo> AllocatorImpl::Allocations;
#endif

#if COUNT_MEMORY
unsigned AllocatorImpl::GetNextId()
{
	return NextId++;
}
#endif

void* AllocatorImpl::Allocate(size_t size)
{
	auto ret = malloc(size);

#if COUNT_MEMORY
	CurrentUsedMemory += size;
	MaxUsedMemory = std::max(MaxUsedMemory, CurrentUsedMemory);
	AllocationInfo info = { GetNextId(), size};
	Allocations.insert(std::make_pair(ret, info));
#endif
	
	return ret;
}

void AllocatorImpl::Deallocate(void* ptr)
{
#if COUNT_MEMORY
	auto it = Allocations.find(ptr);
	assert(it != Allocations.end());
	CurrentUsedMemory -= it->second.Size;
	Allocations.erase(it);
#endif
	free(ptr);
}

void* AllocatorImpl::AllocateAligned(size_t size, size_t alignment)
{
	auto ret = _aligned_malloc(size, alignment);

#if COUNT_MEMORY
	CurrentUsedMemory += size;
	MaxUsedMemory = std::max(MaxUsedMemory, CurrentUsedMemory);
	AllocationInfo info = { GetNextId(), size };
	Allocations.insert(std::make_pair(ret, info));
#endif
	return ret;
}

void AllocatorImpl::DeallocateAligned(void* ptr)
{
#if COUNT_MEMORY
	auto it = Allocations.find(ptr);
	assert(it != Allocations.end());
	CurrentUsedMemory -= it->second.Size;
	Allocations.erase(it);
#endif
	_aligned_free(ptr);
}

void AllocatorImpl::ReportMemory()
{
#if COUNT_MEMORY
	SLOG(Sev_Info, Fac_Voxels, "Max Voxels used memory ", MaxUsedMemory, " bytes");
	assert(Allocations.empty() && "Voxels leaks detected!");
	for (auto it = Allocations.cbegin(); it != Allocations.cend(); ++it)
	{
		SLOG(Sev_Warning, Fac_Voxels, "Voxel leak - Id: ", it->second.Id, " Size: ", it->second.Size);
	}
#endif
}

VolumeRenderingApplication::VolumeRenderingApplication(HINSTANCE instance)
	: DxGraphicsApplication(instance)
	, m_IsRightButtonDown(false)
	, m_LastMousePos(std::make_pair(0, 0))
	, m_InjectionType(Voxels::IT_Add)
	, m_Modification(MT_Inject)
	, m_GridScale(1, 1, 1)
	, m_MaterialId(0)
	, m_MaterialCount(0)
	, m_AddMaterialBlend(true)
	, m_MaterialEditSize(20)
	, m_VolumeEditSize(3)
{}

VolumeRenderingApplication::~VolumeRenderingApplication()
{
	m_Scene.reset();
	DeinitializeVoxels();

#if COUNT_MEMORY
	AllocatorImpl::ReportMemory();
#endif
}

bool VolumeRenderingApplication::Initiate(char* className, char* windowName, unsigned width, unsigned height, WNDPROC winProc, bool sRGBRT)
{
	po::options_description desc("Options");
	desc.add_options()("grid", po::value<std::string>(), "load a specified voxel grid")
		("gridsize", po::value<unsigned>(), "grid size in voxels")
		("surface", po::value<std::string>(), "seed the grid with a surface type")
		("materials", po::value<std::string>(), "materials definition file")
		("heightmap", po::value<std::string>(), "heightmap file")
		("hscale", po::value<float>(), "heightmap scale factor")
		("xscale", po::value<float>(), "grid scale factor on X coordinate")
		("yscale", po::value<float>(), "grid scale factor on Y coordinate")
		("zscale", po::value<float>(), "grid scale factor on Z coordinate")
		("msaa", po::value<int>(), "msaa samples");

	po::variables_map options;
	auto arguments = po::split_winmain(::GetCommandLine());
	try {
		po::store(po::command_line_parser(arguments).options(desc).run(), options);
		po::notify(options);
	} catch (std::exception& ex) {
		SLLOG(Sev_Error, Fac_Rendering, "Unable to parse command line arguments - ", ex.what());
		return false;
	}

	int msaaSamples = 4;
	if(options.count("msaa")) {
		msaaSamples = options["msaa"].as<int>();
	}

	bool result = true;
	result &= DxGraphicsApplication::Initiate(className, windowName, width, height, false, winProc, true, msaaSamples);

	SetProjection(XM_PIDIV2, float(GetWidth())/GetHeight(), 1.0f, 500.f);

	GetMainCamera()->SetLookAt(XMFLOAT3(0, 20, 0)
							 , XMFLOAT3(0, 20, 1)
							 , XMFLOAT3(0, 1, 0));

	DxRenderer* renderer = static_cast<DxRenderer*>(GetRenderer());

	std::string grid = "";
	if(options.count("grid")) {
		grid = options["grid"].as<std::string>();
	}
	Scene::SeedSurface surfaceType = Scene::SSU_Plane;
	if(options.count("surface")) {
		auto type = options["surface"].as<std::string>();
		if(type == "sphere") {
			surfaceType = Scene::SSU_Sphere;
		} else if(type == "proc") {
			surfaceType = Scene::SSU_Proc;
		} else if(type == "box") {
			surfaceType = Scene::SSU_Box;
		}
	}
	std::string materials = "..\\media\\materials.json";
	if(options.count("materials")) {
		materials = options["materials"].as<std::string>();
	}
	std::string heightmap = "";
	if(options.count("heightmap")) {
		heightmap = options["heightmap"].as<std::string>();
	}
	float hscale = 1;
	if(options.count("hscale")) {
		hscale = options["hscale"].as<float>();
	}

	if(options.count("xscale")) {
		m_GridScale.x = options["xscale"].as<float>();
	}
	if(options.count("yscale")) {
		m_GridScale.y = options["yscale"].as<float>();
	}
	if(options.count("zscale")) {
		m_GridScale.z = options["zscale"].as<float>();
	}

	unsigned gridSize = 64;
	if (options.count("gridsize")) {
		gridSize = options["gridsize"].as<unsigned>();
	}

	// Initialize the Voxels library
	Voxels::VoxelsAllocators allocators;
	allocators.VoxelsAllocate = AllocatorImpl::Allocate;
	allocators.VoxelsDeallocate = AllocatorImpl::Deallocate;
	allocators.VoxelsAllocateAligned = AllocatorImpl::AllocateAligned;
	allocators.VoxelsDeallocateAligned = AllocatorImpl::DeallocateAligned;
	if (InitializeVoxels(VOXELS_VERSION, &LogVoxelsMessage, &allocators) != Voxels::IE_Ok) {
		return false;
	}

	m_Scene.reset(new Scene(grid, gridSize, materials, heightmap, surfaceType, hscale, m_GridScale));

	if (!m_Scene->GetPolygonSurface())
		return false;

	m_MaterialCount = m_Scene->GetMaterials().GetMaterialsCount();

	m_ClearRoutine.reset(new ClearRenderingRoutine());
	ReturnUnless(m_ClearRoutine->Initialize(renderer), false);
	renderer->AddRoutine(m_ClearRoutine.get());

	m_DrawRoutine.reset(new DrawRoutine());
	ReturnUnless(m_DrawRoutine->Initialize(renderer, GetMainCamera(), GetProjection(), m_Scene.get()), false);
	renderer->AddRoutine(m_DrawRoutine.get());

	m_PresentRoutine.reset(new PresentRoutine());
	ReturnUnless(m_PresentRoutine->Initialize(renderer), false);
	renderer->AddRoutine(m_PresentRoutine.get());

	return result;
}

void VolumeRenderingApplication::Update(float delta)
{
}

void VolumeRenderingApplication::KeyDown(unsigned int key)
{
	bool isShiftDown = !!(GetKeyState(VK_SHIFT) & 0x8000);
	
	switch(key)
	{
	case VK_UP:
		m_MainCamera.Move(MV_SPEED * (1 + 3*isShiftDown));
		break;
	case VK_DOWN:
		m_MainCamera.Move(-MV_SPEED * (1 + 3*isShiftDown));
		break;
	case VK_LEFT:
		m_MainCamera.Yaw(-MV_SPEED);
		break;
	case VK_RIGHT:
		m_MainCamera.Yaw(MV_SPEED);
		break;
	case VK_INSERT:
		m_MainCamera.Up(MV_SPEED);
		break;
	case VK_DELETE:
		m_MainCamera.Up(-MV_SPEED);
		break;
	case VK_HOME:
		m_MainCamera.Pitch(-MV_SPEED);
		break;
	case VK_END:
		m_MainCamera.Pitch(MV_SPEED);
		break;
	case VK_F2:
		m_Scene->SaveVoxelGrid("output.grd");
		break;
	case 'S':
		m_DrawRoutine->SetDrawSolid(!m_DrawRoutine->GetDrawSolid());
		break;
	case 'W':
		m_DrawRoutine->SetDrawWireframe(!m_DrawRoutine->GetDrawWireframe());
		break;
	case 'T':
		m_DrawRoutine->SetDrawTransitions(!m_DrawRoutine->GetDrawTransitions());
		break;
	case 'A':
		m_DrawRoutine->SetDrawSurface(!m_DrawRoutine->GetDrawSurface());
		break;
	case 'L':
		m_DrawRoutine->SetUseLodOctree(!m_DrawRoutine->GetUseLodOctree());
		break;
	case 'U':
		m_DrawRoutine->SetLodUpdateEnabled(!m_DrawRoutine->GetLodUpdateEnabled());
		break;
	case 'R':
		RecalculateGrid();
		break;
	case VK_ADD:
		m_AddMaterialBlend = true;
		break;
	case VK_SUBTRACT:
		m_AddMaterialBlend = false;
		break;
	case 'Z':
		m_DrawRoutine->SetCurrentLodToDraw(m_DrawRoutine->GetCurrentLodToDraw() + 1);
		break;
	case 'X':
		m_DrawRoutine->SetCurrentLodToDraw(m_DrawRoutine->GetCurrentLodToDraw() - 1);
		break;

	case 'M':
		m_Modification = ModificationType((m_Modification + 1) % MT_Count);
		break;

	case '0':
		m_MaterialId = std::min(m_MaterialCount - 1, 0u);
		break;
	case '1':
		m_InjectionType = Voxels::IT_Add;
		m_MaterialId = std::min(m_MaterialCount - 1, 1u);
		break;
	case '2':
		m_InjectionType = Voxels::IT_SubtractAddInner;
		m_MaterialId = std::min(m_MaterialCount - 1, 2u);
		break;
	case '3':
		m_InjectionType = Voxels::IT_Subtract;
		m_MaterialId = std::min(m_MaterialCount - 1, 3u);
		break;
	case '4':
		m_MaterialId = std::min(m_MaterialCount - 1, 4u);
		break;
	case '5':
		m_MaterialId = std::min(m_MaterialCount - 1, 5u);
		break;

	case VK_F5:
		m_VolumeEditSize = 2;
		m_MaterialEditSize = 5;
		break;
	case VK_F6:
		m_VolumeEditSize = 3;
		m_MaterialEditSize = 10;
		break;
	case VK_F7:
		m_VolumeEditSize = 6;
		m_MaterialEditSize = 20;
		break;
	case VK_F8:
		m_VolumeEditSize = 12;
		m_MaterialEditSize = 40;
		break;

	default:
		break;
	}
}

void VolumeRenderingApplication::MouseButtonDown(MouseBtn button, int x, int y)
{
	switch(button)
	{
	case MBT_Left:
		ModifyGrid(x, y);
		break;
	case MBT_Right:
		m_IsRightButtonDown = true;
		break;
	default:
		break;
	}
}

void VolumeRenderingApplication::MouseButtonUp(MouseBtn button, int x, int y)
{
	switch(button)
	{
	case MBT_Right:
		m_IsRightButtonDown = false;
		break;
	default:
		break;
	}
}

void VolumeRenderingApplication::MouseMove(int x, int y)
{
	if(m_IsRightButtonDown) {
		const int dX = m_LastMousePos.first - x;
		const int dY = m_LastMousePos.second - y;

		m_MainCamera.Yaw(-dX*MV_SPEED);
		m_MainCamera.Pitch(-dY*MV_SPEED);
	}

	m_LastMousePos = std::make_pair(x, y);
}

void VolumeRenderingApplication::ModifyGrid(int mouseX, int mouseY)
{
	const XMMATRIX gridWrld = XMLoadFloat4x4(&m_Scene->GetGridWorldMatrix());
	const XMMATRIX view = XMLoadFloat4x4(&GetMainCamera()->GetViewMatrix());
	const XMMATRIX proj = XMLoadFloat4x4(&GetProjection());

	const XMVECTOR startW = XMVector3Unproject(XMVectorSet(float(mouseX), float(mouseY), 0, 0), 0, 0, float(GetWidth()), float(GetHeight()), 0, 1, proj, view, gridWrld);
	const XMVECTOR endW = XMVector3Unproject(XMVectorSet(float(mouseX), float(mouseY), 1, 0), 0, 0, float(GetWidth()), float(GetHeight()), 0, 1, proj, view, gridWrld);

	Voxels::float3pair modified;
	XMVECTOR intersection;
		
	if(m_Scene->Intersect(startW, endW, intersection)) {
		XMFLOAT3 position;
		XMStoreFloat3(&position, intersection);

		// The grid works with Z up so swap
		auto posfl3 = Voxels::float3(position.x, position.z, position.y);

		if(m_Modification == MT_Inject) {
			auto ball = Voxels::VoxelBall(XMFLOAT3(0, 0, 0), float(m_VolumeEditSize));
			float ext = float(m_VolumeEditSize) * 2 + 1;
			modified = m_Scene->GetVoxelGrid()->InjectSurface(
				posfl3,
				Voxels::float3(ext, ext, ext),
				&ball,
				m_InjectionType);
		} else if(m_Modification == MT_ChangeMaterial) {
			const auto ext = Voxels::float3(
				float(m_MaterialEditSize),
				float(m_MaterialEditSize),
				float(m_MaterialEditSize));
			modified = m_Scene->GetVoxelGrid()->InjectMaterial(
				posfl3,
				ext,
				m_MaterialId,
				m_AddMaterialBlend);
		}

		RecalculateGrid(&modified);
	}
}

void VolumeRenderingApplication::RecalculateGrid(const Voxels::float3pair* modified)
{
	m_Scene->RecalculateGrid(modified);
	auto result = modified ? m_DrawRoutine->UpdateGrid() : m_DrawRoutine->ReloadGrid();

	if(!result)
	{
		SLOG(Sev_Error, Fac_Rendering, "Unable to reload grid for drawing");
	}
}
