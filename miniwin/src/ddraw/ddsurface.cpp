#include "../../../util/compat.h"
#include "d3drmtexture_impl.h"
#include "ddpalette_impl.h"
#include "ddraw_impl.h"
#include "ddsurface_impl.h"
#include "miniwin.h"

#include <assert.h>
#ifdef __DREAMCAST__
#include <dc/pvr.h>
#endif

DirectDrawSurfaceImpl::DirectDrawSurfaceImpl(int width, int height, int format)
{
	m_surface = nullptr;
#ifdef __DREAMCAST__
	m_vram_pixels = nullptr;
	if (false) {
		int bpp = (format == 8 || format == 1) ? 1 : 4;
		int pitch = width * bpp;
		m_vram_pixels = pvr_mem_malloc(height * pitch);
		if (m_vram_pixels) {
			if (bpp == 1) {
				m_surface = SDL_CreateRGBSurfaceFrom(m_vram_pixels, width, height, 8, pitch, 0, 0, 0, 0);
			} else {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				Uint32 rmask = 0xff000000, gmask = 0x00ff0000, bmask = 0x0000ff00, amask = 0x000000ff;
#else
				Uint32 rmask = 0x000000ff, gmask = 0x0000ff00, bmask = 0x00ff0000, amask = 0xff000000;
#endif
				m_surface = SDL_CreateRGBSurfaceFrom(m_vram_pixels, width, height, 32, pitch, rmask, gmask, bmask, amask);
			}
			if (m_surface) {
				SDL_Log("DirectDrawSurfaceImpl: Successfully created %dx%d surface directly in VRAM!", width, height);
			} else {
				pvr_mem_free(m_vram_pixels);
				m_vram_pixels = nullptr;
			}
		} else {
			SDL_Log("DirectDrawSurfaceImpl: pvr_mem_malloc failed for %dx%d surface!", width, height);
		}
	}
#endif

	if (!m_surface) {
		if (format == 8 || format == 1) {
			m_surface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
		} else {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			Uint32 rmask = 0xff000000, gmask = 0x00ff0000, bmask = 0x0000ff00, amask = 0x000000ff;
#else
			Uint32 rmask = 0x000000ff, gmask = 0x0000ff00, bmask = 0x00ff0000, amask = 0xff000000;
#endif
			m_surface = SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask);
		}

		if (!m_surface) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create surface: %s", SDL_GetError());
		}
	}
}

DirectDrawSurfaceImpl::~DirectDrawSurfaceImpl()
{
	SDL_FreeSurface(m_surface);
#ifdef __DREAMCAST__
	if (m_vram_pixels) {
		pvr_mem_free(m_vram_pixels);
	}
#endif
	if (m_palette) {
		m_palette->Release();
	}
	if (m_texture) {
		m_texture->Release();
	}
}

// IUnknown interface
HRESULT DirectDrawSurfaceImpl::QueryInterface(const GUID& riid, void** ppvObject)
{
	if (SDL_memcmp(&riid, &IID_IDirectDrawSurface3, sizeof(GUID)) == 0) {
		this->IUnknown::AddRef();
		*ppvObject = static_cast<IDirectDrawSurface3*>(this);
		return S_OK;
	}
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "DirectDrawSurfaceImpl does not implement guid");
	return E_NOINTERFACE;
}

// IDirectDrawSurface interface
HRESULT DirectDrawSurfaceImpl::AddAttachedSurface(LPDIRECTDRAWSURFACE lpDDSAttachedSurface)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::Blt(
	LPRECT lpDestRect,
	LPDIRECTDRAWSURFACE lpDDSrcSurface,
	LPRECT lpSrcRect,
	DDBltFlags dwFlags,
	LPDDBLTFX lpDDBltFx
)
{
	if ((dwFlags & DDBLT_COLORFILL) == DDBLT_COLORFILL) {
		Uint32 color;
		if (m_surface->format->BitsPerPixel == 8) {
			// For INDEX8 surfaces the fill color is a palette index, not RGBA.
			color = lpDDBltFx->dwFillColor & 0xFF;
		}
		else {
			Uint8 a = (lpDDBltFx->dwFillColor >> 24) & 0xFF;
			Uint8 r = (lpDDBltFx->dwFillColor >> 16) & 0xFF;
			Uint8 g = (lpDDBltFx->dwFillColor >> 8) & 0xFF;
			Uint8 b = lpDDBltFx->dwFillColor & 0xFF;

			
			color = SDL_MapRGBA(m_surface->format, r, g, b, a);
		}
		if (lpDestRect) {
			SDL_Rect dstRect = ConvertRect(lpDestRect);
			SDL_FillRect(m_surface, &dstRect, color);
		}
		else {
			SDL_FillRect(m_surface, nullptr, color);
		}
		return DD_OK;
	}

	auto other = static_cast<DirectDrawSurfaceImpl*>(lpDDSrcSurface);

	SDL_Rect srcRect = lpSrcRect ? ConvertRect(lpSrcRect) : SDL_Rect{0, 0, other->m_surface->w, other->m_surface->h};
	SDL_Rect dstRect = lpDestRect ? ConvertRect(lpDestRect) : SDL_Rect{0, 0, m_surface->w, m_surface->h};

	SDL_Surface* blitSource = other->m_surface;

	if (other->m_surface->format != m_surface->format) {
		blitSource = SDL_ConvertSurface(other->m_surface, m_surface->format, 0);
		if (!blitSource) {
			return DDERR_GENERIC;
		}
	}

	if (SDL_SoftStretch(blitSource, &srcRect, m_surface, &dstRect) < 0) {
		return DDERR_GENERIC;
	}

	if (blitSource != other->m_surface) {
		SDL_FreeSurface(blitSource);
	}
	if (m_texture) {
		m_texture->Changed(TRUE, FALSE);
	}
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::BltFast(
	DWORD dwX,
	DWORD dwY,
	LPDIRECTDRAWSURFACE lpDDSrcSurface,
	LPRECT lpSrcRect,
	DDBltFastFlags dwTrans
)
{
	RECT destRect = {
		(int) dwX,
		(int) dwY,
		(int) (lpSrcRect->right - lpSrcRect->left + dwX),
		(int) (lpSrcRect->bottom - lpSrcRect->top + dwY)
	};
	return Blt(&destRect, lpDDSrcSurface, lpSrcRect, DDBLT_NONE, nullptr);
}

HRESULT DirectDrawSurfaceImpl::Flip(LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DDFlipFlags dwFlags)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::GetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE* lplpDDAttachedSurface)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::GetDC(HDC* lphDC)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::GetPalette(LPDIRECTDRAWPALETTE* lplpDDPalette)
{
	if (!m_palette) {
		return DDERR_GENERIC;
	}
	m_palette->AddRef();
	*lplpDDPalette = m_palette;
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::GetPixelFormat(LPDDPIXELFORMAT lpDDPixelFormat)
{
	memset(lpDDPixelFormat, 0, sizeof(*lpDDPixelFormat));
	lpDDPixelFormat->dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
	
	if (m_surface->format->BitsPerPixel == 8) {
		lpDDPixelFormat->dwFlags |= DDPF_PALETTEINDEXED8;
	}
	lpDDPixelFormat->dwRGBBitCount = m_surface->format->BitsPerPixel;
	lpDDPixelFormat->dwRBitMask = m_surface->format->Rmask;
	lpDDPixelFormat->dwGBitMask = m_surface->format->Gmask;
	lpDDPixelFormat->dwBBitMask = m_surface->format->Bmask;
	lpDDPixelFormat->dwRGBAlphaBitMask = m_surface->format->Amask;
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::GetSurfaceDesc(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	lpDDSurfaceDesc->dwFlags = DDSD_PIXELFORMAT;
	GetPixelFormat(&lpDDSurfaceDesc->ddpfPixelFormat);
	lpDDSurfaceDesc->dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
	lpDDSurfaceDesc->dwWidth = m_surface->w;
	lpDDSurfaceDesc->dwHeight = m_surface->h;
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::IsLost()
{
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::Lock(
	LPRECT lpDestRect,
	LPDDSURFACEDESC lpDDSurfaceDesc,
	DDLockFlags dwFlags,
	HANDLE hEvent
)
{
	if (SDL_LockSurface(m_surface) < 0) {
		return DDERR_GENERIC;
	}

	GetSurfaceDesc(lpDDSurfaceDesc);
	lpDDSurfaceDesc->lpSurface = m_surface->pixels;
	lpDDSurfaceDesc->lPitch = m_surface->pitch;

	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::ReleaseDC(HDC hDC)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::Restore()
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::SetClipper(LPDIRECTDRAWCLIPPER lpDDClipper)
{
	MINIWIN_NOT_IMPLEMENTED();
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::SetColorKey(DDColorKeyFlags dwFlags, LPDDCOLORKEY lpDDColorKey)
{
	if (lpDDColorKey->dwColorSpaceLowValue != lpDDColorKey->dwColorSpaceHighValue) {
		MINIWIN_NOT_IMPLEMENTED();
	}

	if (SDL_SetSurfaceColorKey(m_surface, true, lpDDColorKey->dwColorSpaceLowValue) != 0) {
		return DDERR_GENERIC;
	}

	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::SetPalette(LPDIRECTDRAWPALETTE lpDDPalette)
{
	if (m_surface->format->BytesPerPixel != 1) {
		MINIWIN_NOT_IMPLEMENTED();
	}

	if (m_texture) {
		m_texture->Changed(FALSE, TRUE);
	}

	lpDDPalette->AddRef();

	if (m_palette) {
		m_palette->Release();
	}

	m_palette = lpDDPalette;
	if (m_palette) SDL_SetColors(m_surface, ((DirectDrawPaletteImpl*) m_palette)->m_palette, 0, 256);
	return DD_OK;
}

HRESULT DirectDrawSurfaceImpl::Unlock(LPVOID lpSurfaceData)
{
	SDL_UnlockSurface(m_surface);
	if (m_texture) {
		m_texture->Changed(TRUE, FALSE);
	}
	return DD_OK;
}

IDirect3DRMTexture2* DirectDrawSurfaceImpl::ToTexture()
{
	if (!m_texture) {
		m_texture = new Direct3DRMTextureImpl(this, false);
	}
	return m_texture;
}









