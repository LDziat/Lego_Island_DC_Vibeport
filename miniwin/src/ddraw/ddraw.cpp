#include "../../../util/compat.h"

#include "d3drmrenderer.h"
#include "ddpalette_impl.h"
#include "ddraw_impl.h"
#include "ddsurface_impl.h"
#include "dummysurface_impl.h"
#include "miniwin.h"
#include "miniwin/d3d.h"

#include <SDL/SDL.h>
#include <assert.h>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#ifdef USE_OPENGL1
#include "d3drmrenderer_opengl1.h"
#endif

SDL_Surface* DDWindow;
Direct3DRMRenderer* DDRenderer;

HRESULT DirectDrawImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirectDraw2, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirectDraw2*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3D2, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3D2*>(this);
		return S_OK;
	}
	if (SDL_memcmp(&riid, &IID_IDirect3DMiniwin, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirect3DMiniwin*>(this);
		return S_OK;
	}
	MINIWIN_NOT_IMPLEMENTED();
	return E_NOINTERFACE;
}

// IDirectDraw interface
HRESULT DirectDrawImpl::CreateClipper(DWORD dwFlags, LPDIRECTDRAWCLIPPER* lplpDDClipper, IUnknown* pUnkOuter)
{
	*lplpDDClipper = new IDirectDrawClipper;

	return DD_OK;
}

HRESULT DirectDrawImpl::CreatePalette(
	DDPixelCaps dwFlags,
	LPPALETTEENTRY lpColorTable,
	LPDIRECTDRAWPALETTE* lplpDDPalette,
	IUnknown* pUnkOuter
)
{
	if ((dwFlags & DDPCAPS_8BIT) != DDPCAPS_8BIT) {
		return DDERR_INVALIDPARAMS;
	}

	*lplpDDPalette = static_cast<LPDIRECTDRAWPALETTE>(new DirectDrawPaletteImpl(lpColorTable));
	return DD_OK;
}

HRESULT DirectDrawImpl::CreateSurface(
	LPDDSURFACEDESC lpDDSurfaceDesc,
	LPDIRECTDRAWSURFACE* lplpDDSurface,
	IUnknown* pUnkOuter
)
{
	if ((lpDDSurfaceDesc->dwFlags & DDSD_CAPS) == DDSD_CAPS) {
		if ((lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_ZBUFFER) == DDSCAPS_ZBUFFER) {
			if ((lpDDSurfaceDesc->dwFlags & DDSD_ZBUFFERBITDEPTH) != DDSD_ZBUFFERBITDEPTH) {
				return DDERR_INVALIDPARAMS;
			}
#ifndef __DREAMCAST__
			SDL_Log("Todo: Set %" PRIu32 "bit Z-Buffer", lpDDSurfaceDesc->dwZBufferBitDepth);
#endif
			*lplpDDSurface = static_cast<IDirectDrawSurface*>(new DummySurfaceImpl);
			return DD_OK;
		}
		if ((lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) == DDSCAPS_PRIMARYSURFACE) {
			m_frameBuffer = new FrameBufferImpl(m_virtualWidth, m_virtualHeight);
			*lplpDDSurface = static_cast<IDirectDrawSurface*>(m_frameBuffer);
			return DD_OK;
		}
		if ((lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_3DDEVICE) == DDSCAPS_3DDEVICE) {
			m_frameBuffer->AddRef();
			*lplpDDSurface = static_cast<IDirectDrawSurface*>(m_frameBuffer);
			return DD_OK;
		}
		if ((lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_TEXTURE) == DDSCAPS_TEXTURE) {
			MINIWIN_TRACE("DDSCAPS_TEXTURE"); // Texture for use in 3D
		}
	}

	int format;
#ifdef MINIWIN_PIXELFORMAT
	format = MINIWIN_PIXELFORMAT;
#else
	format = 32;
#endif
	if ((lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT) == DDSD_PIXELFORMAT) {
		if ((lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB) == DDPF_RGB) {
			int bpp = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;
			Uint32 rMask = lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask;
			Uint32 gMask = lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask;
			Uint32 bMask = lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask;
			Uint32 aMask = (lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) == DDPF_ALPHAPIXELS
							   ? lpDDSurfaceDesc->ddpfPixelFormat.dwRGBAlphaBitMask
							   : 0;

			format = bpp;
			if (0) {
				return DDERR_INVALIDPIXELFORMAT;
			}
		}
	}

	if ((lpDDSurfaceDesc->dwFlags & (DDSD_WIDTH | DDSD_HEIGHT)) != (DDSD_WIDTH | DDSD_HEIGHT)) {
		return DDERR_INVALIDPARAMS;
	}

	int width = lpDDSurfaceDesc->dwWidth;
	int height = lpDDSurfaceDesc->dwHeight;
	if (width == 0 || height == 0) {
		return DDERR_INVALIDPARAMS;
	}

#ifndef __DREAMCAST__
	SDL_Log("CreateSurface: w=%d h=%d format=%d dwFlags=0x%x RGBBitCount=%d", width, height, format, lpDDSurfaceDesc->dwFlags, lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount);
#endif

	*lplpDDSurface = static_cast<IDirectDrawSurface*>(new DirectDrawSurfaceImpl(width, height, format));
	return DD_OK;
}

HRESULT DirectDrawImpl::EnumDisplayModes(
	DWORD dwFlags,
	LPDDSURFACEDESC lpDDSurfaceDesc,
	LPVOID lpContext,
	LPDDENUMMODESCALLBACK lpEnumModesCallback
)
{
	SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
	if (!displayID) {
		return DDERR_GENERIC;
	}

	SDL_Rect** modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
	if (modes == (SDL_Rect**)0 || modes == (SDL_Rect**)-1) {
		return DDERR_GENERIC;
	}
	int count_modes = 0;
	while (modes[count_modes]) count_modes++;

	int format;
	HRESULT status = S_OK;

	for (int i = 0; i < count_modes; i++) {
#ifdef MINIWIN_PIXELFORMAT
		format = MINIWIN_PIXELFORMAT;
#else
		format = 32;
#endif

		const SDL_PixelFormat* details = DDWindow->format;
		if (!details) {
			continue;
		}
		DDSURFACEDESC ddsd = {};
		ddsd.dwSize = sizeof(DDSURFACEDESC);
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		ddsd.dwWidth = modes[i]->w;
		ddsd.dwHeight = modes[i]->h;
		ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
		ddsd.ddpfPixelFormat.dwRGBBitCount = details->bits_per_pixel;
		if (details->bits_per_pixel == 8) {
			ddsd.ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
		}
		ddsd.ddpfPixelFormat.dwRBitMask = details->Rmask;
		ddsd.ddpfPixelFormat.dwGBitMask = details->Gmask;
		ddsd.ddpfPixelFormat.dwBBitMask = details->Bmask;
		ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = details->Amask;

		if (!lpEnumModesCallback(&ddsd, lpContext)) {
			status = DDERR_GENERIC;
		}
	}
	SDL_free(modes);

	return status;
}

HRESULT DirectDrawImpl::FlipToGDISurface()
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawImpl::GetCaps(LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps)
{
	if (lpDDDriverCaps) {
		if (lpDDDriverCaps->dwSize >= sizeof(DDCAPS)) {
			lpDDDriverCaps->dwCaps2 = DDCAPS2_CERTIFIED; // Required to enable lighting
		}
	}

	if (lpDDHELCaps) {
		if (lpDDDriverCaps->dwSize >= sizeof(DDCAPS)) {
			lpDDDriverCaps->dwCaps2 = DDCAPS2_CERTIFIED; // Required to enable lighting
		}
	}

	return S_OK;
}

void EnumDevice(
	LPD3DENUMDEVICESCALLBACK cb,
	void* ctx,
	const char* name,
	D3DDEVICEDESC* halDesc,
	D3DDEVICEDESC* helDesc,
	GUID deviceGuid
)
{
	char* deviceNameDup = SDL_strdup(name);
	char* deviceDescDup = SDL_strdup("Miniwin driver");
	cb(&deviceGuid, deviceNameDup, deviceDescDup, halDesc, helDesc, ctx);
	SDL_free(deviceDescDup);
	SDL_free(deviceNameDup);
}

HRESULT DirectDrawImpl::EnumDevices(LPD3DENUMDEVICESCALLBACK cb, void* ctx)
{
	Direct3DRMRenderer_EnumDevices(this, cb, ctx);
	return S_OK;
}

HRESULT DirectDrawImpl::GetDisplayMode(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
	if (!displayID) {
		return DDERR_GENERIC;
	}

	const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(displayID);
	if (0) {
		return DDERR_GENERIC;
	}

	int format;
#ifdef MINIWIN_PIXELFORMAT
	format = MINIWIN_PIXELFORMAT;
#else
	if (m_virtualBPP == 8 || (m_frameBuffer && m_frameBuffer->IsIndex8())) {
		format = SDL_PIXELFORMAT_INDEX8;
	}
	else {
		format = 32;
	}
#endif

	const SDL_PixelFormat* details = DDWindow->format;

	lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
	lpDDSurfaceDesc->dwWidth = 640;
	lpDDSurfaceDesc->dwHeight = 480;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = details->bits_per_pixel;
	lpDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
	if (details->bits_per_pixel == 8) {
		lpDDSurfaceDesc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
	}
	lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = details->Rmask;
	lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = details->Gmask;
	lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = details->Bmask;
	lpDDSurfaceDesc->ddpfPixelFormat.dwRGBAlphaBitMask = details->Amask;

	return DD_OK;
}

HRESULT DirectDrawImpl::RestoreDisplayMode()
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawImpl::SetCooperativeLevel(HWND hWnd, DDSCLFlags dwFlags)
{
	SDL_Surface* sdlWindow = reinterpret_cast<SDL_Surface*>(hWnd);

	if (m_virtualWidth == 0 || m_virtualHeight == 0) {
		if (!SDL_GetWindowSizeInPixels(sdlWindow, &m_virtualWidth, &m_virtualHeight)) {
			SDL_LogError(LOG_CATEGORY_MINIWIN, "SDL_GetWindowSizeInPixels: %s", SDL_GetError());
		}
	}

	if (sdlWindow) {
		bool fullscreen;
		if ((dwFlags & DDSCL_NORMAL) == DDSCL_NORMAL) {
			fullscreen = false;
		}
		else if ((dwFlags & DDSCL_FULLSCREEN) == DDSCL_FULLSCREEN) {
			fullscreen = true;
		}
		else {
			return DDERR_INVALIDPARAMS;
		}

#ifndef __EMSCRIPTEN__
		if (!SDL_SetWindowFullscreen(sdlWindow, fullscreen)) {
			return DDERR_GENERIC;
		}
#endif
		DDWindow = sdlWindow;
	}
	return DD_OK;
}

HRESULT DirectDrawImpl::SetDisplayMode(DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
	m_virtualWidth = dwWidth;
	m_virtualHeight = dwHeight;
	m_virtualBPP = dwBPP;
	return DD_OK;
}

// IDirect3D2 interface
HRESULT DirectDrawImpl::CreateDevice(
	const GUID& guid,
	IDirectDrawSurface* pBackBuffer,
	IDirect3DDevice2** ppDirect3DDevice
)
{
	DDSURFACEDESC DDSDesc;
	DDSDesc.dwSize = sizeof(DDSURFACEDESC);
	pBackBuffer->GetSurfaceDesc(&DDSDesc);

	DDRenderer = CreateDirect3DRMRenderer(this, DDSDesc, &guid);
	if (!DDRenderer) {
		printf("Device GUID not recognized in DirectDrawImpl::CreateDevice:\n");
		printf("GUID passed: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
			guid.m_data1, guid.m_data2, guid.m_data3,
			guid.m_data4[0], guid.m_data4[1], guid.m_data4[2], guid.m_data4[3],
			guid.m_data4[4], guid.m_data4[5], guid.m_data4[6], guid.m_data4[7]);
#ifdef USE_OPENGL1
		printf("Expected OpenGL1_GUID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
			OpenGL1_GUID.m_data1, OpenGL1_GUID.m_data2, OpenGL1_GUID.m_data3,
			OpenGL1_GUID.m_data4[0], OpenGL1_GUID.m_data4[1], OpenGL1_GUID.m_data4[2], OpenGL1_GUID.m_data4[3],
			OpenGL1_GUID.m_data4[4], OpenGL1_GUID.m_data4[5], OpenGL1_GUID.m_data4[6], OpenGL1_GUID.m_data4[7]);
#endif
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Device GUID not recognized");
		return E_NOINTERFACE;
	}
	*ppDirect3DDevice = static_cast<IDirect3DDevice2*>(DDRenderer);
	return DD_OK;
}

HRESULT DirectDrawEnumerate(LPDDENUMCALLBACKA cb, void* context)
{
	const char* driverName = SDL_GetCurrentVideoDriver();
	char* driverNameDup = SDL_strdup(driverName);
	BOOL callback_result = cb(NULL, driverNameDup, NULL, context);
	SDL_free(driverNameDup);
	if (!callback_result) {
		return DDERR_GENERIC;
	}

	return DD_OK;
}

HRESULT DirectDrawCreate(LPGUID lpGuid, LPDIRECTDRAW* lplpDD, IUnknown* pUnkOuter)
{
	*lplpDD = new DirectDrawImpl;
	return DD_OK;
}
