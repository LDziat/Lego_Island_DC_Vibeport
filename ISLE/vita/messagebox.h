#ifndef VITA_MESSAGE_BOX_H
#define VITA_MESSAGE_BOX_H

#include <SDL/SDL_version.h>
#if defined(SDL_VERSION_ATLEAST) && SDL_VERSION_ATLEAST(2,0,0)
#include <SDL/SDL_messagebox.h>
#endif

bool Vita_ShowSimpleMessageBox(SDL_MessageBoxFlags flags, const char* title, const char* message, SDL_Window* window);

#endif // VITA_MESSAGE_BOX_H
