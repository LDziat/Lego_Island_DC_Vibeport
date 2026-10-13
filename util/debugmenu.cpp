#include "debugmenu.h"
#include "compat.h"
#include "dcprof.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef __DREAMCAST__
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glkos.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <arch/timer.h>
#else
#include <SDL/SDL_opengl.h>
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// Forward references to game functions
#include "misc.h"
#include "mxbackgroundaudiomanager.h"
#include "legosoundmanager.h"
#include "legoanimationmanager.h"
#include "realtime/realtimeview.h"
#include <math.h>

// Options state
static bool s_menuOpen = false;
static int s_selectedItem = 0;

enum MenuOption {
	OPT_SOUND = 0,
	OPT_MUSIC,
	OPT_LIGHTING,
	OPT_AMBIENT,
	OPT_QUALITY,
	OPT_DRAWDIST,
	OPT_TEXTURES,
	OPT_FPS,
	OPT_RESOLUTION,
	OPT_COUNT
};

// Ambient levels table
static const float s_ambientLevels[] = { 0.15f, 0.25f, 0.38f, 0.50f, 0.65f, 0.80f };
static const int s_numAmbientLevels = sizeof(s_ambientLevels) / sizeof(s_ambientLevels[0]);

// Quality presets (Extras count + LOD level)
struct QualityPreset {
	int extras;
	float lod;
	const char* name;
};
static const QualityPreset s_qualityPresets[] = {
	{ 0,   0.0f, "Low (0 Extras, Low LOD)" },
	{ 5,   2.0f, "Med-Low (5 Extras, LOD 2)" },
	{ 15,  3.6f, "Medium (15 Extras, LOD 3.6)" },
	{ 30,  5.0f, "High (30 Extras, LOD 5)" },
	{ -1, 10.0f, "Max (All Extras, Full LOD)" }
};
static const int s_numQualityPresets = sizeof(s_qualityPresets) / sizeof(s_qualityPresets[0]);

// Draw distance presets
struct DrawDistPreset {
	float scale;
	const char* name;
};
static const DrawDistPreset s_drawDistPresets[] = {
	{ 0.50f, "Near (0.50x)" },
	{ 0.75f, "Medium (0.75x)" },
	{ 1.00f, "Normal (1.00x)" },
	{ 1.35f, "Far (1.35x)" }
};
static const int s_numDrawDistPresets = sizeof(s_drawDistPresets) / sizeof(s_drawDistPresets[0]);

// Embedded standard 8x8 font for ASCII 32..126 (95 glyphs)
static const uint8_t s_font8x8[95][8] = {
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
	{0x18,0x3c,0x3c,0x18,0x18,0x00,0x18,0x00}, // 33 '!'
	{0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, // 34 '"'
	{0x6c,0x6c,0xfe,0x6c,0xfe,0x6c,0x6c,0x00}, // 35 '#'
	{0x18,0x3e,0x60,0x3c,0x06,0x7c,0x18,0x00}, // 36 '$'
	{0x00,0x63,0x66,0x0c,0x18,0x33,0x63,0x00}, // 37 '%'
	{0x38,0x6c,0x38,0x76,0xdc,0xcc,0x76,0x00}, // 38 '&'
	{0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // 39 '''
	{0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00}, // 40 '('
	{0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00}, // 41 ')'
	{0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00}, // 42 '*'
	{0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00}, // 43 '+'
	{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 44 ','
	{0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00}, // 45 '-'
	{0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 '.'
	{0x06,0x0c,0x18,0x30,0x60,0xc0,0x80,0x00}, // 47 '/'
	{0x3c,0x66,0xc3,0xc3,0xc3,0x66,0x3c,0x00}, // 48 '0'
	{0x18,0x38,0x18,0x18,0x18,0x18,0x7e,0x00}, // 49 '1'
	{0x3c,0x66,0x06,0x1c,0x30,0x60,0x7e,0x00}, // 50 '2'
	{0x3c,0x66,0x06,0x1c,0x06,0x66,0x3c,0x00}, // 51 '3'
	{0x0c,0x1c,0x34,0x64,0x7e,0x04,0x04,0x00}, // 52 '4'
	{0x7e,0x60,0x7c,0x06,0x06,0x66,0x3c,0x00}, // 53 '5'
	{0x1c,0x30,0x60,0x7c,0x66,0x66,0x3c,0x00}, // 54 '6'
	{0x7e,0x06,0x0c,0x18,0x30,0x30,0x30,0x00}, // 55 '7'
	{0x3c,0x66,0x66,0x3c,0x66,0x66,0x3c,0x00}, // 56 '8'
	{0x3c,0x66,0x66,0x3e,0x06,0x0c,0x38,0x00}, // 57 '9'
	{0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, // 58 ':'
	{0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00}, // 59 ';'
	{0x0c,0x18,0x30,0x60,0x30,0x18,0x0c,0x00}, // 60 '<'
	{0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00}, // 61 '='
	{0x30,0x18,0x0c,0x06,0x0c,0x18,0x30,0x00}, // 62 '>'
	{0x3c,0x66,0x06,0x0c,0x18,0x00,0x18,0x00}, // 63 '?'
	{0x3c,0x66,0x6e,0x6a,0x6e,0x60,0x3c,0x00}, // 64 '@'
	{0x18,0x3c,0x66,0x66,0x7e,0x66,0x66,0x00}, // 65 'A'
	{0x7c,0x66,0x66,0x7c,0x66,0x66,0x7c,0x00}, // 66 'B'
	{0x3c,0x66,0x60,0x60,0x60,0x66,0x3c,0x00}, // 67 'C'
	{0x78,0x6c,0x66,0x66,0x66,0x6c,0x78,0x00}, // 68 'D'
	{0x7e,0x60,0x60,0x7c,0x60,0x60,0x7e,0x00}, // 69 'E'
	{0x7e,0x60,0x60,0x7c,0x60,0x60,0x60,0x00}, // 70 'F'
	{0x3c,0x66,0x60,0x6e,0x66,0x66,0x3a,0x00}, // 71 'G'
	{0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0x00}, // 72 'H'
	{0x3c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, // 73 'I'
	{0x0e,0x06,0x06,0x06,0x06,0x66,0x3c,0x00}, // 74 'J'
	{0x66,0x6c,0x78,0x70,0x78,0x6c,0x66,0x00}, // 75 'K'
	{0x60,0x60,0x60,0x60,0x60,0x60,0x7e,0x00}, // 76 'L'
	{0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00}, // 77 'M'
	{0x66,0x76,0x7e,0x7e,0x6e,0x66,0x66,0x00}, // 78 'N'
	{0x3c,0x66,0x66,0x66,0x66,0x66,0x3c,0x00}, // 79 'O'
	{0x7c,0x66,0x66,0x7c,0x60,0x60,0x60,0x00}, // 80 'P'
	{0x3c,0x66,0x66,0x66,0x6a,0x6c,0x36,0x00}, // 81 'Q'
	{0x7c,0x66,0x66,0x7c,0x6c,0x66,0x66,0x00}, // 82 'R'
	{0x3c,0x66,0x60,0x3c,0x06,0x66,0x3c,0x00}, // 83 'S'
	{0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 84 'T'
	{0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00}, // 85 'U'
	{0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00}, // 86 'V'
	{0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00}, // 87 'W'
	{0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0x00}, // 88 'X'
	{0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00}, // 89 'Y'
	{0x7e,0x06,0x0c,0x18,0x30,0x60,0x7e,0x00}, // 90 'Z'
	{0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00}, // 91 '['
	{0xc0,0x60,0x30,0x18,0x0c,0x06,0x02,0x00}, // 92 '\'
	{0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00}, // 93 ']'
	{0x18,0x3c,0x66,0x00,0x00,0x00,0x00,0x00}, // 94 '^'
	{0x00,0x00,0x00,0x00,0x00,0x00,0xff,0x00}, // 95 '_'
	{0x30,0x18,0x0c,0x00,0x00,0x00,0x00,0x00}, // 96 '`'
	{0x00,0x00,0x3c,0x06,0x3e,0x66,0x3e,0x00}, // 97 'a'
	{0x60,0x60,0x7c,0x66,0x66,0x66,0x7c,0x00}, // 98 'b'
	{0x00,0x00,0x3c,0x66,0x60,0x66,0x3c,0x00}, // 99 'c'
	{0x06,0x06,0x3e,0x66,0x66,0x66,0x3e,0x00}, // 100 'd'
	{0x00,0x00,0x3c,0x66,0x7e,0x60,0x3c,0x00}, // 101 'e'
	{0x1c,0x30,0x78,0x30,0x30,0x30,0x30,0x00}, // 102 'f'
	{0x00,0x00,0x3e,0x66,0x66,0x3e,0x06,0x3c}, // 103 'g'
	{0x60,0x60,0x7c,0x66,0x66,0x66,0x66,0x00}, // 104 'h'
	{0x18,0x00,0x38,0x18,0x18,0x18,0x3c,0x00}, // 105 'i'
	{0x0c,0x00,0x1c,0x0c,0x0c,0x0c,0x6c,0x38}, // 106 'j'
	{0x60,0x60,0x66,0x6c,0x78,0x6c,0x66,0x00}, // 107 'k'
	{0x38,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, // 108 'l'
	{0x00,0x00,0x66,0x7f,0x7f,0x6b,0x63,0x00}, // 109 'm'
	{0x00,0x00,0x7c,0x66,0x66,0x66,0x66,0x00}, // 110 'n'
	{0x00,0x00,0x3c,0x66,0x66,0x66,0x3c,0x00}, // 111 'o'
	{0x00,0x00,0x7c,0x66,0x66,0x7c,0x60,0x60}, // 112 'p'
	{0x00,0x00,0x3e,0x66,0x66,0x3e,0x06,0x06}, // 113 'q'
	{0x00,0x00,0x7c,0x66,0x60,0x60,0x60,0x00}, // 114 'r'
	{0x00,0x00,0x3e,0x60,0x3c,0x06,0x7c,0x00}, // 115 's'
	{0x18,0x18,0x7e,0x18,0x18,0x18,0x0e,0x00}, // 116 't'
	{0x00,0x00,0x66,0x66,0x66,0x66,0x3a,0x00}, // 117 'u'
	{0x00,0x00,0x66,0x66,0x66,0x3c,0x18,0x00}, // 118 'v'
	{0x00,0x00,0x63,0x6b,0x7f,0x3e,0x36,0x00}, // 119 'w'
	{0x00,0x00,0x66,0x3c,0x18,0x3c,0x66,0x00}, // 120 'x'
	{0x00,0x00,0x66,0x66,0x66,0x3e,0x06,0x3c}, // 121 'y'
	{0x00,0x00,0x7e,0x0c,0x18,0x30,0x7e,0x00}, // 122 'z'
	{0x0e,0x18,0x18,0x70,0x18,0x18,0x0e,0x00}, // 123 '{'
	{0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // 124 '|'
	{0x70,0x18,0x18,0x0e,0x18,0x18,0x70,0x00}, // 125 '}'
	{0x76,0xdc,0x00,0x00,0x00,0x00,0x00,0x00}  // 126 '~'
};

static GLuint s_fontTexture = 0;

static void InitFontTexture() {
	if (s_fontTexture != 0) return;

	// 128x128 texture holding 16x8 cells of 8x8 glyphs
	uint16_t* texData = (uint16_t*) calloc(128 * 128, sizeof(uint16_t));
	if (!texData) return;

	for (int i = 0; i < 95; ++i) {
		int cellX = (i % 16) * 8;
		int cellY = (i / 16) * 8;
		for (int row = 0; row < 8; ++row) {
			uint8_t byte = s_font8x8[i][row];
			for (int col = 0; col < 8; ++col) {
				if (byte & (0x80 >> col)) {
					texData[(cellY + row) * 128 + (cellX + col)] = 0xFFFF; // ARGB4444: white opaque
				}
			}
		}
	}

	glGenTextures(1, &s_fontTexture);
	glBindTexture(GL_TEXTURE_2D, s_fontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#ifdef __DREAMCAST__
	glEnable(GL_TEXTURE_TWIDDLE_KOS);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, 128, 128, 0, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, texData);
	glDisable(GL_TEXTURE_TWIDDLE_KOS);
#else
	uint32_t* texData32 = (uint32_t*) malloc(128 * 128 * sizeof(uint32_t));
	if (texData32) {
		for (int p = 0; p < 128 * 128; ++p) {
			texData32[p] = texData[p] ? 0xFFFFFFFF : 0x00000000;
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData32);
		free(texData32);
	}
#endif
	free(texData);
}

void DebugMenu_Init(void) {
	s_menuOpen = false;
	s_selectedItem = 0;
}

void DebugMenu_Toggle(void) {
	s_menuOpen = !s_menuOpen;
}

bool DebugMenu_IsOpen(void) {
	return s_menuOpen;
}

static void ChangeOption(int item, int delta) {
	switch (item) {
	case OPT_SOUND:
		g_debugDisableSound = !g_debugDisableSound;
		if (SoundManager()) {
			SoundManager()->SetVolume(g_debugDisableSound ? 0 : 100);
		}
		break;
	case OPT_MUSIC:
		g_debugDisableMusic = !g_debugDisableMusic;
		if (BackgroundAudioManager()) {
			BackgroundAudioManager()->Enable(!g_debugDisableMusic);
		}
		break;
	case OPT_LIGHTING:
		g_debugShadingMode = (g_debugShadingMode + delta + 4) % 4;
		g_debugDisableLighting = (g_debugShadingMode == 0);
		break;
	case OPT_AMBIENT: {
		int curIdx = 2; // Default 0.38f
		for (int i = 0; i < s_numAmbientLevels; ++i) {
			if (fabsf(s_ambientLevels[i] - g_debugAmbientLevel) < 0.02f) {
				curIdx = i;
				break;
			}
		}
		curIdx = (curIdx + delta + s_numAmbientLevels) % s_numAmbientLevels;
		g_debugAmbientLevel = s_ambientLevels[curIdx];
		break;
	}
	case OPT_QUALITY: {
		int curIdx = 2; // Default Medium (15 Extras)
		for (int i = 0; i < s_numQualityPresets; ++i) {
			if (s_qualityPresets[i].extras == g_debugMaxExtras) {
				curIdx = i;
				break;
			}
		}
		curIdx = (curIdx + delta + s_numQualityPresets) % s_numQualityPresets;
		g_debugMaxExtras = s_qualityPresets[curIdx].extras;

		// Apply immediately to the running engine!
		MxU32 allowed = (g_debugMaxExtras < 0) ? 50 : (MxU32) g_debugMaxExtras;
		LegoAnimationManager::configureLegoAnimationManager(g_debugMaxExtras);
		if (AnimationManager()) {
			AnimationManager()->SetMaxAllowedExtras(allowed);
			AnimationManager()->PurgeExtra(TRUE);
		}
		RealtimeView::SetUserMaxLOD(s_qualityPresets[curIdx].lod);
		break;
	}
	case OPT_DRAWDIST: {
		int curIdx = 2; // Default 1.00x Normal
		for (int i = 0; i < s_numDrawDistPresets; ++i) {
			if (fabsf(s_drawDistPresets[i].scale - g_debugDrawDistanceScale) < 0.05f) {
				curIdx = i;
				break;
			}
		}
		curIdx = (curIdx + delta + s_numDrawDistPresets) % s_numDrawDistPresets;
		g_debugDrawDistanceScale = s_drawDistPresets[curIdx].scale;
		break;
	}
	case OPT_TEXTURES:
		g_debugDisableTextures = !g_debugDisableTextures;
		break;
	case OPT_FPS:
		g_debugShowFps = !g_debugShowFps;
		break;
	case OPT_RESOLUTION:
		g_debugLowResMode = !g_debugLowResMode;
		break;
	default:
		break;
	}
}

void DebugMenu_HandleInput(uint32_t pressed, uint32_t held) {
#ifdef __DREAMCAST__
	if (pressed & CONT_DPAD_UP) {
		s_selectedItem = (s_selectedItem + OPT_COUNT - 1) % OPT_COUNT;
	}
	if (pressed & CONT_DPAD_DOWN) {
		s_selectedItem = (s_selectedItem + 1) % OPT_COUNT;
	}
	if (pressed & CONT_DPAD_LEFT) {
		ChangeOption(s_selectedItem, -1);
	}
	if (pressed & (CONT_DPAD_RIGHT | CONT_A)) {
		ChangeOption(s_selectedItem, 1);
	}
	if (pressed & CONT_B) {
		s_menuOpen = false;
	}

	// Also support analog stick navigation
	static int s_stickRepeat = 0;
	if (abs(g_dreamcastJoyY) > 50) {
		if (s_stickRepeat == 0) {
			if (g_dreamcastJoyY < -50) {
				s_selectedItem = (s_selectedItem + OPT_COUNT - 1) % OPT_COUNT;
			} else {
				s_selectedItem = (s_selectedItem + 1) % OPT_COUNT;
			}
			s_stickRepeat = 12;
		} else {
			s_stickRepeat--;
		}
	} else if (abs(g_dreamcastJoyX) > 50) {
		if (s_stickRepeat == 0) {
			if (g_dreamcastJoyX < -50) {
				ChangeOption(s_selectedItem, -1);
			} else {
				ChangeOption(s_selectedItem, 1);
			}
			s_stickRepeat = 14;
		} else {
			s_stickRepeat--;
		}
	} else {
		s_stickRepeat = 0;
	}
#else
	(void)held;
	// PC / generic handling if needed
	if (pressed & 1) s_selectedItem = (s_selectedItem + OPT_COUNT - 1) % OPT_COUNT;
	if (pressed & 2) s_selectedItem = (s_selectedItem + 1) % OPT_COUNT;
	if (pressed & 4) ChangeOption(s_selectedItem, -1);
	if (pressed & 8) ChangeOption(s_selectedItem, 1);
	if (pressed & 16) s_menuOpen = false;
#endif
}

struct MenuVertex {
	float x, y, z;
	float u, v;
};

#define MAX_MENU_VERTS 512
static MenuVertex s_menuVerts[MAX_MENU_VERTS];
static int s_menuVertCount = 0;

static void DrawUntexturedQuad(float x1, float y1, float x2, float y2) {
	MenuVertex q[6] = {
		{ x1, y1, 0.0f, 0.0f, 0.0f },
		{ x2, y1, 0.0f, 0.0f, 0.0f },
		{ x1, y2, 0.0f, 0.0f, 0.0f },
		{ x2, y1, 0.0f, 0.0f, 0.0f },
		{ x2, y2, 0.0f, 0.0f, 0.0f },
		{ x1, y2, 0.0f, 0.0f, 0.0f }
	};
	glVertexPointer(3, GL_FLOAT, sizeof(MenuVertex), &q[0].x);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void EmitChar(char c, float x, float y, float w, float h) {
	if (c < 32 || c > 126) return;
	if (s_menuVertCount + 6 > MAX_MENU_VERTS) return;

	int idx = c - 32;
	float u1 = (float)((idx % 16) * 8) / 128.0f;
	float v1 = (float)((idx / 16) * 8) / 128.0f;
	float u2 = u1 + (8.0f / 128.0f);
	float v2 = v1 + (8.0f / 128.0f);

	// Tri 1: TL, TR, BL
	s_menuVerts[s_menuVertCount + 0].x = x;
	s_menuVerts[s_menuVertCount + 0].y = y;
	s_menuVerts[s_menuVertCount + 0].z = 0.0f;
	s_menuVerts[s_menuVertCount + 0].u = u1;
	s_menuVerts[s_menuVertCount + 0].v = v1;

	s_menuVerts[s_menuVertCount + 1].x = x + w;
	s_menuVerts[s_menuVertCount + 1].y = y;
	s_menuVerts[s_menuVertCount + 1].z = 0.0f;
	s_menuVerts[s_menuVertCount + 1].u = u2;
	s_menuVerts[s_menuVertCount + 1].v = v1;

	s_menuVerts[s_menuVertCount + 2].x = x;
	s_menuVerts[s_menuVertCount + 2].y = y + h;
	s_menuVerts[s_menuVertCount + 2].z = 0.0f;
	s_menuVerts[s_menuVertCount + 2].u = u1;
	s_menuVerts[s_menuVertCount + 2].v = v2;

	// Tri 2: TR, BR, BL
	s_menuVerts[s_menuVertCount + 3].x = x + w;
	s_menuVerts[s_menuVertCount + 3].y = y;
	s_menuVerts[s_menuVertCount + 3].z = 0.0f;
	s_menuVerts[s_menuVertCount + 3].u = u2;
	s_menuVerts[s_menuVertCount + 3].v = v1;

	s_menuVerts[s_menuVertCount + 4].x = x + w;
	s_menuVerts[s_menuVertCount + 4].y = y + h;
	s_menuVerts[s_menuVertCount + 4].z = 0.0f;
	s_menuVerts[s_menuVertCount + 4].u = u2;
	s_menuVerts[s_menuVertCount + 4].v = v2;

	s_menuVerts[s_menuVertCount + 5].x = x;
	s_menuVerts[s_menuVertCount + 5].y = y + h;
	s_menuVerts[s_menuVertCount + 5].z = 0.0f;
	s_menuVerts[s_menuVertCount + 5].u = u1;
	s_menuVerts[s_menuVertCount + 5].v = v2;

	s_menuVertCount += 6;
}

static void DrawString(const char* str, float x, float y, float charW, float charH) {
	s_menuVertCount = 0;
	while (*str) {
		if (*str == ' ') {
			x += charW;
			str++;
			continue;
		}
		EmitChar(*str, x, y, charW, charH);
		x += charW;
		str++;
	}
	if (s_menuVertCount > 0) {
		glDrawArrays(GL_TRIANGLES, 0, s_menuVertCount);
	}
}

void DebugMenu_Render(void) {
	if (!s_menuOpen) return;

	InitFontTexture();
	if (s_fontTexture == 0) return;

#ifdef __DREAMCAST__
	extern int g_targetWidth;
	extern int g_targetHeight;
	glViewport(0, 0, g_targetWidth, g_targetHeight);
#else
	glViewport(0, 0, 640, 480);
#endif
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	// 1. Draw backdrop quad and solid border
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// Dark semi-transparent box
	glColor4f(0.04f, 0.06f, 0.12f, 0.88f);
	DrawUntexturedQuad(50.0f, 40.0f, 590.0f, 440.0f);

	// Border (solid 2px outline using quads)
	glColor4f(0.2f, 0.6f, 1.0f, 0.9f);
	DrawUntexturedQuad(50.0f,  40.0f, 590.0f,  42.0f); // Top
	DrawUntexturedQuad(50.0f, 438.0f, 590.0f, 440.0f); // Bottom
	DrawUntexturedQuad(50.0f,  40.0f,  52.0f, 440.0f); // Left
	DrawUntexturedQuad(588.0f,  40.0f, 590.0f, 440.0f); // Right

	// 2. Draw text using font texture and client arrays
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_fontTexture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(MenuVertex), &s_menuVerts[0].x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(MenuVertex), &s_menuVerts[0].u);

	// Title
	glColor4f(0.3f, 0.9f, 1.0f, 1.0f);
	DrawString("LEGO ISLAND DREAMCAST DEBUG MENU", 80.0f, 60.0f, 14.0f, 18.0f);
	DrawString("================================", 80.0f, 82.0f, 14.0f, 14.0f);

	// Menu items
	float itemY = 104.0f;
	const float itemStep = 27.0f;
	const float charW = 11.0f;
	const float charH = 16.0f;

	char lineBuf[80];

	for (int i = 0; i < OPT_COUNT; ++i) {
		bool isSelected = (i == s_selectedItem);
		if (isSelected) {
			glColor4f(1.0f, 1.0f, 0.2f, 1.0f); // Bright yellow highlight
		} else {
			glColor4f(0.85f, 0.85f, 0.85f, 1.0f); // Light gray
		}

		const char* cursor = isSelected ? "> " : "  ";

		switch (i) {
		case OPT_SOUND:
			snprintf(lineBuf, sizeof(lineBuf), "%sSound:        [%s]",
				cursor, g_debugDisableSound ? "DISABLED" : "ENABLED");
			break;
		case OPT_MUSIC:
			snprintf(lineBuf, sizeof(lineBuf), "%sMusic:        [%s]",
				cursor, g_debugDisableMusic ? "DISABLED" : "ENABLED");
			break;
		case OPT_LIGHTING: {
			const char* lightDesc = (g_debugShadingMode == 0) ? "0: Off/Fullbright" :
			                        ((g_debugShadingMode == 1) ? "1: Fast Directional" :
			                        ((g_debugShadingMode == 2) ? "2: Vertex Lambert" : "3: Fixed-Func HW"));
			snprintf(lineBuf, sizeof(lineBuf), "%sLighting:     [%s]", cursor, lightDesc);
			break;
		}
		case OPT_AMBIENT:
			snprintf(lineBuf, sizeof(lineBuf), "%sAmbient Floor:[%.2f]", cursor, g_debugAmbientLevel);
			break;
		case OPT_QUALITY: {
			const char* qDesc = "Medium (15 Extras, LOD 3.6)";
			for (int q = 0; q < s_numQualityPresets; ++q) {
				if (s_qualityPresets[q].extras == g_debugMaxExtras) {
					qDesc = s_qualityPresets[q].name;
					break;
				}
			}
			snprintf(lineBuf, sizeof(lineBuf), "%sModel Quality:[%s]", cursor, qDesc);
			break;
		}
		case OPT_DRAWDIST: {
			const char* ddDesc = "Normal (1.00x)";
			for (int d = 0; d < s_numDrawDistPresets; ++d) {
				if (fabsf(s_drawDistPresets[d].scale - g_debugDrawDistanceScale) < 0.05f) {
					ddDesc = s_drawDistPresets[d].name;
					break;
				}
			}
			snprintf(lineBuf, sizeof(lineBuf), "%sDraw Distance:[%s]", cursor, ddDesc);
			break;
		}
		case OPT_TEXTURES:
			snprintf(lineBuf, sizeof(lineBuf), "%sTextures:     [%s]",
				cursor, g_debugDisableTextures ? "DISABLED" : "ENABLED");
			break;
		case OPT_FPS:
			snprintf(lineBuf, sizeof(lineBuf), "%sShow FPS:     [%s]",
				cursor, g_debugShowFps ? "ENABLED" : "DISABLED");
			break;
		case OPT_RESOLUTION:
			snprintf(lineBuf, sizeof(lineBuf), "%sResolution:   [%s]",
				cursor, g_debugLowResMode ? "320x240 (on boot)" : "640x480 (Native)");
			break;
		default:
			lineBuf[0] = '\0';
			break;
		}

		DrawString(lineBuf, 70.0f, itemY, charW, charH);
		itemY += itemStep;
	}

	// Footer instructions
	glColor4f(0.5f, 0.7f, 0.9f, 0.9f);
	DrawString("----------------------------------------", 70.0f, 375.0f, 12.0f, 12.0f);
	DrawString("DPAD: Select/Adjust | A: Toggle | START/B: Exit", 70.0f, 398.0f, 10.0f, 14.0f);

	// Restore state
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void DebugMenu_RenderFpsOverlay(void) {
	if (!g_debugShowFps) return;

	// Calculate moving average FPS
	static uint64_t s_lastTimeUs = 0;
	static float s_smoothFps = 0.0f;
	static int s_fpsFrameCount = 0;
	static uint64_t s_fpsAccumTimeUs = 0;

#ifdef __DREAMCAST__
	uint64_t now = timer_us_gettime64();
#else
	uint64_t now = (uint64_t) SDL_GetTicks() * 1000ULL;
#endif

	if (s_lastTimeUs != 0) {
		uint64_t deltaUs = now - s_lastTimeUs;
		s_fpsAccumTimeUs += deltaUs;
		s_fpsFrameCount++;
		if (s_fpsAccumTimeUs >= 250000ULL) { // Update 4 times per second
			s_smoothFps = (float) s_fpsFrameCount * 1000000.0f / (float) s_fpsAccumTimeUs;
			s_fpsFrameCount = 0;
			s_fpsAccumTimeUs = 0;
		}
	}
	s_lastTimeUs = now;

	if (s_smoothFps <= 0.0f) return;

	InitFontTexture();

#ifdef __DREAMCAST__
	extern int g_targetWidth;
	extern int g_targetHeight;
	glViewport(0, 0, g_targetWidth, g_targetHeight);
#else
	glViewport(0, 0, 640, 480);
#endif
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, 640.0, 480.0, 0.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	char fpsStr[32];
	snprintf(fpsStr, sizeof(fpsStr), "FPS: %.1f", s_smoothFps);

	// Small dark backing rectangle for legibility
	float x = 524.0f;
	float y = 14.0f;
	glColor4f(0.0f, 0.0f, 0.0f, 0.70f);
	DrawUntexturedQuad(x - 6.0f, y - 3.0f, 626.0f, y + 17.0f);

	// Colored text based on framerate
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s_fontTexture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(MenuVertex), &s_menuVerts[0].x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(MenuVertex), &s_menuVerts[0].u);

	if (s_smoothFps >= 35.0f) {
		glColor4f(0.2f, 1.0f, 0.4f, 1.0f); // Bright green
	} else if (s_smoothFps >= 20.0f) {
		glColor4f(1.0f, 1.0f, 0.2f, 1.0f); // Yellow
	} else {
		glColor4f(1.0f, 0.3f, 0.2f, 1.0f); // Red
	}

	DrawString(fpsStr, x, y, 9.0f, 14.0f);

	// Restore GL state
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	if (g_debugLowResMode) {
		glViewport(0, 0, 320, 240);
	}
}
