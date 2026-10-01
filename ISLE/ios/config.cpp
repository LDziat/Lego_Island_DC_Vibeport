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
#include <iniparser.h>

void IOS_SetupDefaultConfigOverrides(dictionary* p_dictionary)
{
	SDL_Log("Overriding default config for iOS");

	// Use DevelopmentFiles path for disk and cd paths
	// It's good to use that path since user can easily
	// connect through SMB and copy the files
	MxString documentFolder = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
	documentFolder += "isle";

	if (!SDL_GetPathInfo(documentFolder.GetData(), NULL)) {
		SDL_CreateDirectory(documentFolder.GetData());
	}

	iniparser_set(p_dictionary, "isle:diskpath", documentFolder.GetData());
	iniparser_set(p_dictionary, "isle:cdpath", documentFolder.GetData());
}
