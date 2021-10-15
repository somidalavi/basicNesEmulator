#ifndef PPU_INCLUDED
#define PPU_INCLUDED
#include "def.h"

void PPU_setPattenTableMap(memMap mapper);
void PPU_draw();
void PPU_drawToBuffer(u8 buffer[SCR_MXY * SCR_MXX * 3]);
void PPU_Init();
void PPU_memWrite(u16 addr,u8 val);
u8 PPU_memRead(u16 addr);
#endif