#pragma once

#include "miniwin/ddraw.h"

struct DirectDrawPaletteImpl : public IDirectDrawPalette {
	DirectDrawPaletteImpl(LPPALETTEENTRY lpColorTable);
	~DirectDrawPaletteImpl() override;
	HRESULT GetEntries(DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries) override;
	HRESULT SetEntries(DWORD dwFlags, DWORD dwStartingEntry, DWORD dwCount, LPPALETTEENTRY lpEntries) override;

#ifdef __DREAMCAST__
	SDL_Color m_palette[256];
#else
	SDL_Palette* m_palette;
#endif
};
