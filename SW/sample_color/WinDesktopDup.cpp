#include "WinDesktopDup.h"
#include <winerror.h>
#include <winuser.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#pragma comment(lib,"d3d11.lib") // add d3d11 to link libraries

WinDesktopDup::~WinDesktopDup() {
	Close();
}

bool WinDesktopDup::Initialize() {
	// Get desktop
	HDESK hDesk = OpenInputDesktop(0, FALSE, GENERIC_ALL);
	if (!hDesk)
	{
		printf("Failed to open desktop.\n");
		return false;
	}

	// Attach desktop to this thread (presumably for cases where this is not the main/UI thread)
	bool deskAttached = SetThreadDesktop(hDesk) != 0;
	CloseDesktop(hDesk);
	hDesk = nullptr;
	if (!deskAttached)
	{
		printf("Failed to attach recording thread to desktop.\n");
		return false;
	}		

	// Initialize DirectX
	HRESULT hr = S_OK;

	// Driver types supported
	D3D_DRIVER_TYPE driverTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	auto numDriverTypes = ARRAYSIZE(driverTypes);

	// Feature levels supported
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1 };
	auto numFeatureLevels = ARRAYSIZE(featureLevels);

	D3D_FEATURE_LEVEL featureLevel;

	// Create device
	for (size_t i = 0; i < numDriverTypes; i++)
	{
		hr = D3D11CreateDevice(nullptr, driverTypes[i], nullptr, 0, featureLevels, (UINT)numFeatureLevels,
			D3D11_SDK_VERSION, &D3DDevice, &featureLevel, &D3DDeviceContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
	{
		printf("D3D11CreateDevice failed.\n");
		return false;
	}
	printf("Feature level is %d.\n", featureLevel);
		

	// Initialize the Desktop Duplication system

	// Get DXGI device
	hr = D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if (FAILED(hr))
	{
		printf("D3DDevice->QueryInterface failed.\n");
		return false;
	}

	// Get DXGI adapter
	hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);
	dxgiDevice->Release();
	dxgiDevice = nullptr;
	if (FAILED(hr))
	{
		printf("dxgiDevice->GetParent failed.\n");
		return false;
	}		

	// Get output
	hr = dxgiAdapter->EnumOutputs(OutputNumber, &dxgiOutput);
	dxgiAdapter->Release();
	dxgiAdapter = nullptr;
	if (FAILED(hr))
	{
		printf("dxgiAdapter->EnumOutputs failed.\n");
		return false;
	}		

	dxgiOutput->GetDesc(&OutputDesc);

	// QI for Output 1
	hr = dxgiOutput->QueryInterface(__uuidof(dxgiOutput1), (void**)&dxgiOutput1);
	dxgiOutput->Release();
	dxgiOutput = nullptr;
	if (FAILED(hr))
	{
		printf("dxgiOutput->QueryInterface failed.\n"); 
		return false;
	}

	// Create desktop duplication
	hr = dxgiOutput1->DuplicateOutput(D3DDevice, &DeskDupl);
	dxgiOutput1->Release();
	dxgiOutput1 = nullptr;
	if (FAILED(hr))
	{
		if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		{
			printf("Too many desktop recorders already active.\n");
			return false;
		}

		printf("DuplicateOutput failed.\n");
		return false;
	}

	return "";
}

bool WinDesktopDup::CaptureFrame(uint8_t *screen) {
	if (!DeskDupl)
		return false;

	HRESULT hr;

	// according to the docs, it's best for performance if we hang onto the frame for as long as possible,
	// and only release the previous frame immediately before acquiring the next one. Something about
	// the OS coalescing updates, so that it doesn't have to store them as distinct things.
	if (HaveFrameLock)
	{
		HaveFrameLock = false;
		DeskDupl->ReleaseFrame();
	}
	HaveFrameLock = true;

	hr = DeskDupl->AcquireNextFrame(0, &frameInfo, &deskRes);

	if (hr == DXGI_ERROR_WAIT_TIMEOUT || FAILED(hr))
		return false;

	hr = deskRes->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&gpuTex);
	deskRes->Release();
	deskRes = nullptr;
	if (FAILED(hr))
		return false;

	gpuTex->GetDesc(&desc);
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; // only read is needed?
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // choose format for cpu texture, can't change this for some reason
	desc.MiscFlags = 0; // D3D11_RESOURCE_MISC_GDI_COMPATIBLE ?

	hr = D3DDevice->CreateTexture2D(&desc, nullptr, &cpuTex);
	if (!SUCCEEDED(hr))
		return false;

	D3DDeviceContext->CopyResource(cpuTex, gpuTex);
	D3D11_MAPPED_SUBRESOURCE sr;
	hr = D3DDeviceContext->Map(cpuTex, 0, D3D11_MAP_READ, 0, &sr);
	if (!SUCCEEDED(hr))
		return false;

	for (int y = 0; y < (int)desc.Height; y++)
		memcpy(screen + y * desc.Width * 4, (uint8_t*)sr.pData + sr.RowPitch * y, desc.Width * 4);

	//memcpy(screen, sr.pData, desc.Height * desc.Width * 4);

	D3DDeviceContext->Unmap(cpuTex, 0);
	cpuTex->Release();
	gpuTex->Release();

	return true;
}

void WinDesktopDup::Close() {
	if (DeskDupl)
		DeskDupl->Release();

	if (D3DDeviceContext)
		D3DDeviceContext->Release();

	if (D3DDevice)
		D3DDevice->Release();

	DeskDupl = nullptr;
	D3DDeviceContext = nullptr;
	D3DDevice = nullptr;
	HaveFrameLock = false;
}
