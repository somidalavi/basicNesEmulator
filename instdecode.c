//for information look at http://nparker.llx.com/a2/opcodes.html#chart
#include "def.h"
#include "instdecode.h"
#include <stdio.h>

const u8 MASK_AAA  = 0xe0;
const u8 MASK_BBB  = 0x1c;
const u8 MASK_CC   = 0x03;
const u8 MASK_XXY  = 0xe0;
const u8 MASK_BOPS = 0x1f;   //Branch operations Mask (for identifing them)

Instruction newInstruction(enum BranchOperation btype,enum Operation op, enum AddressMode addr){
    Instruction inst = {btype,op,addr};
    return inst;
}
Instruction decodeInstruction(u8 opcode){


    //These are the exceptions that are handled first
    switch (opcode)
    {
    case 0x00:            //BRK
        return newInstruction(B_NOBRANCH,OP_BRK,MODE_IMPLIED);
    case 0x20:              //JSR abs
        return newInstruction(B_NOBRANCH,OP_JSRA,MODE_ABSOLUTE);
    case 0x40:              //RTI
        return newInstruction(B_NOBRANCH,OP_RTI,MODE_IMPLIED);
    case 0x60:              //RTS
        return newInstruction(B_NOBRANCH,OP_RTS,MODE_IMPLIED);
    case 0x08:              //PHP
        return newInstruction(B_NOBRANCH,OP_PHP,MODE_IMPLIED);
    case 0x28:              //PLP
        return newInstruction(B_NOBRANCH,OP_PLP,MODE_IMPLIED);
    case 0x48:              //PHA
        return newInstruction(B_NOBRANCH,OP_PHA,MODE_IMPLIED);
    case 0x68:              //PLA
        return newInstruction(B_NOBRANCH,OP_PLA,MODE_IMPLIED);      
    case 0x88:              //DEY
        return newInstruction(B_NOBRANCH,OP_DEY,MODE_IMPLIED);
    case 0xa8:              //TAY
        return newInstruction(B_NOBRANCH,OP_TAY,MODE_IMPLIED);
    case 0xc8:              //INY
        return newInstruction(B_NOBRANCH,OP_INY,MODE_IMPLIED);
    case 0xe8:              //INX  
        return newInstruction(B_NOBRANCH,OP_INX,MODE_IMPLIED);
    case 0x18:              //CLC
        return newInstruction(B_NOBRANCH,OP_CLC,MODE_IMPLIED);
    case 0x38:              //SEC
        return newInstruction(B_NOBRANCH,OP_SEC,MODE_IMPLIED);
    case 0x58:              //CLI
        return newInstruction(B_NOBRANCH,OP_CLI,MODE_IMPLIED);
    case 0x78:              //SEI
        return newInstruction(B_NOBRANCH,OP_SEI,MODE_IMPLIED);
    case 0x98:              //TYA
        return newInstruction(B_NOBRANCH,OP_TYA,MODE_IMPLIED);
    case 0xb8:              //CLV
        return newInstruction(B_NOBRANCH,OP_CLV,MODE_IMPLIED);
    case 0xd8:              //CLD
        return newInstruction(B_NOBRANCH,OP_CLD,MODE_IMPLIED);
    case 0xf8:              //SED
        return newInstruction(B_NOBRANCH,OP_SED,MODE_IMPLIED);
    case 0x8a:              //TXA
        return newInstruction(B_NOBRANCH,OP_TXA,MODE_IMPLIED);
    case 0x9a:              //TXS
        return newInstruction(B_NOBRANCH,OP_TXS,MODE_IMPLIED);
    case 0xaa:              //TAX
        return newInstruction(B_NOBRANCH,OP_TAX,MODE_IMPLIED);
    case 0xba:              //TSX
        return newInstruction(B_NOBRANCH,OP_TSX,MODE_IMPLIED);
    case 0xca:              //DEX
        return newInstruction(B_NOBRANCH,OP_DEX,MODE_IMPLIED);
    case 0xea:             //NOP
        return newInstruction(B_NOBRANCH,OP_NOP,MODE_IMPLIED);
    default:  break;
    // find it somewhere else
    }

    switch (opcode & MASK_CC)
    {
    case 0b01:
        return decode01(opcode);
        break;
    case 0b10:
        return decode10(opcode);
        break;
    case 0b00:
        return decode00(opcode);
        break;
    default:
        //do bad stuff if cc == 11
        //but for now:
        break;
    }
    Instruction inst = {B_NOBRANCH,OP_NON,MODE_NONE};
    return inst;
}
//general operations #1 : most used ones
Instruction decode01(u8 opcode){
    // aaa
    static const enum Operation OPE[] = {
        OP_ORA, // 000
        OP_AND, // 001 
        OP_EOR, // ...
        OP_ADC,
        OP_STA,
        OP_LDA,
        OP_CMP,
        OP_SBC // 111
    };
    // bbb
    static const enum AddressMode ADDR[] = {
        MODE_INDIRECTX, // 000
        MODE_ZEROPAGE,  // 0001
        MODE_IMMEDIATE, // ...
        MODE_ABSOLUTE,
        MODE_INDIRECTY,
        MODE_ZEROPAGEX,
        MODE_ABSOLUTEY,
        MODE_ABSOLUTEX, // 111
    };
    Instruction inst;
    inst.Btype = B_NOBRANCH;
    inst.op =  OPE[(opcode & MASK_AAA) >> 5];
    inst.addrMode = ADDR[(opcode & MASK_BBB) >> 2];
    return inst;
}
//general operations #2
Instruction decode10(u8 opcode){
    static const enum Operation OPE[] = {
        OP_ASL, // 000
        OP_ROL, // 001 
        OP_LSR, // ...
        OP_ROR,
        OP_STX,
        OP_LDX,
        OP_DEC,
        OP_INC  // 111
    };
    //bbb
    //None modes aren't defined
    static const enum AddressMode ADDR[] = {
        MODE_IMMEDIATE,     // 000
        MODE_ZEROPAGE,      // 001
        MODE_ACCUMULATOR,   // 010
        MODE_ABSOLUTE,      // 011
        MODE_NONE,          // 100
        MODE_ZEROPAGEX,     // 101
        MODE_NONE,          // 110
        MODE_ABSOLUTEX      // 111
    };
    Instruction inst;
    inst.Btype = B_NOBRANCH;
    inst.op =  OPE[(opcode & MASK_AAA) >> 5];
    inst.addrMode = ADDR[(opcode & MASK_BBB) >> 2];
    if (opcode == 0xb6 || opcode == 0x96) inst.addrMode = MODE_ZEROPAGEY;                      //the only instructions that use zero page y adressing 
    if (opcode == 0xbe ) inst.addrMode = MODE_ABSOLUTEY;                                      // for $BE absolute, x means aboslute, y addressing mode           
    return inst;
}
Instruction decode00Default(u8 opcode){
    //aaa
     static const enum Operation OPE[] = {
        OP_NON, // 000 this op is not defined
        OP_BIT, // 001 
        OP_JMP, // ...
        OP_JMPA,
        OP_STY,
        OP_LDY,
        OP_CPY,
        OP_CPX  // 111
    };
    //bbb
    //None modes aren't defined
    static const enum AddressMode ADDR[] = {
        MODE_IMMEDIATE, // 000
        MODE_ZEROPAGE,  // 001
        MODE_NONE,      // 010
        MODE_ABSOLUTE,  // 011
        MODE_NONE,      // 100
        MODE_ZEROPAGEX, // 101
        MODE_NONE,      // 110
        MODE_ABSOLUTEX  // 111
    };
    Instruction inst;
    inst.Btype = B_NOBRANCH;
    inst.op =  OPE[(opcode & MASK_AAA) >> 5];
    inst.addrMode = ADDR[(opcode & MASK_BBB) >> 2];
    return inst;
}
//other operations
Instruction decode00(u8 opcode){
    // Check branch Instructions
    if ( (opcode & MASK_BOPS) == 0b10000){
        return decodexxy1(opcode);
    }
    // for the rest of the instructions
    Instruction inst2 = decode00Default(opcode);
    if (inst2.addrMode != MODE_NONE && inst2.op != OP_NON) return inst2;

    // return dummy Instruction if none of the above
    return newInstruction(B_NOBRANCH,OP_NON,MODE_NONE);    
}

//Branch Operations
Instruction decodexxy1(u8 opcode){
    static const enum BranchOperation OPE[] = {
        B_NEG_0,    //000
        B_NEG_1,    //001
        B_OVERF_0,  //...
        B_OVERF_1,
        B_CARRY_0,
        B_CARRY_1,
        B_ZERO_0,
        B_ZERO_1    //111
    };
    Instruction inst;
    inst.Btype =  OPE[(opcode & MASK_XXY) >> 5];
    inst.op = OP_BRC;
    inst.addrMode =  MODE_RELATIVE;
    return inst;
}
u8 isStackOp(Instruction inst);
u8 isRMW(Instruction inst);
u8 writesToMemory(Instruction inst);

eData getExecData(Instruction inst){
    eData rData = {.bytes = 1,.cycles = 2};    //To be returned
    //Calculating number of read bytes and cycles for instruction
    switch (inst.addrMode)
    {
    case MODE_IMPLIED: rData.bytes = 1; rData.cycles =1; break;
    case MODE_ACCUMULATOR: rData.bytes = 1;rData.cycles = 1;break;
    case MODE_RELATIVE: rData.bytes = 2; rData.cycles = 2;break;
    case MODE_IMMEDIATE: rData.bytes = 2;rData.cycles = 2;break;
    case MODE_ZEROPAGEX: rData.bytes = 2;rData.cycles = 3;break;
    case MODE_ZEROPAGE: rData.bytes = 2;rData.cycles = 3;break;
    case MODE_INDIRECTX: rData.bytes = 2;rData.cycles = 6;break;
    case MODE_INDIRECTY: rData.bytes = 2;rData.cycles = 5;break;
    case MODE_ABSOLUTE:
    case MODE_ABSOLUTEX:
    case MODE_ABSOLUTEY:
        rData.bytes = 3;
        rData.cycles = 4;
        break;
    default:
        rData.bytes = 1;
        rData.cycles = 2;
        break;
    }
    //Special Cases Calculated
    
    if (isStackOp(inst)) rData.cycles = 3;
    
    if (rData.cycles == 1) rData.cycles = 2;
    
    if (inst.addrMode == MODE_ZEROPAGEX) rData.cycles++;

    //Rule #3 will be calculated at runtime or with (writesToMemory(inst))
    //Rule #4 will also be calculated at runtime
    
    if (isRMW(inst) && inst.addrMode != MODE_ACCUMULATOR)          //RMW = READ_MODIFY_WRITE
        rData.cycles++;

    if (isRMW(inst) && inst.addrMode == MODE_ABSOLUTEX)            //Speical case of Rule #5
        rData.cycles++;
    
    if (inst.op == OP_PLA || inst.op == OP_PLP)
        rData.cycles = 4;

    if (inst.op == OP_RTS || inst.op == OP_RTI)
        rData.cycles = 6;
    
    //The rest is handling exceptions that arise some of which i haven't checked why

    if (inst.op == OP_JSRA)
        rData.cycles = 6;
    
    if (inst.op == OP_BRK)
        rData.cycles = 7;
        
    if (inst.op == OP_JMPA)
        rData.cycles = 5;

    if (inst.op == OP_JMP)
        rData.cycles = 3;

    if (isRMW(inst) &&  inst.addrMode != MODE_ACCUMULATOR) 
        rData.cycles++;

    if (inst.op == OP_STA && (inst.addrMode == MODE_INDIRECTY || inst.addrMode == MODE_ABSOLUTEY || inst.addrMode == MODE_ABSOLUTEX))
        rData.cycles++;

    if (inst.addrMode == MODE_ZEROPAGEY){
        rData.cycles = 4;
        rData.bytes = 2;
    }
    return rData;
}
u8 isStackOp(Instruction inst){
    switch (inst.op)
    {
    case OP_PLA:
    case OP_PLP:
    case OP_PHA:
    case OP_PHP:
        return 1;
    default:
        return 0;
    }
}
u8 isRMW(Instruction inst){
    switch (inst.op)
    {
    case OP_ASL:
    case OP_DEC:
    case OP_INC:
    case OP_LSR:
    case OP_ROL:
    case OP_ROR:
        return 1;
    default:
        return 0;
    }
}
u8 writesToMemory(Instruction inst){
    if (isRMW(inst) && inst.addrMode != MODE_ACCUMULATOR) return 1;
    if (inst.op == OP_PHP || inst.op == OP_PHA) return 1;
    if (inst.op == OP_STA || inst.op == OP_STX || inst.op == OP_STY) return 1;
    return 0;
}