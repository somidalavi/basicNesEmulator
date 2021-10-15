#ifndef INSTDECODE_INCLUDED
#define INSTDECODE_INCLUDED
#include "def.h"

Instruction decodeInstruction(u8 opcode);
Instruction decode01(u8 opcode);
Instruction decode10(u8 opcode);
Instruction decode00(u8 opcode);
Instruction decode00Default(u8 opcode);
Instruction decodexxy1(u8 opcode);
eData getExecData(Instruction inst);
u8 isRMW(Instruction inst);
u8 writesToMemory(Instruction inst);

#endif