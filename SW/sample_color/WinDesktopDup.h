#include <vector>
#include <d3dcommon.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <string>

class WinDesktopDup {
public:
	~WinDesktopDup();

	bool Initialize();
	bool  CaptureFrame(uint8_t *screen);
	void  Close();

private:
	IDXGIOutputDuplication* DeskDupl;
	IDXGIResource*			deskRes;
	IDXGIDevice*			dxgiDevice;
	IDXGIAdapter*			dxgiAdapter;
	IDXGIOutput*			dxgiOutput;
	IDXGIOutput1*			dxgiOutput1;
	DXGI_OUTDUPL_FRAME_INFO frameInfo;
	ID3D11Device*			D3DDevice;
	ID3D11Texture2D*		cpuTex;
	ID3D11Texture2D*		gpuTex;
	D3D11_TEXTURE2D_DESC    desc;
	ID3D11DeviceContext*	D3DDeviceContext;
	DXGI_OUTPUT_DESC        OutputDesc;
	HRESULT                 hr;
	bool                    HaveFrameLock = false;
	int						OutputNumber = 0;
};