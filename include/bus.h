#ifndef BUS_INCLUDED
#define BUS_INCLUDED

#include "def.h"
#include <stdio.h>
int BUS_insertMap(memMap map);
u8 BUS_read(u16 address);
u16 BUS_readU16(u16 address);
u16 BUS_readU16WrapAround(u16 address);
i32 BUS_writeU8(u16 address,u8 val);
i32 BUS_writeU16(u16 address,u16 val);
i32 BUS_DUMP();
void BUS_Init();
#endif