#ifndef EMSCRIPTEN_MESSAGE_BOX_H
#define EMSCRIPTEN_MESSAGE_BOX_H

#include <SDL/SDL_version.h>
#if defined(SDL_VERSION_ATLEAST) && SDL_VERSION_ATLEAST(2,0,0)
#include <SDL/SDL_messagebox.h>
#endif

bool Emscripten_ShowSimpleMessageBox(
	SDL_MessageBoxFlags flags,
	const char* title,
	const char* message,
	SDL_Window* window
);

#endif // EMSCRIPTEN_MESSAGE_BOX_H
