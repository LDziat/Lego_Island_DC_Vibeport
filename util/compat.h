#ifndef COMPAT_H
#define COMPAT_H

// Various macros to enable compiling with other/newer compilers.
#include <string.h>
#include <math.h>
#include <stdlib.h>

#if defined(__MINGW32__) || (defined(_MSC_VER) && _MSC_VER >= 1100) || !defined(_WIN32)
#define COMPAT_MODE
#endif

#ifndef MINIWIN
#define D3DCOLOR_NONE 0
#define D3DRMMAP_NONE 0
#define PC_NONE 0
#define DDBLT_NONE 0
#define D3DRMRENDERMODE DWORD
#define DDSCapsFlags DWORD
#define DDBitDepths DWORD
#endif

// SDL will not put the message box on the main thread by default.
// See: https://github.com/libsdl-org/SDL/issues/12943
#ifdef __EMSCRIPTEN__
#define Any_ShowSimpleMessageBox Emscripten_ShowSimpleMessageBox
#elif defined(__vita__)
#define Any_ShowSimpleMessageBox Vita_ShowSimpleMessageBox
#elif defined(__DREAMCAST__)
#include <stdio.h>
#define SDL_MESSAGEBOX_ERROR 0
inline void Any_ShowSimpleMessageBox(int flags, const char* title, const char* message, void* window) {
    printf("%s: %s\n", title, message);
}
#else
#define Any_ShowSimpleMessageBox SDL_ShowSimpleMessageBox
#endif

// Disable "identifier was truncated to '255' characters" warning.
// Impossible to avoid this if using STL map or set.
// This removes most (but not all) occurrences of the warning.
#pragma warning(disable : 4786)

#define MSVC420_VERSION 1020

// We use `override` so newer compilers can tell us our vtables are valid,
// however this keyword was added in C++11, so we define it as empty for
// compatibility with older compilers.
#if __cplusplus < 201103L
#define override
#define static_assert(expr, msg)
#endif

#include <SDL/SDL_version.h>
#if !defined(SDL_VERSION_ATLEAST) || !SDL_VERSION_ATLEAST(3,0,0)
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL_rwops.h>
typedef SDL_RWops SDL_IOStream;
#define SDL_IOFromFile SDL_RWFromFile
#define SDL_IOFromMem SDL_RWFromMem
#define SDL_ReadIO(ctx, ptr, size) SDL_RWread(ctx, ptr, 1, size)
#define SDL_WriteIO(ctx, ptr, size) SDL_RWwrite(ctx, ptr, 1, size)
#define SDL_SeekIO SDL_RWseek
#define SDL_TellIO SDL_RWtell
#define SDL_CloseIO SDL_RWclose
#define SDL_rand(x) (rand() % (x))
#define SDL_randf() ((float)rand() / (float)RAND_MAX)
#define SDL_Semaphore SDL_semaphore
#define SDL_WaitSemaphore SDL_SemWait
#define SDL_SignalSemaphore SDL_SemPost
#define SDL_zero(x) memset(&(x), 0, sizeof((x)))
#define SDL_zerop(x) memset((x), 0, sizeof(*(x)))
#define SDL_zeroa(x) memset((x), 0, sizeof((x)))

#define SDL_IO_STATUS_ERROR -1
#define SDL_GetIOStatus(ctx) 0

// SDL3 filesystem API shims
#include <sys/stat.h>
#include <stdio.h>

typedef enum {
	SDL_PATHTYPE_NONE,
	SDL_PATHTYPE_FILE,
	SDL_PATHTYPE_DIRECTORY,
	SDL_PATHTYPE_OTHER
} SDL_PathType;

typedef struct SDL_PathInfo {
	SDL_PathType type;
	unsigned long long size;
	unsigned long long create_time;
	unsigned long long modify_time;
	unsigned long long access_time;
} SDL_PathInfo;

static inline int SDL_GetPathInfo(const char* path, SDL_PathInfo* info) {
	struct stat st;
	if (stat(path, &st) != 0) {
		return 0;
	}
	if (info) {
		if (S_ISREG(st.st_mode)) info->type = SDL_PATHTYPE_FILE;
		else if (S_ISDIR(st.st_mode)) info->type = SDL_PATHTYPE_DIRECTORY;
		else info->type = SDL_PATHTYPE_OTHER;
		info->size = (unsigned long long)st.st_size;
		info->modify_time = (unsigned long long)st.st_mtime;
		info->create_time = 0;
		info->access_time = (unsigned long long)st.st_atime;
	}
	return 1;
}

#define SDL_RemovePath(path) (remove(path) == 0)
#define SDL_RenamePath(oldpath, newpath) (rename(oldpath, newpath) == 0)

// SDL3 logging shims
#ifndef SDL_Log
#define SDL_Log printf
#endif
#ifndef SDL_LogInfo
#define SDL_LogInfo(cat, ...) printf(__VA_ARGS__)
#endif
#endif
#ifndef SDL_LogWarn
#define SDL_LogWarn(cat, ...) printf(__VA_ARGS__)
#endif

// SDL3 key constants use uppercase (SDLK_A), SDL 1.2 uses lowercase (SDLK_a)
#define SDLK_A SDLK_a
#define SDLK_B SDLK_b
#define SDLK_C SDLK_c
#define SDLK_D SDLK_d
#define SDLK_E SDLK_e
#define SDLK_F SDLK_f
#define SDLK_G SDLK_g
#define SDLK_H SDLK_h
#define SDLK_I SDLK_i
#define SDLK_J SDLK_j
#define SDLK_K SDLK_k
#define SDLK_L SDLK_l
#define SDLK_M SDLK_m
#define SDLK_N SDLK_n
#define SDLK_O SDLK_o
#define SDLK_P SDLK_p
#define SDLK_Q SDLK_q
#define SDLK_R SDLK_r
#define SDLK_S SDLK_s
#define SDLK_T SDLK_t
#define SDLK_U SDLK_u
#define SDLK_V SDLK_v
#define SDLK_W SDLK_w
#define SDLK_X SDLK_x
#define SDLK_Y SDLK_y
#define SDLK_Z SDLK_z


#if !defined(SDL_VERSION_ATLEAST) || !SDL_VERSION_ATLEAST(2,0,0)
#define SDL_SCANCODE_KP_8 SDLK_KP8
#define SDL_SCANCODE_KP_2 SDLK_KP2
#define SDL_SCANCODE_KP_4 SDLK_KP4
#define SDL_SCANCODE_KP_6 SDLK_KP6
#define SDL_SCANCODE_UP SDLK_UP
#define SDL_SCANCODE_DOWN SDLK_DOWN
#define SDL_SCANCODE_LEFT SDLK_LEFT
#define SDL_SCANCODE_RIGHT SDLK_RIGHT
#define SDL_SCANCODE_W SDLK_w
#define SDL_SCANCODE_A SDLK_a
#define SDL_SCANCODE_S SDLK_s
#define SDL_SCANCODE_D SDLK_d
#define SDL_SCANCODE_LCTRL SDLK_LCTRL
#define SDL_SCANCODE_RCTRL SDLK_RCTRL

#define SDL_GetKeyboardState(x) ((const bool*)SDL_GetKeyState(x))
#define SDL_clamp(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define SDL_strtok_r strtok_r
#define SDL_floorf floorf
#define SDL_fabsf fabsf
#define SDL_fabs fabs
#define SDL_rand_bits() ((rand() << 16) | rand())
#define SDL_cosf cosf
#define SDL_sinf sinf
#define SDL_atan2f atan2f
#define SDL_sqrtf sqrtf
#ifndef SDL_PI_F
#define SDL_PI_F 3.14159265358979323846f
#endif
#define SDL_EVENT_MOUSE_BUTTON_DOWN SDL_MOUSEBUTTONDOWN
#define SDL_EVENT_MOUSE_BUTTON_UP SDL_MOUSEBUTTONUP
#define SDL_EVENT_MOUSE_MOTION SDL_MOUSEMOTION
#define SDL_EVENT_KEY_DOWN SDL_KEYDOWN
#define SDL_EVENT_KEY_UP SDL_KEYUP

#define SDL_SetHint(name, value) (void)0
#define SDL_HINT_MOUSE_TOUCH_EVENTS ""
#define SDL_HINT_TOUCH_MOUSE_EVENTS ""

typedef Uint32 SDL_MouseButtonFlags;

// Gamepad/Haptic/Touch stubs
#define SDL_OpenHapticFromMouse() NULL
#define SDL_InitHapticRumble(h) 0
#define SDL_CloseHaptic(h) (void)0
#define SDL_OpenGamepad(x) NULL
#define SDL_GetGamepadJoystick(x) NULL
#define SDL_OpenHapticFromJoystick(x) NULL
#define SDL_CloseGamepad(x) (void)0
#define SDL_GetGamepadAxis(x, y) 0
#define SDL_RumbleGamepad(j, low, high, duration) 0
#define SDL_PlayHapticRumble(h, strength, length) 0
#define SDL_GAMEPAD_AXIS_LEFTX 0
#define SDL_GAMEPAD_AXIS_LEFTY 1
#define SDL_GAMEPAD_AXIS_RIGHTX 2
#define SDL_GAMEPAD_AXIS_RIGHTY 3
#define SDL_GAMEPAD_AXIS_RIGHT_TRIGGER 5
#define SDL_GAMEPAD_BUTTON_SOUTH 0
#define SDL_GAMEPAD_BUTTON_START 1
#define SDL_GAMEPAD_BUTTON_DPAD_UP 2
#define SDL_GAMEPAD_BUTTON_DPAD_DOWN 3
#define SDL_GAMEPAD_BUTTON_DPAD_LEFT 4
#define SDL_GAMEPAD_BUTTON_DPAD_RIGHT 5
#define SDL_GAMEPAD_BUTTON_EAST 6
#define SDL_JOYSTICK_AXIS_MAX 32767.0f
#define SDL_JOYSTICK_AXIS_MIN -32768.0f
#define gaxis jaxis
#define gbutton jbutton
#define gdevice jaxis
#define jdevice jaxis
#define kdevice key
#define mdevice motion
#define SDL_GetHapticID(x) 0

// Cursor shims
#define SDL_HideCursor() SDL_ShowCursor(SDL_DISABLE)
#define SDL_CreateSystemCursor(x) NULL
#define SDL_SYSTEM_CURSOR_DEFAULT 0
#define SDL_SYSTEM_CURSOR_WAIT 1
#define SDL_SYSTEM_CURSOR_NOT_ALLOWED 2
#define SDL_PRIx32 "x"

// SDL3 Window Property shims
typedef int SDL_PropertiesID;
#define SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER ""
#define SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER ""
#define SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN ""
#define SDL_PROP_WINDOW_CREATE_TITLE_STRING ""
#define SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN ""
#define SDL_SetNumberProperty(props, name, value) (void)0
#define SDL_SetBooleanProperty(props, name, value) (void)0
#define SDL_SetStringProperty(props, name, value) (void)0

#define SDL_TouchFingerEvent SDL_Event
#define SDL_EVENT_FINGER_UP 0x1000
#define SDL_EVENT_FINGER_DOWN 0x1001
#define SDL_EVENT_FINGER_MOTION 0x1002
#define SDL_EVENT_FINGER_CANCELED 0x1003
#define SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED 0x1004
#define SDL_EVENT_WINDOW_FOCUS_GAINED 0x1005
#define SDL_EVENT_WINDOW_FOCUS_LOST 0x1006
#define SDL_EVENT_WINDOW_CLOSE_REQUESTED 0x1007
#define SDL_EVENT_KEYBOARD_ADDED 0x1008
#define SDL_EVENT_KEYBOARD_REMOVED 0x1009
#define SDL_EVENT_MOUSE_ADDED 0x100A
#define SDL_EVENT_MOUSE_REMOVED 0x100B
#define SDL_EVENT_GAMEPAD_ADDED 0x100C
#define SDL_EVENT_GAMEPAD_REMOVED 0x100D
#define SDL_EVENT_GAMEPAD_BUTTON_DOWN 0x100E
#define SDL_EVENT_GAMEPAD_BUTTON_UP 0x100F
#define SDL_EVENT_GAMEPAD_AXIS_MOTION 0x1010
#define SDL_EVENT_QUIT SDL_QUIT

typedef int SDL_Keycode;
typedef int SDL_KeyboardID;
typedef int SDL_MouseID;
typedef int SDL_JoystickID;
typedef int SDL_TouchID;
typedef int SDL_FingerID;
typedef int SDL_HapticID;
typedef struct SDL_Gamepad SDL_Gamepad;
typedef struct SDL_Haptic SDL_Haptic;
typedef struct SDL_FPoint { float x; float y; } SDL_FPoint;
typedef struct SDL_Point { int x; int y; } SDL_Point;
#endif



// Additional SDL3 shims
static inline char* SDL_GetBasePath() { char* p = (char*)SDL_malloc(4); strcpy(p, "/cd"); return p; }
static inline char* SDL_GetPrefPath(const char* a, const char* b) { char* p = (char*)SDL_malloc(6); strcpy(p, "/ram/"); return p; }

#define SDL_CreateProperties() 0
#define SDL_GetWindowProperties(w) 0
#define SDL_SetPointerProperty(p, n, v) (void)0
#define SDL_DestroyProperties(p) (void)0
#define SDL_WarpMouseInWindow(w, x, y) (void)0
#define SDL_GetClosestFullscreenDisplayMode(a,b,c,d,e,f) 0
#define SDL_SetWindowFullscreenMode(w, m) (void)0
#define SDL_DisplayMode int
#define SDL_DisplayID int
#define SDL_GetDisplayForWindow(w) 0
#define SDL_NS_TO_MS(x) ((x) / 1000000)
#define SDL_LoadBMP_IO(stream, close) SDL_LoadBMP_RW(stream, close)
#define SDL_SetWindowIcon(w, icon) (void)0

#ifndef SDL_DestroySurface
#define SDL_DestroySurface(s) SDL_FreeSurface(s)
#endif
#ifndef SDL_CreateSurface
#define SDL_CreateSurface(w, h, fmt) SDL_CreateRGBSurface(0, w, h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000)
#endif

#ifndef SDL_PIXELFORMAT_INDEX8
#define SDL_PIXELFORMAT_INDEX8 1
#endif
#define SDL_PixelFormatDetails SDL_PixelFormat
#define SDL_GetPixelFormatDetails(x) (x)
#define bits_per_pixel BitsPerPixel

#define SDL_FillSurfaceRect(s, r, c) SDL_FillRect(s, r, c)
#define SDL_GetSurfaceColorKey(s, k) ((*(k) = (s)->format->colorkey), ((s)->flags & SDL_SRCCOLORKEY))
#define SDL_SurfaceHasColorKey(s) ((s)->flags & SDL_SRCCOLORKEY)
#define SDL_SetSurfaceColorKey(s, flag, key) SDL_SetColorKey(s, flag ? SDL_SRCCOLORKEY : 0, key)
#define SDL_GetSurfacePalette(s) ((s)->format->palette)
#define SDL_SetWindowRelativeMouseMode(w, b) SDL_WM_GrabInput(b ? SDL_GRAB_ON : SDL_GRAB_OFF)
#define SDL_DestroyWindow(w) (void)0
#define SDL_LOG_CATEGORY_APPLICATION 0
#define SDL_LogError(cat, ...) printf(__VA_ARGS__)
#define SDL_GetCurrentVideoDriver() "KallistiOS"
#define SDL_GL_MakeCurrent(w, c) 1
#ifdef __DREAMCAST__
extern "C" void glKosSwapBuffers();
#define SDL_GL_SwapWindow(w) glKosSwapBuffers()
#else
#define SDL_GL_SwapWindow(w) SDL_GL_SwapBuffers()
#endif
#define SDL_GL_DestroyContext(c) (void)0
#define SDL_GLContext void*
#define SDL_GL_ResetAttributes() (void)0
#define SDL_DisplayID int
#define SDL_GetPrimaryDisplay() 1
#define SDL_GetFullscreenDisplayModes(d, c) NULL
#define SDL_GetCurrentDisplayMode(d) NULL
#define SDL_GetWindowSizeInPixels(w, x, y) (*(x)=640, *(y)=480, 1)
#define SDL_SetWindowFullscreen(w, f) 1

#define SDL_GL_CONTEXT_PROFILE_MASK (SDL_GLattr)0
#define SDL_GL_CONTEXT_PROFILE_COMPATIBILITY 0
#define SDL_GL_CONTEXT_MAJOR_VERSION (SDL_GLattr)0
#define SDL_GL_CONTEXT_MINOR_VERSION (SDL_GLattr)0
#define SDL_GL_CreateContext(w) ((void*)1)
#define SDL_GL_ExtensionSupported(x) 0



#ifndef SDL_PIXELFORMAT_RGBA32
#define SDL_PIXELFORMAT_RGBA32 32
#endif
#ifndef SDL_SCALEMODE_NEAREST
#define SDL_SCALEMODE_NEAREST 0
#endif
#ifndef SDL_BlitSurfaceScaled
#define SDL_BlitSurfaceScaled(src, srcrect, dst, dstrect, mode) SDL_SoftStretch(src, srcrect, dst, dstrect)
#endif

extern bool g_debugDisableTextures;
extern bool g_debugDisableSound;
extern bool g_debugDisableMusic;
extern bool g_debugFlatShaded;
extern bool g_debugTwiddle3D;
extern bool g_debugStrips;
extern int g_debugMaxExtras;
extern bool g_debugDisableLighting;
extern int g_debugShadingMode;
extern float g_debugAmbientLevel;
extern float g_debugSunDirX;
extern float g_debugSunDirY;
extern float g_debugSunDirZ;
extern bool g_debugLowResMode;
extern float g_debugDrawDistanceScale;
extern bool g_debugShowFps;
extern bool g_debugMenuOpen;
extern bool g_leftTriggerHeld;
extern int g_dreamcastJoyX;
extern int g_dreamcastJoyY;
void InitDebugFlags();

#endif // COMPAT_H

