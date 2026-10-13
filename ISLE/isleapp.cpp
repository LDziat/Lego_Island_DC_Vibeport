#define INITGUID

#include "isleapp.h"

#include "3dmanager/lego3dmanager.h"
#include "decomp.h"
#include "legoanimationmanager.h"
#include "legobuildingmanager.h"
#include "legogamestate.h"
#include "legoinputmanager.h"
#include "legomain.h"
#include "legomodelpresenter.h"
#include "legopartpresenter.h"
#include "legoutils.h"
#include "legovideomanager.h"
#include "legoworldpresenter.h"
#include "misc.h"
#include "mxbackgroundaudiomanager.h"
#include "mxdirectx/mxdirect3d.h"
#include "mxdsaction.h"
#include "mxmisc.h"
#include "mxomnicreateflags.h"
#include "mxomnicreateparam.h"
#include "mxstreamer.h"
#include "mxticklemanager.h"
#include "mxtimer.h"
#include "mxtransitionmanager.h"
#include "mxutilities.h"
#include "mxvariabletable.h"
#include "res/arrow_bmp.h"
#include "res/busy_bmp.h"
#include "res/isle_bmp.h"
#include "res/no_bmp.h"
#include "res/resource.h"
#include "roi/legoroi.h"
#include "tgl/d3drm/impl.h"
#include "viewmanager/viewmanager.h"
#include "dcprof.h"

#include <array>
#include <extensions/multiplayer.h>
#include <extensions/thirdpersoncamera.h>
#include <miniwin/miniwindevice.h>
#include <type_traits>
#include <vec.h>

#define SDL_MAIN_USE_CALLBACKS
#ifdef _arch_dreamcast
#include <SDL/SDL.h>
#else
#ifdef _arch_dreamcast
#include <SDL/SDL.h>
#else
#ifdef _arch_dreamcast
#include <SDL/SDL.h>
#else
#include <SDL3/SDL.h>
#endif
#endif
#endif
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
#include <SDL3/SDL_main.h>
#endif
#endif
#endif
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
#include <SDL3/SDL_revision.h>
#endif
#endif
#endif
#include <errno.h>
#include <iniparser.h>
#include <stdlib.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include "emscripten/config.h"
#include "emscripten/events.h"
#include "emscripten/filesystem.h"
#include "emscripten/haptic.h"
#include "emscripten/messagebox.h"
#include "emscripten/window.h"
#endif

#ifdef __3DS__
#include "3ds/apthooks.h"
#include "3ds/config.h"
#endif

#ifdef __SWITCH__
#include "switch/config.h"

#include <switch.h>
#endif

#ifdef WINDOWS_STORE
#include "xbox_one_series/config.h"
#endif

#ifdef IOS
#include "ios/config.h"
#endif

#ifdef ANDROID
#include "android/config.h"
#endif

#ifdef __vita__
#include "vita/config.h"
#include "vita/messagebox.h"

#include <psp2/appmgr.h>
#include <psp2/kernel/clib.h>
#endif

#ifdef __DREAMCAST__
#include <malloc.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include "debugmenu.h"
extern "C" void thd_pass(void);
extern "C" void glKosInitConfig(void* config);
extern "C" void glKosInitEx(void* config);
struct GLdcConfig {
	unsigned char autosort_enabled;
	unsigned char fsaa_enabled;
	unsigned int internal_palette_format;
	unsigned int initial_op_capacity;
	unsigned int initial_tr_capacity;
	unsigned int initial_pt_capacity;
	unsigned int initial_immediate_capacity;
	unsigned char texture_twiddle;
};

static void PrintHeapStats(const char* label)
{
	struct mallinfo mi = mallinfo();
	printf("[RAM] %s: arena=%d, uordblks=%d, fordblks=%d\n",
		label, mi.arena, mi.uordblks, mi.fordblks);
}
#endif
DECOMP_SIZE_ASSERT(IsleApp, 0x8c)

// GLOBAL: ISLE 0x410030
IsleApp* g_isle = NULL;



// GLOBAL: ISLE 0x410034
MxU8 g_mousedown = FALSE;

// GLOBAL: ISLE 0x410038
MxU8 g_mousemoved = FALSE;

// GLOBAL: ISLE 0x41003c
MxS32 g_closed = FALSE;

// GLOBAL: ISLE 0x410050
MxS32 g_rmDisabled = FALSE;

// GLOBAL: ISLE 0x410054
MxS32 g_waitingForTargetDepth = TRUE;

// GLOBAL: ISLE 0x410058
MxS32 g_targetWidth = 640;

// GLOBAL: ISLE 0x41005c
MxS32 g_targetHeight = 480;

// GLOBAL: ISLE 0x410060
MxS32 g_targetDepth = 16;

// GLOBAL: ISLE 0x410064
MxS32 g_reqEnableRMDevice = FALSE;

MxFloat g_lastJoystickMouseX = 0;
MxFloat g_lastJoystickMouseY = 0;
MxFloat g_lastMouseX = 320;
MxFloat g_lastMouseY = 240;
MxBool g_mouseWarped = FALSE;

bool g_dpadUp = false;
bool g_dpadDown = false;
bool g_dpadLeft = false;
bool g_dpadRight = false;

// STRING: ISLE 0x4101dc
#define WINDOW_TITLE "LEGO®"

#ifdef __DREAMCAST__
SDL_Surface* window;
#else
SDL_Window* window;
#endif

extern const char* g_files[46];

// FUNCTION: ISLE 0x401000
IsleApp::IsleApp()
{
	m_hdPath = NULL;
	m_cdPath = NULL;
	m_deviceId = NULL;
	m_savePath = NULL;
	m_fullScreen = TRUE;
	m_flipSurfaces = FALSE;
	m_backBuffersInVram = TRUE;
	m_using8bit = FALSE;
	m_using16bit = TRUE;
	m_hasLightSupport = FALSE;
#if defined(__DJGPP__) || defined(__DREAMCAST__)
	m_drawCursor = TRUE;
#else
	m_drawCursor = FALSE;
#endif
	m_use3dSound = TRUE;
	m_useMusic = TRUE;
	m_wideViewAngle = TRUE;
#ifdef __DJGPP__
	m_islandQuality = 1;
#elif defined(__DREAMCAST__)
	m_islandQuality = 0;
#else
	m_islandQuality = 2;
#endif
#ifdef __DREAMCAST__
	m_islandTexture = 0;
#else
	m_islandTexture = 1;
#endif
	m_gameStarted = FALSE;
	m_frameDelta = 10;
	m_windowActive = TRUE;

#ifdef COMPAT_MODE
	{
		MxRect32 r(0, 0, 639, 479);
		MxVideoParamFlags flags;
		m_videoParam = MxVideoParam(r, NULL, 1, flags);
	}
#else
	m_videoParam = MxVideoParam(MxRect32(0, 0, 639, 479), NULL, 1, MxVideoParamFlags());
#endif
	m_videoParam.Flags().Set16Bit(MxDirectDraw::GetPrimaryBitDepth() == 16);

	m_windowHandle = NULL;
	m_cursorArrow = NULL;
	m_cursorBusy = NULL;
	m_cursorNo = NULL;
	m_cursorCurrent = NULL;
	m_cursorArrowBitmap = NULL;
	m_cursorBusyBitmap = NULL;
	m_cursorNoBitmap = NULL;
	m_cursorCurrentBitmap = NULL;

	LegoOmni::CreateInstance();

	m_mediaPath = NULL;
	m_iniPath = NULL;
	m_maxLod = RealtimeView::GetUserMaxLOD();
#ifdef __DJGPP__
	m_maxLod = 1.0f;
#elif defined(__DREAMCAST__)
	m_maxLod = 0.0f;
#endif
	m_maxAllowedExtras = m_islandQuality <= 1 ? 10 : 20;
	m_transitionType = MxTransitionManager::e_mosaic;
	m_cursorSensitivity = 4;
	m_touchScheme = LegoInputManager::e_gamepad;
	m_haptic = TRUE;
	m_wasd = FALSE;
#ifdef __DJGPP__
	m_xRes = 320;
	m_yRes = 200;
#else
	m_xRes = 640;
	m_yRes = 480;
#endif
	m_exclusiveXRes = m_xRes;
	m_exclusiveYRes = m_yRes;
	m_exclusiveFrameRate = 60.00f;
	m_frameRate = 100.0f;
	m_exclusiveFullScreen = FALSE;
	m_msaaSamples = 0;
	m_anisotropic = 16.0f;
	m_activeInBackground = FALSE;
}

// FUNCTION: ISLE 0x4011a0
IsleApp::~IsleApp()
{
	if (LegoOmni::GetInstance()) {
		Close();
		MxOmni::DestroyInstance();
	}

	SDL_free(m_hdPath);
	SDL_free(m_cdPath);
	SDL_free(m_deviceId);
	SDL_free(m_savePath);
	SDL_free(m_mediaPath);
}

// FUNCTION: ISLE 0x401260
void IsleApp::Close()
{
	MxDSAction ds;
	ds.SetUnknown24(-2);

	if (Lego()) {
		GameState()->Save(0);
		if (InputManager()) {
			InputManager()->QueueEvent(c_notificationKeyPress, 0, 0, 0, SDLK_SPACE);
		}

		VideoManager()->Get3DManager()->GetLego3DView()->GetViewManager()->RemoveAll(NULL);

		Lego()->RemoveWorld(ds.GetAtomId(), ds.GetObjectId());
		Lego()->DeleteObject(ds);
		TransitionManager()->SetWaitIndicator(NULL);
		Lego()->Resume();

		if (BackgroundAudioManager()) {
			BackgroundAudioManager()->Stop();
		}

		while (Streamer()->Close(NULL) == SUCCESS) {
		}

		while (Lego() && !Lego()->DoesEntityExist(ds)) {
			Timer()->GetRealTime();
			TickleManager()->Tickle();
		}
	}
}

// FUNCTION: ISLE 0x4013b0
MxS32 IsleApp::SetupLegoOmni()
{
	MxS32 result = FALSE;

	MxOmniCreateFlags flags;
	if (g_debugDisableSound) {
		flags.CreateSoundManager(FALSE);
	}
#ifdef COMPAT_MODE
	MxS32 failure;
	{
		MxOmniCreateParam param(m_mediaPath, m_windowHandle, m_videoParam, flags);
		failure = Lego()->Create(param) == FAILURE;
	}
#else
	MxS32 failure =
		Lego()->Create(MxOmniCreateParam(m_mediaPath, m_windowHandle, m_videoParam, flags)) == FAILURE;
#endif

	if (!failure) {
		VariableTable()->SetVariable("ACTOR_01", "");
		TickleManager()->SetClientTickleInterval(VideoManager(), 10);
		DcProfInit();
		result = TRUE;
	}

	return result;
}

// FUNCTION: ISLE 0x401560
void IsleApp::SetupVideoFlags(
	MxS32 fullScreen,
	MxS32 flipSurfaces,
	MxS32 backBuffers,
	MxS32 using8bit,
	MxS32 using16bit,
	MxS32 hasLightSupport,
	MxS32 param_7,
	MxS32 wideViewAngle,
	char* deviceId
)
{
	m_videoParam.Flags().SetFullScreen(fullScreen);
	m_videoParam.Flags().SetFlipSurfaces(flipSurfaces);
	m_videoParam.Flags().SetBackBuffers(!backBuffers);
	m_videoParam.Flags().SetLacksLightSupport(!hasLightSupport);
	m_videoParam.Flags().SetF1bit7(param_7);
	m_videoParam.Flags().SetWideViewAngle(wideViewAngle);
	m_videoParam.Flags().SetEnabled(TRUE);
	m_videoParam.SetDeviceName(deviceId);
	if (using8bit) {
		m_videoParam.Flags().Set16Bit(0);
	}
	if (using16bit) {
		m_videoParam.Flags().Set16Bit(1);
	}
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	*appstate = NULL;

	{
#ifdef _arch_dreamcast
		SDL_Log("SDL version 1.2 (KallistiOS)");
#else
		int version = SDL_GetVersion();
		SDL_Log(
			"SDL version %d.%d.%d (%s)",
			SDL_VERSIONNUM_MAJOR(version),
			SDL_VERSIONNUM_MINOR(version),
			SDL_VERSIONNUM_MICRO(version),
			SDL_GetRevision()
		);
#endif
	}

	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
	SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#ifdef __DJGPP__
	SDL_SetHint("SDL_DOS_ALLOW_DIRECT_FRAMEBUFFER", "1");
#endif

#ifdef _arch_dreamcast
	Uint32 initFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK;
#else
	Uint32 initFlags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD;
#ifndef __DJGPP__
	initFlags |= SDL_INIT_HAPTIC;
#endif
#endif

#ifdef _arch_dreamcast
	if (SDL_Init(initFlags) < 0) {
#else
	if (!SDL_Init(initFlags)) {
#endif
		char buffer[256];
		SDL_snprintf(
			buffer,
			sizeof(buffer),
			"\"LEGO® Island\" failed to start.\nPlease quit all other applications and try again.\nSDL error: %s",
			SDL_GetError()
		);
		Any_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "LEGO® Island Error", buffer, NULL);
		return SDL_APP_FAILURE;
	}

	// [library:window]
	// Original game checks for an existing instance here.
	// We don't really need that.

	// Create global app instance
	g_isle = new IsleApp();

#ifdef __vita__
	SceAppUtilInitParam appUtilInitParam = {0};
	SceAppUtilBootParam appUtilBootParam = {0};
	sceAppUtilInit(&appUtilInitParam, &appUtilBootParam);
	SceAppUtilAppEventParam eventParam = {0};
	sceAppUtilReceiveAppEvent(&eventParam);
	if (eventParam.type == 0x05) {
		g_isle->LoadConfig();
		char buffer[2048];
		sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
		if (strstr(buffer, "-config")) {
			sceClibPrintf("Loading Config App.\n");
			sceAppMgrLoadExec("app0:/isle-config.self", NULL, NULL);
		}
	}
#endif

	switch (g_isle->ParseArguments(argc, argv)) {
	case SDL_APP_FAILURE:
		Any_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			"LEGO® Island Error",
			"\"LEGO® Island\" failed to start.  Invalid CLI arguments.",
			window
		);
		return SDL_APP_FAILURE;
	case SDL_APP_SUCCESS:
		return SDL_APP_SUCCESS;
	case SDL_APP_CONTINUE:
		break;
	}

	// Create window
	if (g_isle->SetupWindow() != SUCCESS) {
		Any_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			"LEGO® Island Error",
			"\"LEGO® Island\" failed to start.\nPlease quit all other applications and try again.",
			window
		);
		return SDL_APP_FAILURE;
	}

	// Get reference to window
	*appstate = g_isle->GetWindowHandle();

#ifdef __EMSCRIPTEN__
	SDL_AddEventWatch(
		[](void* userdata, SDL_Event* event) -> bool {
			if (event->type == SDL_EVENT_TERMINATING && g_isle && g_isle->GetGameStarted()) {
				GameState()->Save(0);
				return false;
			}

			return true;
		},
		NULL
	);
#endif
#ifdef __3DS__
	N3DS_SetupAptHooks();
#endif
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	if (g_closed) {
		return SDL_APP_SUCCESS;
	}

	if (!g_isle->Tick()) {
		Any_ShowSimpleMessageBox(
			SDL_MESSAGEBOX_ERROR,
			"LEGO® Island Error",
			"\"LEGO® Island\" failed to start.\nPlease quit all other applications and try again."
			"\nFailed to initialize; see logs for details",
			NULL
		);
		return SDL_APP_FAILURE;
	}

	if (!g_closed) {
		if (g_reqEnableRMDevice) {
			g_reqEnableRMDevice = FALSE;
			VideoManager()->EnableRMDevice();
			g_rmDisabled = FALSE;
			Lego()->Resume();
		}

		if (g_closed) {
			return SDL_APP_SUCCESS;
		}

		if (g_mousedown && g_mousemoved && g_isle) {
			if (!g_isle->Tick()) {
				Any_ShowSimpleMessageBox(
					SDL_MESSAGEBOX_ERROR,
					"LEGO® Island Error",
					"\"LEGO® Island\" failed to start.\nPlease quit all other applications and try again."
					"\nFailed to initialize; see logs for details",
					NULL
				);
				return SDL_APP_FAILURE;
			}
		}

		if (g_mousemoved) {
			g_mousemoved = FALSE;
		}

		g_isle->MoveVirtualMouseViaJoystick();
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	if (!g_isle) {
		return SDL_APP_CONTINUE;
	}

	if (InputManager()) {
		InputManager()->UpdateLastInputMethod(event);
	}

	switch (event->type) {
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
	case SDL_EVENT_MOUSE_MOTION:
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
	case SDL_EVENT_FINGER_MOTION:
	case SDL_EVENT_FINGER_DOWN:
	case SDL_EVENT_FINGER_UP:
	case SDL_EVENT_FINGER_CANCELED:
#endif
#endif
#endif
		IDirect3DRMMiniwinDevice* device = GetD3DRMMiniwinDevice();
		if (device) {
			if (!device->ConvertEventToRenderCoordinates(event)) {
				SDL_Log("Failed to convert event coordinates: %s", SDL_GetError());
			}

			device->Release();
		}

#ifdef __EMSCRIPTEN__
		Emscripten_ConvertEventToRenderCoordinates(event);
#endif
		break;
	}

#ifdef __vita__
	// reject back touch panel
	switch (event->type) {
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
	case SDL_EVENT_FINGER_MOTION:
	case SDL_EVENT_FINGER_DOWN:
	case SDL_EVENT_FINGER_UP:
	case SDL_EVENT_FINGER_CANCELED:
#endif
#endif
#endif
		if (event->tfinger.touchID == 2) {
			return SDL_APP_CONTINUE;
		}
	}
#endif

	switch (event->type) {
	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		if (!g_isle->GetActiveInBackground()) {
			g_isle->SetWindowActive(TRUE);
			Lego()->Resume();
		}
		break;
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		if (!g_isle->GetActiveInBackground() && g_isle->GetGameStarted()) {
			g_isle->SetWindowActive(FALSE);
			Lego()->Pause();
#ifdef __EMSCRIPTEN__
			GameState()->Save(0);
#endif
		}
		break;
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
	case SDL_EVENT_QUIT:
		if (!g_closed) {
			delete g_isle;
			g_isle = NULL;
			g_closed = TRUE;
		}
		break;
	case SDL_EVENT_KEY_DOWN: {
#ifdef _arch_dreamcast
		if (event->type == SDL_KEYDOWN) {
			SDL_Keycode keyCode = event->key.keysym.sym;

			if ((event->key.keysym.mod & KMOD_LALT) && keyCode == SDLK_RETURN) {
				SDL_WM_ToggleFullScreen(window);
			}
			else {
				InputManager()->QueueEvent(c_notificationKeyPress, keyCode, 0, 0, keyCode);
			}
		}
#else
		if (event->key.repeat) {
			break;
		}

		SDL_Keycode keyCode = event->key.key;

		if ((event->key.mod & SDL_KMOD_LALT) && keyCode == SDLK_RETURN) {
			SDL_SetWindowFullscreen(window, !(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN));
		}
		else {
			if (InputManager()) {
				InputManager()->QueueEvent(c_notificationKeyPress, keyCode, 0, 0, keyCode);
			}
		}
#endif
		break;
	}
#ifndef _arch_dreamcast
	case SDL_EVENT_KEYBOARD_ADDED:
		if (InputManager()) {
			InputManager()->AddKeyboard(event->kdevice.which);
		}
		break;
	case SDL_EVENT_KEYBOARD_REMOVED:
		if (InputManager()) {
			InputManager()->RemoveKeyboard(event->kdevice.which);
		}
		break;
	case SDL_EVENT_MOUSE_ADDED:
		if (InputManager()) {
			InputManager()->AddMouse(event->mdevice.which);
		}
		break;
	case SDL_EVENT_MOUSE_REMOVED:
		if (InputManager()) {
			InputManager()->RemoveMouse(event->mdevice.which);
		}
		break;
#endif
	case SDL_EVENT_GAMEPAD_ADDED:
		if (InputManager()) {
			InputManager()->AddJoystick(event->jdevice.which);
		}
		break;
	case SDL_EVENT_GAMEPAD_REMOVED:
		if (InputManager()) {
			InputManager()->RemoveJoystick(event->jdevice.which);
		}
		break;
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
		switch (event->gbutton.button) {
		case SDL_GAMEPAD_BUTTON_DPAD_UP:
			g_dpadUp = true;
			break;
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
			g_dpadDown = true;
			break;
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
			g_dpadLeft = true;
			break;
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
			g_dpadRight = true;
			break;
		case SDL_GAMEPAD_BUTTON_EAST:
			g_mousedown = TRUE;
			if (InputManager()) {
				InputManager()->QueueEvent(
					c_notificationButtonDown,
					LegoEventNotificationParam::c_lButtonState,
					g_lastMouseX,
					g_lastMouseY,
					0
				);
			}
			break;

		case SDL_GAMEPAD_BUTTON_SOUTH:
			if (InputManager()) {
				InputManager()->QueueEvent(c_notificationKeyPress, SDLK_SPACE, 0, 0, SDLK_SPACE);
			}
			break;

#ifdef __vita__ // conflicts with screenshot button combination
		case SDL_GAMEPAD_BUTTON_BACK:
#else
		case SDL_GAMEPAD_BUTTON_START:
#endif
			if (InputManager()) {
				InputManager()->QueueEvent(c_notificationKeyPress, SDLK_ESCAPE, 0, 0, SDLK_ESCAPE);
			}
			break;
		}
		break;
	}

	case SDL_EVENT_GAMEPAD_BUTTON_UP: {
		switch (event->gbutton.button) {
		case SDL_GAMEPAD_BUTTON_DPAD_UP:
			g_dpadUp = false;
			break;
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
			g_dpadDown = false;
			break;
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
			g_dpadLeft = false;
			break;
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
			g_dpadRight = false;
			break;
		case SDL_GAMEPAD_BUTTON_EAST:
			g_mousedown = FALSE;
			if (InputManager()) {
				InputManager()->QueueEvent(
					c_notificationButtonUp,
					LegoEventNotificationParam::c_lButtonState,
					g_lastMouseX,
					g_lastMouseY,
					0
				);
			}
			break;
		}
		break;
	}
	case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
		MxS16 axisValue = 0;
		if (event->gaxis.value < -8000 || event->gaxis.value > 8000) {
			// Ignore small axis values
			axisValue = event->gaxis.value;
		}
		if (event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHTX) {
			g_lastJoystickMouseX = ((MxFloat) axisValue) / SDL_JOYSTICK_AXIS_MAX * g_isle->GetCursorSensitivity();
		}
		else if (event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHTY) {
			g_lastJoystickMouseY = ((MxFloat) axisValue) / SDL_JOYSTICK_AXIS_MAX * g_isle->GetCursorSensitivity();
		}
		else if (event->gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
			if (axisValue != 0 && !g_mousedown) {
				g_mousedown = TRUE;

				if (InputManager()) {
					InputManager()->QueueEvent(
						c_notificationButtonDown,
						LegoEventNotificationParam::c_lButtonState,
						g_lastMouseX,
						g_lastMouseY,
						0
					);
				}
			}
			else if (axisValue == 0 && g_mousedown) {
				g_mousedown = FALSE;

				if (InputManager()) {
					InputManager()->QueueEvent(
						c_notificationButtonUp,
						LegoEventNotificationParam::c_lButtonState,
						g_lastMouseX,
						g_lastMouseY,
						0
					);
				}
			}
		}
		break;
	}
	case SDL_EVENT_MOUSE_MOTION:
		if (g_mouseWarped) {
			g_mouseWarped = FALSE;
			break;
		}

		g_mousemoved = TRUE;

		if (InputManager()) {
			InputManager()->QueueEvent(
				c_notificationMouseMove,
				IsleApp::MapMouseButtonFlagsToModifier(event->motion.state),
				event->motion.x,
				event->motion.y,
				0
			);
		}

		g_lastMouseX = event->motion.x;
		g_lastMouseY = event->motion.y;

#if defined(__DJGPP__) || defined(__DREAMCAST__)
		if (VideoManager()) {
			VideoManager()->MoveCursor(Min((MxS32) g_lastMouseX, 639), Min((MxS32) g_lastMouseY, 479));
		}
#else
		SDL_ShowCursor(1);
		g_isle->SetDrawCursor(FALSE);
		if (VideoManager()) {
			VideoManager()->SetCursorBitmap(NULL);
		}
#endif
		break;
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
	case SDL_EVENT_FINGER_MOTION: {
		g_mousemoved = TRUE;

		float x = event->tfinger.x * g_targetWidth;
		float y = event->tfinger.y * g_targetHeight;

		if (InputManager()) {
			MxU8 modifier = LegoEventNotificationParam::c_lButtonState;
			if (InputManager()->HandleTouchEvent(event, g_isle->GetTouchScheme())) {
				modifier |= LegoEventNotificationParam::c_motionHandled;
			}

			InputManager()->QueueEvent(c_notificationMouseMove, modifier, x, y, 0);
		}

		g_lastMouseX = x;
		g_lastMouseY = y;

		SDL_ShowCursor(0);
		g_isle->SetDrawCursor(FALSE);
		if (VideoManager()) {
			VideoManager()->SetCursorBitmap(NULL);
		}
		break;
	}
#endif
#endif
#endif
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		g_mousedown = TRUE;

		if (InputManager()) {
			InputManager()->QueueEvent(
				c_notificationButtonDown,
				IsleApp::MapMouseButtonFlagsToModifier(SDL_GetMouseState(NULL, NULL)),
				event->button.x,
				event->button.y,
				0
			);
		}
		break;
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
	case SDL_EVENT_FINGER_DOWN: {
		g_mousedown = TRUE;

		float x = event->tfinger.x * g_targetWidth;
		float y = event->tfinger.y * g_targetHeight;

		if (InputManager()) {
			InputManager()->HandleTouchEvent(event, g_isle->GetTouchScheme());
			InputManager()->QueueEvent(c_notificationButtonDown, LegoEventNotificationParam::c_lButtonState, x, y, 0);
		}

		g_lastMouseX = x;
		g_lastMouseY = y;

		SDL_ShowCursor(0);
		g_isle->SetDrawCursor(FALSE);
		if (VideoManager()) {
			VideoManager()->SetCursorBitmap(NULL);
		}
		break;
	}
#endif
#endif
#endif
	case SDL_EVENT_MOUSE_BUTTON_UP:
		g_mousedown = FALSE;

		if (InputManager()) {
			InputManager()->QueueEvent(
				c_notificationButtonUp,
				IsleApp::MapMouseButtonFlagsToModifier(SDL_GetMouseState(NULL, NULL)),
				event->button.x,
				event->button.y,
				0
			);
		}
		break;
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
	case SDL_EVENT_FINGER_UP:
	case SDL_EVENT_FINGER_CANCELED: {
		g_mousedown = FALSE;

		float x = event->tfinger.x * g_targetWidth;
		float y = event->tfinger.y * g_targetHeight;

		g_isle->DetectDoubleTap(event->tfinger);

		if (InputManager()) {
			InputManager()->HandleTouchEvent(event, g_isle->GetTouchScheme());
			InputManager()->QueueEvent(c_notificationButtonUp, 0, x, y, 0);
		}
		break;
	}
#endif
#endif
#endif
	}

	if (event->user.type == g_legoSdlEvents.m_windowsMessage) {
		switch (event->user.code) {
		case WM_ISLE_SETCURSOR:
			g_isle->SetupCursor((Cursor) (uintptr_t) event->user.data1);
			break;
		case WM_TIMER:
			if (InputManager()) {
				InputManager()->QueueEvent(c_notificationTimer, (MxU8) (uintptr_t) event->user.data1, 0, 0, 0);
			}
			break;
		default:
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unknown SDL Windows message: 0x%" SDL_PRIx32, event->user.code);
			break;
		}
	}
	else if (event->user.type == g_legoSdlEvents.m_presenterProgress) {
		MxDSAction* action = static_cast<MxDSAction*>(event->user.data1);
		MxPresenter::TickleState state = static_cast<MxPresenter::TickleState>(event->user.code);

#ifdef __EMSCRIPTEN__
		if (!g_isle->GetGameStarted()) {
			Emscripten_SendPresenterProgress(action, state);
		}
#endif

		if (!g_isle->GetGameStarted() && action && state == MxPresenter::e_ready &&
			!SDL_strncmp(action->GetObjectName(), "Lego_Smk", 8)) {
			g_isle->SetGameStarted(TRUE);

#ifdef __EMSCRIPTEN__
			Emscripten_SetupWindow((SDL_Window*) g_isle->GetWindowHandle());
#endif

			SDL_Log("Game started");
		}
	}
	else if (event->user.type == g_legoSdlEvents.m_gameEvent) {
		auto rumble = [](float p_strength, float p_lowFrequencyRumble, float p_highFrequencyRumble, MxU32 p_milliseconds
					  ) {
			if (g_isle->GetHaptic() &&
				!InputManager()
					 ->HandleRumbleEvent(p_strength, p_lowFrequencyRumble, p_highFrequencyRumble, p_milliseconds)) {
// Platform-specific handling
#ifdef __EMSCRIPTEN__
				Emscripten_HandleRumbleEvent(p_lowFrequencyRumble, p_highFrequencyRumble, p_milliseconds);
#endif
			}
		};

		switch (event->user.code) {
		case e_hitActor:
			rumble(0.5f, 0.5f, 0.5f, 700);
			break;
		case e_skeletonKick:
			rumble(0.8f, 0.8f, 0.8f, 2500);
			break;
		case e_raceFinished:
		case e_goodEnding:
		case e_badEnding:
			rumble(1.0f, 1.0f, 1.0f, 3000);
			break;
#ifdef __EMSCRIPTEN__
		case e_saveSlotWritten:
			Emscripten_SendSaveSlotWritten((MxS32) (intptr_t) event->user.data1);
			break;
		case e_saveStateChanged:
			Emscripten_SendSaveStateChanged();
			break;
#endif
		}
	}

#ifdef EXTENSIONS
	Extensions::ThirdPersonCameraExt::HandleSDLEvent(event);
#endif

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	if (appstate != NULL) {
		#ifndef __DREAMCAST__
		SDL_DestroyWindow((SDL_Window*) appstate);
#endif
	}

	SDL_Quit();
}

MxU8 IsleApp::MapMouseButtonFlagsToModifier(SDL_MouseButtonFlags p_flags)
{
	// [library:window]
	// Map button states to Windows button states (LegoEventNotificationParam)
	// Not mapping mod keys SHIFT and CTRL since they are not used by the game.

	MxU8 modifier = 0;
	if (p_flags & SDL_BUTTON_LMASK) {
		modifier |= LegoEventNotificationParam::c_lButtonState;
	}
	if (p_flags & SDL_BUTTON_RMASK) {
		modifier |= LegoEventNotificationParam::c_rButtonState;
	}

	return modifier;
}

#ifdef MINIWIN
// #include "miniwin/debug.h"
#endif

MxResult IsleApp::SetupWindow()
{
	if (!LoadConfig()) {
		return FAILURE;
	}

#ifdef MINIWIN
	InitDebugFlags();
#endif

#if defined(MINIWIN)
	// MINIWIN: window/VESA mode matches the game's rendering resolution.
#ifdef __DREAMCAST__
	if (g_debugLowResMode) {
		g_targetWidth = 320;
		g_targetHeight = 240;
	}
	else {
		g_targetWidth = m_xRes;
		g_targetHeight = m_yRes;
	}
#else
	g_targetWidth = m_xRes;
	g_targetHeight = m_yRes;
#endif
#else
	// DX5: fullscreen uses exclusive resolution for display mode switching.
	if (m_fullScreen) {
		g_targetWidth = m_exclusiveXRes;
		g_targetHeight = m_exclusiveYRes;
	}
	else {
		g_targetWidth = m_xRes;
		g_targetHeight = m_yRes;
	}
#endif

	SetupVideoFlags(
		m_fullScreen,
		m_flipSurfaces,
		m_backBuffersInVram,
		m_using8bit,
		m_using16bit,
		m_hasLightSupport,
		FALSE,
		m_wideViewAngle,
		m_deviceId
	);

	MxOmni::SetSound3D(m_use3dSound);

	srand(time(NULL));

	// [library:window] Use original game cursors in the resources instead?
	m_cursorCurrent = m_cursorArrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
	m_cursorBusy = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
	m_cursorNo = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);
	SDL_SetCursor(m_cursorCurrent);
	m_cursorCurrentBitmap = m_cursorArrowBitmap = &arrow_cursor;
	m_cursorBusyBitmap = &busy_cursor;
	m_cursorNoBitmap = &no_cursor;

	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, g_targetWidth);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, g_targetHeight);
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, m_fullScreen);
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, WINDOW_TITLE);
#if defined(MINIWIN) && !defined(__3DS__) && !defined(WINDOWS_STORE) && !defined(__vita__) && !defined(__DJGPP__)
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN, true);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
#endif

	#ifdef __DREAMCAST__
	SDL_ClearError();
	Uint32 my_flags = SDL_FULLSCREEN | SDL_HWSURFACE;
	window = SDL_SetVideoMode(g_targetWidth, g_targetHeight, 16, my_flags);
	#ifdef __DREAMCAST__
	GLdcConfig glconfig;
	glKosInitConfig(&glconfig);
	glconfig.autosort_enabled = 0;
	glconfig.fsaa_enabled = 0;
	glconfig.initial_op_capacity = 1024 * 24;
	glconfig.initial_tr_capacity = 1024 * 2;
	glconfig.initial_pt_capacity = 512;
	glconfig.initial_immediate_capacity = 512;
	glconfig.texture_twiddle = 0;
	glKosInitEx(&glconfig);
#endif
#ifndef __DREAMCAST__
	printf("SDL_SetVideoMode returned %p. SDL Error: %s\n", window, SDL_GetError());
#endif
#else
	window = SDL_CreateWindowWithProperties(props);
#endif

#ifdef __EMSCRIPTEN__
	// Force correct window size since SDL3 may have picked up CSS dimensions
	SDL_SetWindowSize(window, g_targetWidth, g_targetHeight);
#endif

	SDL_SetPointerProperty(SDL_GetWindowProperties(window), ISLE_PROP_WINDOW_CREATE_VIDEO_PARAM, &m_videoParam);

#ifdef __DJGPP__
	// DOS: request an 8-bit (INDEX8) fullscreen mode so the VESA
	// framebuffer is paletted and we can blit INDEX8 surfaces directly.
	{
		SDL_DisplayMode mode = {};
		mode.w = g_targetWidth;
		mode.h = g_targetHeight;
		mode.format = SDL_PIXELFORMAT_INDEX8;
		SDL_SetWindowFullscreenMode(window, &mode);
	}
#else
	if (m_exclusiveFullScreen && m_fullScreen) {
		SDL_DisplayMode closestMode;
		SDL_DisplayID displayID = SDL_GetDisplayForWindow(window);
		if (SDL_GetClosestFullscreenDisplayMode(
				displayID,
				m_exclusiveXRes,
				m_exclusiveYRes,
				m_exclusiveFrameRate,
				true,
				&closestMode
			)) {
			SDL_SetWindowFullscreenMode(window, &closestMode);
		}
	}
#endif

#ifdef MINIWIN
	m_windowHandle = reinterpret_cast<HWND>(window);
#else
	m_windowHandle =
		(HWND) SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif

	SDL_DestroyProperties(props);

	if (!m_windowHandle) {
		return FAILURE;
	}

	SDL_IOStream* icon_stream = SDL_IOFromMem(isle_bmp, isle_bmp_len);

	if (icon_stream) {
		SDL_Surface* icon = SDL_LoadBMP_IO(icon_stream, true);

		if (icon) {
			SDL_SetWindowIcon(window, icon);
			SDL_DestroySurface(icon);
		}
		else {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load icon: %s", SDL_GetError());
		}
	}
	else {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open SDL_IOStream for icon: %s", SDL_GetError());
	}

#ifndef __DREAMCAST__
	printf("Calling SetupLegoOmni...\n");
#endif
	if (!SetupLegoOmni()) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SetupLegoOmni FAILED!");
		return FAILURE;
	}
#ifndef __DREAMCAST__
	printf("SetupLegoOmni SUCCEEDED!\n");
#endif

	GameState()->SetSavePath(m_savePath);

#ifndef __DREAMCAST__
	printf("Calling VerifyFilesystem...\n");
#endif
	if (VerifyFilesystem() != SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "VerifyFilesystem FAILED!");
		return FAILURE;
	}
#ifndef __DREAMCAST__
	printf("VerifyFilesystem SUCCEEDED!\n");
#endif

	Lego()->LoadSiLoader();

	DetectGameVersion();
	GameState()->SerializePlayersInfo(LegoStorage::c_read);
	GameState()->SerializeScoreHistory(LegoStorage::c_read);

	MxS32 iVar10;
	switch (m_islandQuality) {
	case 0:
		iVar10 = 1;
		break;
	case 1:
		iVar10 = 2;
		break;
	default:
		iVar10 = 100;
	}

	MxS32 uVar1 = (m_islandTexture == 0);
	LegoModelPresenter::configureLegoModelPresenter(uVar1);
	LegoPartPresenter::configureLegoPartPresenter(uVar1, iVar10);
	LegoWorldPresenter::configureLegoWorldPresenter(m_islandQuality);
	LegoBuildingManager::configureLegoBuildingManager(m_islandQuality);
	LegoROI::configureLegoROI(iVar10);
#ifdef __DREAMCAST__
	if (g_debugMaxExtras >= 0) {
		m_maxAllowedExtras = g_debugMaxExtras;
	}
#endif
	LegoAnimationManager::configureLegoAnimationManager(m_maxAllowedExtras);
	MxTransitionManager::configureMxTransitionManager(m_transitionType);
	RealtimeView::SetUserMaxLOD(m_maxLod);
	if (LegoOmni::GetInstance()) {
		if (LegoOmni::GetInstance()->GetVideoManager()) {
			LegoOmni::GetInstance()->GetVideoManager()->SetCursorBitmap(m_cursorCurrentBitmap);
		}
		MxDirect3D* d3d = LegoOmni::GetInstance()->GetVideoManager()->GetDirect3D();
		if (d3d) {
			SDL_Log(
				"Direct3D driver name=\"%s\" description=\"%s\"",
				d3d->GetDeviceName().c_str(),
				d3d->GetDeviceDescription().c_str()
			);
		}
		else {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to get D3D device name and description");
		}
		if (LegoOmni::GetInstance()->GetInputManager()) {
			LegoOmni::GetInstance()->GetInputManager()->SetWasd(m_wasd);
		}
	}

	return SUCCESS;
}

// FUNCTION: ISLE 0x4028d0
bool IsleApp::LoadConfig()
{
#ifdef IOS
	const char* prefPath = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
#elif defined(ANDROID)
	MxString androidPath = MxString(SDL_GetAndroidExternalStoragePath()) + "/";
	const char* prefPath = androidPath.GetData();
#elif defined(EMSCRIPTEN)
	if (m_iniPath && !Emscripten_SetupConfig(m_iniPath)) {
		m_iniPath = NULL;
	}
	char* prefPath = SDL_GetPrefPath("isledecomp", "isle");
#else
	char* prefPath = SDL_GetPrefPath("isledecomp", "isle");
#endif

	MxString iniConfig;
	if (m_iniPath) {
		iniConfig = m_iniPath;
	}
	else if (prefPath) {
		iniConfig = prefPath;
		iniConfig += "isle.ini";
	}
	else {
		iniConfig = "isle.ini";
	}

	SDL_Log("Reading configuration from \"%s\"", iniConfig.GetData());

	dictionary* dict = iniparser_load(iniConfig.GetData());

	// [library:config]
	// Load sane defaults if dictionary failed to load
	if (!dict || dict->n == 0) {
		iniparser_freedict(dict);

		if (m_iniPath) {
			SDL_Log("Invalid config path '%s'", m_iniPath);
			return false;
		}

		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Loading sane defaults");
		FILE* iniFP = fopen(iniConfig.GetData(), "wb");

		if (!iniFP) {
			SDL_LogError(
				SDL_LOG_CATEGORY_APPLICATION,
				"Failed to write config at '%s': %s",
				iniConfig.GetData(),
				strerror(errno)
			);
			return false;
		}

		char buf[32];
		dict = dictionary_new(0);
		iniparser_set(dict, "isle", NULL);

		iniparser_set(dict, "isle:diskpath", SDL_GetBasePath());
		iniparser_set(dict, "isle:cdpath", MxOmni::GetCD());
		iniparser_set(dict, "isle:mediapath", SDL_GetBasePath());
		iniparser_set(dict, "isle:savepath", prefPath);

		iniparser_set(dict, "isle:Flip Surfaces", m_flipSurfaces ? "true" : "false");
		iniparser_set(dict, "isle:Full Screen", m_fullScreen ? "true" : "false");
		iniparser_set(dict, "isle:Exclusive Full Screen", m_exclusiveFullScreen ? "true" : "false");
		iniparser_set(dict, "isle:Wide View Angle", m_wideViewAngle ? "true" : "false");

		iniparser_set(dict, "isle:3DSound", m_use3dSound ? "true" : "false");
		iniparser_set(dict, "isle:Music", m_useMusic ? "true" : "false");

		SDL_snprintf(buf, sizeof(buf), "%f", m_cursorSensitivity);
		iniparser_set(dict, "isle:Cursor Sensitivity", buf);

		iniparser_set(dict, "isle:Back Buffers in Video RAM", "-1");

		iniparser_set(dict, "isle:Island Quality", SDL_itoa(m_islandQuality, buf, 10));
		iniparser_set(dict, "isle:Island Texture", SDL_itoa(m_islandTexture, buf, 10));
		SDL_snprintf(buf, sizeof(buf), "%f", m_maxLod);
		iniparser_set(dict, "isle:Max LOD", buf);
		iniparser_set(dict, "isle:Max Allowed Extras", SDL_itoa(m_maxAllowedExtras, buf, 10));
		iniparser_set(dict, "isle:Transition Type", SDL_itoa(m_transitionType, buf, 10));
		iniparser_set(dict, "isle:Touch Scheme", SDL_itoa(m_touchScheme, buf, 10));
		iniparser_set(dict, "isle:Haptic", m_haptic ? "true" : "false");
		iniparser_set(dict, "isle:WASD", m_wasd ? "true" : "false");
		iniparser_set(dict, "isle:Horizontal Resolution", SDL_itoa(m_xRes, buf, 10));
		iniparser_set(dict, "isle:Vertical Resolution", SDL_itoa(m_yRes, buf, 10));
		iniparser_set(dict, "isle:Exclusive X Resolution", SDL_itoa(m_exclusiveXRes, buf, 10));
		iniparser_set(dict, "isle:Exclusive Y Resolution", SDL_itoa(m_exclusiveYRes, buf, 10));
		iniparser_set(dict, "isle:Exclusive Framerate", SDL_itoa(m_exclusiveFrameRate, buf, 10));
		iniparser_set(dict, "isle:Frame Delta", SDL_itoa(m_frameDelta, buf, 10));
		iniparser_set(dict, "isle:MSAA", SDL_itoa(m_msaaSamples, buf, 10));
		iniparser_set(dict, "isle:Anisotropic", SDL_itoa(m_anisotropic, buf, 10));
		iniparser_set(dict, "isle:Active in background", m_activeInBackground ? "true" : "false");

#ifdef EXTENSIONS
		iniparser_set(dict, "extensions", NULL);
		for (const char* key : Extensions::availableExtensions) {
			iniparser_set(dict, key, "false");
		}
#endif

#ifdef __3DS__
		N3DS_SetupDefaultConfigOverrides(dict);
#endif
#ifdef __SWITCH__
		NX_SetupDefaultConfigOverrides(dict);
#endif
#ifdef WINDOWS_STORE
		XBONE_SetupDefaultConfigOverrides(dict);
#endif
#ifdef IOS
		IOS_SetupDefaultConfigOverrides(dict);
#endif
#ifdef ANDROID
		Android_SetupDefaultConfigOverrides(dict);
#endif

#ifdef __vita__
		VITA_SetupDefaultConfigOverrides(dict);
#endif
		iniparser_dump_ini(dict, iniFP);
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "New config written at '%s'", iniConfig.GetData());
		fclose(iniFP);
	}

#ifdef __EMSCRIPTEN__
	Emscripten_SetupDefaultConfigOverrides(dict);
#endif

	MxOmni::SetHD((m_hdPath = SDL_strdup(iniparser_getstring(dict, "isle:diskpath", SDL_GetBasePath()))));
	MxOmni::SetCD((m_cdPath = SDL_strdup(iniparser_getstring(dict, "isle:cdpath", MxOmni::GetCD()))));
	m_savePath = SDL_strdup(iniparser_getstring(dict, "isle:savepath", prefPath));
	m_mediaPath = SDL_strdup(iniparser_getstring(dict, "isle:mediapath", m_hdPath));
	m_flipSurfaces = iniparser_getboolean(dict, "isle:Flip Surfaces", m_flipSurfaces);
	m_fullScreen = iniparser_getboolean(dict, "isle:Full Screen", m_fullScreen);
	m_exclusiveFullScreen = iniparser_getboolean(dict, "isle:Exclusive Full Screen", m_exclusiveFullScreen);
	m_wideViewAngle = iniparser_getboolean(dict, "isle:Wide View Angle", m_wideViewAngle);
	m_use3dSound = iniparser_getboolean(dict, "isle:3DSound", m_use3dSound);
	m_useMusic = iniparser_getboolean(dict, "isle:Music", m_useMusic);
	m_cursorSensitivity = iniparser_getdouble(dict, "isle:Cursor Sensitivity", m_cursorSensitivity);

	MxS32 backBuffersInVRAM = iniparser_getboolean(dict, "isle:Back Buffers in Video RAM", -1);
	if (backBuffersInVRAM != -1) {
		m_backBuffersInVram = !backBuffersInVRAM;
	}

	MxS32 bitDepth = iniparser_getint(dict, "isle:Display Bit Depth", -1);
	if (bitDepth != -1) {
		if (bitDepth == 8) {
			m_using8bit = TRUE;
		}
		else if (bitDepth == 16) {
			m_using16bit = TRUE;
		}
	}

	m_islandQuality = iniparser_getint(dict, "isle:Island Quality", m_islandQuality);
	m_islandTexture = iniparser_getint(dict, "isle:Island Texture", m_islandTexture);
	m_maxLod = iniparser_getdouble(dict, "isle:Max LOD", m_maxLod);
	m_maxAllowedExtras = iniparser_getint(dict, "isle:Max Allowed Extras", m_maxAllowedExtras);
	m_transitionType =
		(MxTransitionManager::TransitionType) iniparser_getint(dict, "isle:Transition Type", m_transitionType);
	m_touchScheme = (LegoInputManager::TouchScheme) iniparser_getint(dict, "isle:Touch Scheme", m_touchScheme);
	m_haptic = iniparser_getboolean(dict, "isle:Haptic", m_haptic);
	m_wasd = iniparser_getboolean(dict, "isle:WASD", m_wasd);
	m_xRes = iniparser_getint(dict, "isle:Horizontal Resolution", m_xRes);
	m_yRes = iniparser_getint(dict, "isle:Vertical Resolution", m_yRes);
	m_exclusiveXRes = iniparser_getint(dict, "isle:Exclusive X Resolution", m_exclusiveXRes);
	m_exclusiveYRes = iniparser_getint(dict, "isle:Exclusive Y Resolution", m_exclusiveXRes);
	m_exclusiveFrameRate = iniparser_getdouble(dict, "isle:Exclusive Framerate", m_exclusiveFrameRate);
	if (!m_fullScreen) {
		m_videoParam.GetRect() = MxRect32(0, 0, (m_xRes - 1), (m_yRes - 1));
	}
	m_frameRate = (1000.0f / iniparser_getdouble(dict, "isle:Frame Delta", m_frameDelta));
	m_frameDelta = static_cast<int>(iniparser_getdouble(dict, "isle:Frame Delta", m_frameDelta));
	m_videoParam.SetMSAASamples((m_msaaSamples = iniparser_getint(dict, "isle:MSAA", m_msaaSamples)));
	m_videoParam.SetAnisotropic((m_anisotropic = iniparser_getdouble(dict, "isle:Anisotropic", m_anisotropic)));
	m_activeInBackground = iniparser_getboolean(dict, "isle:Active in Background", m_activeInBackground);

	const char* deviceId = iniparser_getstring(dict, "isle:3D Device ID", NULL);
	if (deviceId != NULL) {
		m_deviceId = SDL_strdup(deviceId);
	}

#ifdef EXTENSIONS
	for (const char* key : Extensions::availableExtensions) {
		if (iniparser_getboolean(dict, key, 0)) {
			std::vector<const char*> extensionKeys;
			const char* section = SDL_strchr(key, ':') + 1;
			extensionKeys.resize(iniparser_getsecnkeys(dict, section));
			iniparser_getseckeys(dict, section, extensionKeys.data());

			std::map<std::string, std::string> extensionDict;
			for (const char* key : extensionKeys) {
				extensionDict[key] = iniparser_getstring(dict, key, NULL);
			}

			Extensions::Enable(key, std::move(extensionDict));
		}
	}
#endif

	iniparser_freedict(dict);

	[](auto path) {
		if constexpr (std::is_same_v<decltype(path), char*>) {
			SDL_free(path);
		}
	}(prefPath);
	return true;
}

// FUNCTION: ISLE 0x402c20
inline bool IsleApp::Tick()
{
	// GLOBAL: ISLE 0x4101c0
	static MxLong g_lastFrameTime = 0;

	// GLOBAL: ISLE 0x4101bc
	static MxS32 g_startupDelay = 1;

	if (!m_windowActive) {
#ifdef __DREAMCAST__
		thd_pass();
#else
		SDL_Delay(1);
#endif
		return true;
	}

	if (!Lego()) {
		return true;
	}

#ifdef EXTENSIONS
	if (Extensions::IsMultiplayerRejected()) {
		g_closed = TRUE;
		return true;
	}
#endif

	if (!TickleManager()) {
		return true;
	}
	if (!Timer()) {
		return true;
	}

	MxLong currentTime = Timer()->GetRealTime();
	if (currentTime < g_lastFrameTime) {
		g_lastFrameTime = -m_frameDelta;
	}

	if (m_frameDelta + g_lastFrameTime >= currentTime) {
#ifdef __DREAMCAST__
		thd_pass();
#else
		SDL_Delay(1);
#endif
		return true;
	}

	if (!Lego()->IsPaused()) {
		DC_ZONE(Z_TICK);
		TickleManager()->Tickle();
	}
	DcProfFrameEnd();
	g_lastFrameTime = currentTime;

	if (g_startupDelay == 0) {
		return true;
	}

	g_startupDelay--;
	if (g_startupDelay != 0) {
		return true;
	}

	LegoOmni::GetInstance()->CreateBackgroundAudio();
	BackgroundAudioManager()->Enable(m_useMusic);

	MxStreamController* stream = Streamer()->Open("\\lego\\scripts\\isle\\isle", MxStreamer::e_diskStream);
	MxDSAction ds;

	if (!stream) {
		stream = Streamer()->Open("\\lego\\scripts\\nocd", MxStreamer::e_diskStream);
		if (!stream) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open NOCD.si: Streamer failed to load");
			return false;
		}

		ds.SetAtomId(stream->GetAtom());
		ds.SetUnknown24(-1);
		ds.SetObjectId(0);
		VideoManager()->EnableFullScreenMovie(TRUE, TRUE);

		if (Start(&ds) != SUCCESS) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open NOCD.si: Failed to start initial action");
			return false;
		}
	}
	else {
		ds.SetAtomId(stream->GetAtom());
		ds.SetUnknown24(-1);
		ds.SetObjectId(0);
		if (Start(&ds) != SUCCESS) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open ISLE.si: Failed to start initial action");
			return false;
		}
	}

	return true;
}

// FUNCTION: ISLE 0x402e80
void IsleApp::SetupCursor(Cursor p_cursor)
{
	switch (p_cursor) {
	case e_cursorArrow:
		m_cursorCurrent = m_cursorArrow;
		m_cursorCurrentBitmap = m_cursorArrowBitmap;
		break;
	case e_cursorBusy:
		m_cursorCurrent = m_cursorBusy;
		m_cursorCurrentBitmap = m_cursorBusyBitmap;
		break;
	case e_cursorNo:
		m_cursorCurrent = m_cursorNo;
		m_cursorCurrentBitmap = m_cursorNoBitmap;
		break;
	case e_cursorNone:
		m_cursorCurrent = NULL;
		m_cursorCurrentBitmap = NULL;
	case e_cursorUnused3:
	case e_cursorUnused4:
	case e_cursorUnused5:
	case e_cursorUnused6:
	case e_cursorUnused7:
	case e_cursorUnused8:
	case e_cursorUnused9:
	case e_cursorUnused10:
		break;
	}

#ifdef __DREAMCAST__
	SDL_ShowCursor(0);
	if (VideoManager()) {
		VideoManager()->SetCursorBitmap(m_cursorCurrentBitmap);
	}
#else
	if (g_isle->GetDrawCursor()) {
		VideoManager()->SetCursorBitmap(m_cursorCurrentBitmap);
	}
	else {
		if (m_cursorCurrent != NULL) {
			SDL_SetCursor(m_cursorCurrent);
			SDL_ShowCursor(1);
		}
		else {
			SDL_ShowCursor(0);
		}
	}
#endif
}

SDL_AppResult IsleApp::ParseArguments(int argc, char** argv)
{
	for (int i = 1, consumed; i < argc; i += consumed) {
		consumed = -1;

		if (strcmp(argv[i], "--ini") == 0 && i + 1 < argc) {
			m_iniPath = argv[i + 1];
			consumed = 2;
		}
		else if (strcmp(argv[i], "--help") == 0) {
			DisplayArgumentHelp(argv[0]);
			return SDL_APP_SUCCESS;
		}
		if (consumed <= 0) {
			SDL_Log("Invalid argument(s): %s", argv[i]);
			DisplayArgumentHelp(argv[0]);
			return SDL_APP_FAILURE;
		}
	}

	return SDL_APP_CONTINUE;
}

void IsleApp::DisplayArgumentHelp(const char* p_execName)
{
	SDL_Log("Usage: %s [options]", p_execName);
	SDL_Log("Options:");
	SDL_Log("	--ini <path>		Set custom path to .ini config");
	SDL_Log("	--help			Show this help message");
}

MxResult IsleApp::VerifyFilesystem()
{
#ifdef __EMSCRIPTEN__
	Emscripten_SetupFilesystem();
#else
	for (const char* file : g_files) {
		const char* searchPaths[] = {".", m_hdPath, m_cdPath};
		bool found = false;

		for (const char* base : searchPaths) {
			MxString path(base);
			path += file;
			path.MapPathToFilesystem();

			FILE* f = fopen(path.GetData(), "rb");
			if (f) {
				fclose(f);
				found = true;
				break;
			}
		}

		if (!found) {
			char buffer[1024];
			SDL_snprintf(
				buffer,
				sizeof(buffer),
				"\"LEGO® Island\" failed to start.\nPlease make sure the file %s is located in either diskpath or "
				"cdpath.\nSDL error: %s",
				file,
				SDL_GetError()
			);

			Any_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "LEGO® Island Error", buffer, NULL);
			return FAILURE;
		}
	}
#endif

	return SUCCESS;
}

void IsleApp::DetectGameVersion()
{
	const char* file = "/lego/scripts/infocntr/infomain.si";
	SDL_PathInfo info;
	bool success = false;

	MxString path = MxString(m_hdPath) + file;
	path.MapPathToFilesystem();
	FILE* f = fopen(path.GetData(), "rb");
	if (!f) {
		path = MxString(m_cdPath) + file;
		path.MapPathToFilesystem();
		f = fopen(path.GetData(), "rb");
	}
	if (f) {
		fclose(f);
		success = true;
	} else {
		success = false;
	}

	assert(success);

	// File sizes of INFOMAIN.SI in English 1.0 and Japanese 1.0
	Lego()->SetVersion10(info.size == 58130432 || info.size == 57737216);

	if (Lego()->IsVersion10()) {
		SDL_Log("Detected game version 1.0");
		#ifndef __DREAMCAST__
		SDL_SetWindowTitle(reinterpret_cast<SDL_Window*>(m_windowHandle), "Lego Island");
#endif
	}
	else {
		SDL_Log("Detected game version 1.1");
	}
}

IDirect3DRMMiniwinDevice* GetD3DRMMiniwinDevice()
{
	LegoVideoManager* videoManager = LegoOmni::GetInstance()->GetVideoManager();
	if (!videoManager) {
		return nullptr;
	}
	Lego3DManager* lego3DManager = videoManager->Get3DManager();
	if (!lego3DManager) {
		return nullptr;
	}
	Lego3DView* lego3DView = lego3DManager->GetLego3DView();
	if (!lego3DView) {
		return nullptr;
	}
	TglImpl::DeviceImpl* tgl_device = (TglImpl::DeviceImpl*) lego3DView->GetDevice();
	if (!tgl_device) {
		return nullptr;
	}
	IDirect3DRMDevice2* d3drmdev = tgl_device->ImplementationData();
	if (!d3drmdev) {
		return nullptr;
	}
	IDirect3DRMMiniwinDevice* d3drmMiniwinDev = nullptr;
	if (!SUCCEEDED(d3drmdev->QueryInterface(IID_IDirect3DRMMiniwinDevice, (void**) &d3drmMiniwinDev))) {
		return nullptr;
	}
	return d3drmMiniwinDev;
}

void IsleApp::MoveVirtualMouseViaJoystick()
{
#ifdef __DREAMCAST__
	return;
#endif
	float dpadX = 0.0f;
	float dpadY = 0.0f;

	if (g_dpadLeft) {
		dpadX -= m_cursorSensitivity;
	}
	if (g_dpadRight) {
		dpadX += m_cursorSensitivity;
	}
	if (g_dpadUp) {
		dpadY -= m_cursorSensitivity;
	}
	if (g_dpadDown) {
		dpadY += m_cursorSensitivity;
	}

	// Use joystick axis if non-zero, else fall back to dpad
	float moveX = (g_lastJoystickMouseX != 0) ? g_lastJoystickMouseX : dpadX;
	float moveY = (g_lastJoystickMouseY != 0) ? g_lastJoystickMouseY : dpadY;

	if (moveX != 0 || moveY != 0) {
		g_mousemoved = TRUE;

		g_lastMouseX = SDL_clamp(g_lastMouseX + moveX, 0, g_targetWidth);
		g_lastMouseY = SDL_clamp(g_lastMouseY + moveY, 0, g_targetHeight);

		if (InputManager()) {
			InputManager()->QueueEvent(
				c_notificationMouseMove,
				g_mousedown ? LegoEventNotificationParam::c_lButtonState : 0,
				g_lastMouseX,
				g_lastMouseY,
				0
			);
		}

		SDL_ShowCursor(0);
		g_isle->SetDrawCursor(TRUE);
		if (VideoManager()) {
			VideoManager()->SetCursorBitmap(m_cursorCurrentBitmap);
			VideoManager()->MoveCursor(Min((MxS32) g_lastMouseX, 639), Min((MxS32) g_lastMouseY, 479));
		}
		IDirect3DRMMiniwinDevice* device = GetD3DRMMiniwinDevice();
		if (device) {
			Sint32 x, y;
			device->ConvertRenderToWindowCoordinates(g_lastMouseX, g_lastMouseY, x, y);
			g_mouseWarped = TRUE;
			SDL_WarpMouseInWindow(window, x, y);
		}
	}
}

#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
#ifndef _arch_dreamcast
void IsleApp::DetectDoubleTap(const SDL_TouchFingerEvent& p_event)
{
	typedef std::pair<Uint64, std::array<float, 2>> LastTap;

	const MxU32 doubleTapMs = 500;
	const float doubleTapDist = 0.001;
	static LastTap lastTap = {0, {0, 0}};

	LastTap currentTap = {p_event.timestamp, {p_event.x, p_event.y}};
	if (SDL_NS_TO_MS(currentTap.first - lastTap.first) < doubleTapMs &&
		DISTSQRD2(currentTap.second, lastTap.second) < doubleTapDist) {

		if (InputManager()) {
			InputManager()->QueueEvent(c_notificationKeyPress, SDLK_SPACE, 0, 0, SDLK_SPACE);
		}

		lastTap = {0, {0, 0}};
	}
	else {
		lastTap = currentTap;
	}
}
#endif
#endif
#endif

#ifdef __DREAMCAST__
extern "C" void SDL_DC_EmulateMouse(int);
extern "C" void SDL_DC_EmulateKeyboard(int);

int main(int argc, char* argv[]) {
	void* appstate = NULL;
	if (SDL_AppInit(&appstate, argc, argv) == SDL_APP_FAILURE) {
		SDL_AppQuit(appstate, SDL_APP_FAILURE);
		return 1;
	}

	// Disable SDL 1.2 built-in mouse & keyboard emulation to eliminate phantom clicks & motion
	SDL_DC_EmulateMouse(0);
	SDL_DC_EmulateKeyboard(0);
	SDL_ShowCursor(SDL_DISABLE);

	bool running = true;
	SDL_Joystick* dc_joy = NULL;
	if (SDL_NumJoysticks() > 0) {
		dc_joy = SDL_JoystickOpen(0);
		SDL_JoystickEventState(SDL_ENABLE);
		(void) dc_joy;
	}
	float dc_mouseX = 320.0f;
	float dc_mouseY = 240.0f;
	uint32_t prevButtons = 0;

	while (running) {
		// 1. Maple Controller Direct Polling
		maple_device_t* cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
		if (cont) {
			cont_state_t* st = (cont_state_t*) maple_dev_status(cont);
			if (st) {
				uint32_t pressed = st->buttons & ~prevButtons;
				prevButtons = st->buttons;

				if (pressed & CONT_START) {
					DebugMenu_Toggle();
				}
				else if (DebugMenu_IsOpen()) {
					DebugMenu_HandleInput(pressed, st->buttons);
				}
				else {
					g_leftTriggerHeld = (st->ltrig > 30);
					g_dreamcastJoyX = (abs(st->joyx) > 20) ? st->joyx : 0;
					g_dreamcastJoyY = (abs(st->joyy) > 20) ? st->joyy : 0;

					bool in3DView = (NavController() != nullptr);
					if (in3DView && !g_leftTriggerHeld) {
						// In full 3D island view:
						// Thumbstick navigates player directly via GetJoystickState().
						// Cursor remains stationary.
					}
					else {
						// In 2D scenes/menus, or Left Trigger held in 3D:
						// Thumbstick moves virtual cursor!
						if (g_dreamcastJoyX != 0 || g_dreamcastJoyY != 0) {
							dc_mouseX += (g_dreamcastJoyX / 128.0f) * 6.0f;
							dc_mouseY += (g_dreamcastJoyY / 128.0f) * 6.0f;
							if (dc_mouseX < 0) dc_mouseX = 0;
							if (dc_mouseX > 639) dc_mouseX = 639;
							if (dc_mouseY < 0) dc_mouseY = 0;
							if (dc_mouseY > 479) dc_mouseY = 479;

							SDL_Event m_evt;
							m_evt.type = SDL_MOUSEMOTION;
							m_evt.motion.state = SDL_GetMouseState(NULL, NULL);
							m_evt.motion.x = (int) dc_mouseX;
							m_evt.motion.y = (int) dc_mouseY;
							m_evt.motion.xrel = (int) ((g_dreamcastJoyX / 128.0f) * 6.0f);
							m_evt.motion.yrel = (int) ((g_dreamcastJoyY / 128.0f) * 6.0f);
							SDL_AppEvent(appstate, &m_evt);
						}
					}
				}
			}
		}

		// 2. SDL Event Queue Processing
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (DebugMenu_IsOpen()) {
				// While debug menu is open, suppress joystick and mouse events from reaching the game
				if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP ||
				    event.type == SDL_JOYAXISMOTION || event.type == SDL_JOYHATMOTION ||
				    event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN ||
				    event.type == SDL_MOUSEBUTTONUP) {
					continue;
				}
			}
			if (event.type == SDL_JOYBUTTONDOWN || event.type == SDL_JOYBUTTONUP) {
				// Button 3 is START on Dreamcast SDL 1.2 — ignore for mouse click
				if (event.jbutton.button == 3) {
					continue;
				}
				SDL_Event m_evt;
				m_evt.type = (event.type == SDL_JOYBUTTONDOWN) ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
				m_evt.button.button = (event.jbutton.button == 2) ? SDL_BUTTON_LEFT : ((event.jbutton.button == 1) ? SDL_BUTTON_RIGHT : 0);
				m_evt.button.state = (event.type == SDL_JOYBUTTONDOWN) ? SDL_PRESSED : SDL_RELEASED;
				m_evt.button.x = (int) dc_mouseX;
				m_evt.button.y = (int) dc_mouseY;
				if (m_evt.button.button != 0) {
					SDL_AppEvent(appstate, &m_evt);
				}
				continue;
			}
			if (SDL_AppEvent(appstate, &event) == SDL_APP_FAILURE) {
				running = false;
				break;
			}
		}

		// 3. Engine Frame Iteration
		if (SDL_AppIterate(appstate) == SDL_APP_FAILURE) {
			running = false;
		}
	}

	SDL_AppQuit(appstate, SDL_APP_SUCCESS);
	return 0;
}
#endif









