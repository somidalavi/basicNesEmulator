#ifndef CPU_INCLUDED
#define CPU_INCLUDED
#include "def.h"
#include "bus.h"

i32 CPU_nextCycle(void);
i32 CPU_init(void);
void CPU_IRQ();
void CPU_NMI();
void CPU_DMI();
void CPU_DMA();
#endif