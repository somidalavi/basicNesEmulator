#ifndef CONTROLLER_INCLUDED
#define CONTROLLER_INCLUDED
#include "def.h"


void CONTROLLER_Init();
void CONTROLLER_keyDown(u8 key);
void CONTROLLER_keyUp(u8 key);
u8 CONTROLLER_getKeyState();
#endif