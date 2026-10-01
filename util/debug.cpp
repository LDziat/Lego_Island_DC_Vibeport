#include "compat.h"
#include <SDL/SDL.h>
#include <stdio.h>
#include <string.h>

bool g_debugDisableTextures = false;
bool g_debugDisableSound = false;
bool g_debugDisableMusic = false;
bool g_debugFlatShaded = false;

void InitDebugFlags() {
#ifdef __DREAMCAST__
    FILE* f = fopen("/cd/debug.ini", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "DisableTextures=1")) g_debugDisableTextures = true;
            if (strstr(line, "DisableSound=1")) g_debugDisableSound = true;
            if (strstr(line, "DisableMusic=1")) g_debugDisableMusic = true;
            if (strstr(line, "FlatShaded=1")) g_debugFlatShaded = true;
        }
        fclose(f);
        SDL_Log("Loaded /cd/debug.ini: Tex=%d Snd=%d Mus=%d Flat=%d", g_debugDisableTextures, g_debugDisableSound, g_debugDisableMusic, g_debugFlatShaded);
    } else {
        SDL_Log("Could not open /cd/debug.ini, using defaults");
    }
#endif
}
