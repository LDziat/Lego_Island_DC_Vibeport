#include "mxdebug.h"

#ifdef _DEBUG

// Debug-only wrapper for OutputDebugString to support variadic arguments.
// Identical functions at BETA10 0x100ec9fe and 0x101741b5 are more limited in scope.
// This is the most widely used version.

#include <SDL/SDL_version.h>
#if defined(SDL_VERSION_ATLEAST) && SDL_VERSION_ATLEAST(2,0,0)
#include <SDL/SDL_log.h>
#else
#include <stdio.h>
#ifndef SDL_Log
#define SDL_Log printf
#endif
#ifndef SDL_LogInfo
#define SDL_LogInfo(cat, ...) printf(__VA_ARGS__)
#endif
#ifndef SDL_LogError
#define SDL_LogError(cat, ...) printf(__VA_ARGS__)
#endif
#endif
#include <stdio.h>

// FUNCTION: BETA10 0x10124cb9
int DebugHeapState()
{
	return 0;
}

// FUNCTION: BETA10 0x10124cdd
void _MxTrace(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_TRACE, format, args);
	va_end(args);
}

#endif
