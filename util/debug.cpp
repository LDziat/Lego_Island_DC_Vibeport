#include "compat.h"
#include "dcprof.h"
#include <SDL/SDL.h>
#include <stdio.h>
#include <string.h>

#if defined(__DREAMCAST__) && defined(ISLE_DC_PROFILE)
#include <malloc.h>
#include <dc/pvr.h>

uint64_t g_dcZoneUs[Z_COUNT] = {0};
uint32_t g_dcCounter[C_COUNT] = {0};
bool g_debugProfile = true;

static uint64_t s_lastProfPrintUs = 0;

void DcProfInit() {
    s_lastProfPrintUs = timer_us_gettime64();
}

void DcProfFrameEnd() {
    if (!g_debugProfile) return;

    g_dcCounter[C_FRAMES]++;

    uint64_t now = timer_us_gettime64();
    if (s_lastProfPrintUs == 0) {
        s_lastProfPrintUs = now;
        return;
    }

    uint64_t elapsedUs = now - s_lastProfPrintUs;
    if (elapsedUs >= 2000000ULL) { // 2 seconds
        uint32_t frames = g_dcCounter[C_FRAMES];
        if (frames == 0) frames = 1;

        float fps = (float)frames * 1000000.0f / (float)elapsedUs;
        float tickMs = (float)g_dcZoneUs[Z_TICK] / (float)frames / 1000.0f;
        float vidMs  = (float)g_dcZoneUs[Z_VIDEO_TICKLE] / (float)frames / 1000.0f;
        float othMs  = (tickMs > vidMs) ? (tickMs - vidMs) : 0.0f;
        float vmMs   = (float)g_dcZoneUs[Z_VIEWMGR] / (float)frames / 1000.0f;
        float colMs  = (float)g_dcZoneUs[Z_COLLECT] / (float)frames / 1000.0f;
        float glMs   = (float)g_dcZoneUs[Z_GLSUBMIT] / (float)frames / 1000.0f;
        float swpMs  = (float)g_dcZoneUs[Z_SWAP] / (float)frames / 1000.0f;
        float texMs  = (float)g_dcZoneUs[Z_TEXUPLOAD] / (float)frames / 1000.0f;
        float mshMs  = (float)g_dcZoneUs[Z_MESHUPLOAD] / (float)frames / 1000.0f;

        uint32_t drawsPerFrame = g_dcCounter[C_DRAWS] / frames;
        uint32_t idxPerFrame   = g_dcCounter[C_INDICES] / frames;
        uint32_t dropPerFrame  = g_dcCounter[C_INDICES_DROPPED] / frames;
        uint32_t dstCull       = g_dcCounter[C_DIST_CULLED] / frames;
        uint32_t frstCull      = g_dcCounter[C_FRUSTUM_CULLED] / frames;
        uint32_t texUps        = g_dcCounter[C_TEX_UPLOADS];
        uint32_t mshUps        = g_dcCounter[C_MESH_UPLOADS];

        struct mallinfo mi = mallinfo();
        int freeRamK = mi.fordblks / 1024;

        pvr_stats_t pvrst;
        float pvrRndMs = 0.0f;
        if (pvr_get_stats(&pvrst) == 0) {
            pvrRndMs = (float)pvrst.rnd_last_time / 1000000.0f;
        }

        printf("[PROF] FPS:%4.1f | Tick:%5.1fms (Vid:%4.1f Oth:%4.1f) | VM:%4.1f | Col:%4.1f | GL:%4.1f | Swp:%4.1f | TexUp:%3.1f(n=%d) | MshUp:%3.1f(n=%d)\n",
               fps, tickMs, vidMs, othMs, vmMs, colMs, glMs, swpMs, texMs, texUps, mshMs, mshUps);
        printf("[PROF]   Draws:%3d | Idx:%5d (Drop:%4d) | Cull(Dst:%3d Frst:%3d) | RAM free:%4dKB | PVR rnd:%4.1fms\n",
               drawsPerFrame, idxPerFrame, dropPerFrame, dstCull, frstCull, freeRamK, pvrRndMs);

        for (int i = 0; i < Z_COUNT; ++i) g_dcZoneUs[i] = 0;
        for (int i = 0; i < C_COUNT; ++i) g_dcCounter[i] = 0;
        s_lastProfPrintUs = timer_us_gettime64();
    }
}
#endif

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
#ifdef ISLE_DC_PROFILE
            if (strstr(line, "Profile=0")) g_debugProfile = false;
            if (strstr(line, "Profile=1")) g_debugProfile = true;
#endif
        }
        fclose(f);
        SDL_Log("Loaded /cd/debug.ini: Tex=%d Snd=%d Mus=%d Flat=%d", g_debugDisableTextures, g_debugDisableSound, g_debugDisableMusic, g_debugFlatShaded);
    } else {
        SDL_Log("Could not open /cd/debug.ini, using defaults");
    }
#endif
}

