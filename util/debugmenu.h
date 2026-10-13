#ifndef DEBUGMENU_H
#define DEBUGMENU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void DebugMenu_Init(void);
void DebugMenu_Toggle(void);
bool DebugMenu_IsOpen(void);
void DebugMenu_HandleInput(uint32_t pressed, uint32_t held);
void DebugMenu_Render(void);
void DebugMenu_RenderFpsOverlay(void);

#ifdef __cplusplus
}
#endif

#endif // DEBUGMENU_H
