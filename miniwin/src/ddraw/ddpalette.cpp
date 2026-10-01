#include "ddpalette_impl.h"
#include "miniwin/ddraw.h"

#include <SDL/SDL.h>

DirectDrawPaletteImpl::DirectDrawPaletteImpl(LPPALETTEENTRY lpColorTable)
{
#ifndef __DREAMCAST__
	
    m_palette = (SDL_Palette*)malloc(sizeof(SDL_Palette));
    m_palette->ncolors = 256;
    m_palette->colors = (SDL_Color*)malloc(sizeof(SDL_Color) * 256);

#endif
	SetEntries(0, 0, 256, lpColorTable);
}

DirectDrawPaletteImpl::~DirectDrawPaletteImpl()
{
#ifndef __DREAMCAST__
	SDL_DestroyPalette(m_palette);
#endif
}

HRESULT DirectDrawPaletteImpl::GetEntries(DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries)
{
	for (DWORD i = 0; i < dwNumEntries; i++) {
#ifdef __DREAMCAST__
		lpEntries[i].peRed = m_palette[dwBase + i].r;
		lpEntries[i].peGreen = m_palette[dwBase + i].g;
		lpEntries[i].peBlue = m_palette[dwBase + i].b;
#else
		lpEntries[i].peRed = m_palette->colors[dwBase + i].r;
		lpEntries[i].peGreen = m_palette->colors[dwBase + i].g;
		lpEntries[i].peBlue = m_palette->colors[dwBase + i].b;
#endif
		lpEntries[i].peFlags = PC_NONE;
	}
	return DD_OK;
}

HRESULT DirectDrawPaletteImpl::SetEntries(DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries)
{
#ifdef __DREAMCAST__
	for (DWORD i = 0; i < dwCount; i++) {
		m_palette[i + dwStartingEntry].r = lpEntries[i].peRed;
		m_palette[i + dwStartingEntry].g = lpEntries[i].peGreen;
		m_palette[i + dwStartingEntry].b = lpEntries[i].peBlue;
		m_palette[i + dwStartingEntry].unused = 255;
	}
#else
	SDL_Color colors[256];
	for (DWORD i = 0; i < dwCount; i++) {
		colors[i + dwStartingEntry].r = lpEntries[i].peRed;
		colors[i + dwStartingEntry].g = lpEntries[i].peGreen;
		colors[i + dwStartingEntry].b = lpEntries[i].peBlue;
		colors[i + dwStartingEntry].a = SDL_ALPHA_OPAQUE;
	}

	SDL_SetPaletteColors(m_palette, colors, dwStartingEntry, dwCount);
#endif

	return DD_OK;
}
