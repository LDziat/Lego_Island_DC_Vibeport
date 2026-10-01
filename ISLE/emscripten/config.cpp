#include "config.h"

#include "filesystem.h"
#include "window.h"

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
#include <emscripten.h>
#include <iniparser.h>

void Emscripten_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
	SDL_Log("Overriding default config for Emscripten");

	iniparser_set(p_dictionary, "isle:diskpath", Emscripten_bundledPath);
	iniparser_set(p_dictionary, "isle:cdpath", Emscripten_streamPath);
	iniparser_set(p_dictionary, "isle:savepath", Emscripten_savePath);
	iniparser_set(p_dictionary, "isle:Full Screen", "false");
	iniparser_set(p_dictionary, "isle:Flip Surfaces", "true");

	// Emscripten-only for now
	Emscripten_SetScaleAspect(iniparser_getboolean(p_dictionary, "isle:Original Aspect Ratio", true));
	Emscripten_SetOriginalResolution(iniparser_getboolean(p_dictionary, "isle:Original Resolution", true));

	// clang-format off
	MAIN_THREAD_EM_ASM({JSEvents.fullscreenEnabled = function() { return false; }});
// clang-format on
}
