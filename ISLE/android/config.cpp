#include "config.h"

#include "mxstring.h"

#include <SDL/SDL_version.h>
#if defined(SDL_VERSION_ATLEAST) && SDL_VERSION_ATLEAST(2,0,0)
#include <SDL/SDL_filesystem.h>
#endif
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
#include <SDL/SDL_system.h>
#include <iniparser.h>

void Android_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
	SDL_Log("Overriding default config for Android");

	const char* data = SDL_GetAndroidExternalStoragePath();
	MxString savedata = MxString(data) + "/saves/";

	if (!SDL_GetPathInfo(savedata.GetData(), NULL)) {
		SDL_CreateDirectory(savedata.GetData());
	}

	iniparser_set(p_dictionary, "isle:diskpath", data);
	iniparser_set(p_dictionary, "isle:cdpath", data);
	iniparser_set(p_dictionary, "isle:mediapath", data);
	iniparser_set(p_dictionary, "isle:savepath", savedata.GetData());

	// Default to Virtual Mouse
	char buf[16];
	iniparser_set(p_dictionary, "isle:Touch Scheme", SDL_itoa(0, buf, 10));
}
