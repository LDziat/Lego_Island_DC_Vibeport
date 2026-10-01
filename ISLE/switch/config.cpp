#include "config.h"

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
#include <iniparser.h>

void NX_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
	iniparser_set(p_dictionary, "isle:diskpath", "sdmc:/switch/isle/LEGO/");
	iniparser_set(p_dictionary, "isle:cdpath", "sdmc:/switch/isle/");
	iniparser_set(p_dictionary, "isle:savepath", "sdmc:/switch/isle/");
	iniparser_set(p_dictionary, "isle:Cursor Sensitivity", "16.000000");
}
