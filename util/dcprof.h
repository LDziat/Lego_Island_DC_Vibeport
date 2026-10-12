#pragma once

#if defined(__DREAMCAST__) && defined(ISLE_DC_PROFILE)
#include <kos/timer.h>
#include <stdint.h>

enum DcZone {
	Z_TICK = 0,
	Z_VIDEO_TICKLE,
	Z_VIEWMGR,
	Z_COLLECT,
	Z_GLSUBMIT,
	Z_SWAP,
	Z_TEXUPLOAD,
	Z_MESHUPLOAD,
	Z_COUNT
};

enum DcCounter {
	C_FRAMES = 0,
	C_DRAWS,
	C_INDICES,
	C_INDICES_DROPPED,
	C_DIST_CULLED,
	C_FRUSTUM_CULLED,
	C_TEX_UPLOADS,
	C_TEX_PIXELS,
	C_MESH_UPLOADS,
	C_COUNT
};

extern uint64_t g_dcZoneUs[Z_COUNT];
extern uint32_t g_dcCounter[C_COUNT];
extern bool g_debugProfile;

struct DcScope {
	DcZone z;
	uint64_t t0;
	inline DcScope(DcZone zz) : z(zz) {
		t0 = timer_us_gettime64();
	}
	inline ~DcScope() {
		g_dcZoneUs[z] += (timer_us_gettime64() - t0);
	}
};

#define DC_ZONE(z) DcScope _dcs_##__LINE__(z)
#define DC_COUNT(c, n) do { g_dcCounter[c] += (n); } while (0)

void DcProfInit();
void DcProfFrameEnd();

#else

#define DC_ZONE(z) ((void)0)
#define DC_COUNT(c, n) ((void)0)
inline void DcProfInit() {}
inline void DcProfFrameEnd() {}

#endif
