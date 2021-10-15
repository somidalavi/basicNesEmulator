#include "def.h"
#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "instdecode.h"
#include <err.h>

// STATE_FETCH: at the start of reading an instruction STATE_EXECUTE means in middle
static enum {
    STATE_FETCH,
    STATE_EXECUTE
} curState;

//Registers
static struct{
    u8 A;
    u8 X;
    u8 Y;
    u16 PC;
    u8 SP;
    u8 ST;
} RS;


static cInstruction instructions[256];

static cInstruction curInst;
static u8 curCycle;
static u8 curOpCode;
static u8 curOps[2];
static u32 shouldWait;

i32 CPU_init(){
    curState = STATE_FETCH;
    
    //initial value for registers
    RS.A = 0x00;
    RS.X = 0x00;
    RS.Y = 0x00;
    RS.PC = BUS_readU16(0xfffc);        
    RS.ST = 0x24;
    RS.SP = 0xFD;                   //normally ff but i think nes has it as fd at start

    for (u16 opcode = 0;opcode <= 255;opcode++){
        Instruction inst = decodeInstruction(opcode);
        eData edata = getExecData(inst);
        instructions[opcode].eData = edata;
        instructions[opcode].inst = inst;
    }
}

// sets processor flag according to a flag
static inline void SETST(u8 flag,u8 CPUFLAG){
    if (flag) RS.ST |= (CPUFLAG);
    else RS.ST &= (0xff) - (CPUFLAG);
}
static inline void CHKR(u8 reg){
    SETST(reg == 0,ST_ZERO);
    SETST(reg & 0x80,ST_NEG);
}
static inline void TRR(u8 src, u8 *dst){
    *dst = src;
    SETST((*dst) == 0,ST_ZERO);
    SETST((*dst) & (1 << 7),ST_NEG);
}
static inline void AND(u16 address){
    if (curInst.inst.addrMode == MODE_IMMEDIATE) RS.A &= curOps[0];
    else RS.A &= BUS_read(address);
    SETST(RS.A == 0,ST_ZERO);
    SETST(RS.A & 0x80,ST_NEG);
}
static inline void ASL(u16 address){
    if (curInst.inst.addrMode == MODE_ACCUMULATOR){
        SETST(RS.A & 0x80,ST_CARRY);
        RS.A <<= 1;
        CHKR(RS.A);
    }else {
        u8 op = BUS_read(address);
        SETST(op & 0x80,ST_CARRY);
        BUS_writeU8(address,op << 1);
        CHKR(op << 1);
    }
}
// check if a branch suceedes
static inline u8 CPU_checkBranch(){
    switch (curInst.inst.Btype)
    {
    case B_NEG_0 : return (RS.ST & ST_NEG) == 0; 
    case B_NEG_1: return (RS.ST & ST_NEG);
    case B_CARRY_1: return (RS.ST & ST_CARRY);
    case B_CARRY_0 : return (RS.ST & ST_CARRY) == 0;
    case B_ZERO_1: return (RS.ST & ST_ZERO);
    case B_ZERO_0: return (RS.ST & ST_ZERO) == 0;
    case B_OVERF_0: return (RS.ST & ST_OVERF) == 0;
    case B_OVERF_1 : return (RS.ST & ST_OVERF);
    default:
        return 0;
        break;
    }
}
static inline void BRC(u16 newPc){
    u8 weBranch = CPU_checkBranch();
    if (weBranch){
        shouldWait += 1;
        if ( ( newPc >> 8 ) != (RS.PC >> 8)) shouldWait += 2;   //wait additional cycles if crossing page
        RS.PC = newPc;
    }
}
const u16 stackOffset = 0x100;
static inline void PSH(u8 rs){
    BUS_writeU8(RS.SP + stackOffset,rs);
    RS.SP--;
}
static inline void PUL(u8 *reg){
    RS.SP++;
    (*reg) = BUS_read(RS.SP + stackOffset);
}
static inline void BRK(){
    u16 tmpPc = RS.PC;
    PSH((tmpPc >> 8) & 0xff);
    PSH((tmpPc) & 0xff);
    PSH(RS.ST | ST_BRK1 | ST_BRK2);
    RS.PC = BUS_readU16(0xfffe);
    SETST(1,ST_INT);
}
static inline void CMP(u8 *reg,u16 addr){
    u8 op2;
    if (curInst.inst.addrMode == MODE_IMMEDIATE) op2 = curOps[0];
    else op2 = BUS_read(addr);
    u8 res = (*reg) + ((~op2) + 1);
    SETST(((u8)(*reg)) >= ((u8)op2),ST_CARRY);
    SETST(res == 0,ST_ZERO);
    SETST(res & 0x80,ST_NEG);
}
static inline void DEC(u16 address){
    u8 res;
    res = BUS_read(address);
    res--;
    BUS_writeU8(address,res);
    SETST(res == 0,ST_ZERO);
    SETST(res & 0x80,ST_NEG);
}
static inline void DECR(u8 *dst){
    (*dst)--;
    SETST((*dst) == 0 ,ST_ZERO);
    SETST((*dst) & 0x80,ST_NEG);
}
static inline void EOR(u16 addr){
    u8 op;
    if (curInst.inst.addrMode == MODE_IMMEDIATE) op = curOps[0];
    else op = BUS_read(addr);
    RS.A ^= op;
    SETST(RS.A == 0,ST_ZERO);
    SETST(RS.A & 0x80,ST_NEG);
}
static inline void INC(u16 addr){
    u8 res;
    res = BUS_read(addr);
    res++;
    BUS_writeU8(addr,res);
    SETST(res == 0,ST_ZERO);
    SETST(res & 0x80,ST_NEG);
}
static inline void INCR(u8 *dst){
    (*dst)++;
    SETST((*dst) == 0 ,ST_ZERO);
    SETST((*dst) & (1 << 7),ST_NEG);
}
static inline void LSR(u16 address){
    u8 res,bit0;
    if (curInst.inst.addrMode == MODE_ACCUMULATOR) res = RS.A;
    else res = BUS_read(address);
    bit0 = res & 1;
    res >>= 1;
    if (curInst.inst.addrMode == MODE_ACCUMULATOR) RS.A = res;
    else BUS_writeU8(address,res);
    SETST(bit0 , ST_CARRY);
    SETST(res == 0,ST_ZERO);
    SETST(res & 0x80,ST_NEG);
}
static inline void ORA(u16 addr){
    u8 op;
    if (curInst.inst.addrMode == MODE_IMMEDIATE) op = curOps[0];
    else op = BUS_read(addr);
    RS.A |= op;
    SETST(RS.A == 0,ST_ZERO);
    SETST(RS.A & 0x80,ST_NEG);
}
static inline void JSR(u16 addr){
    u16 tmp = RS.PC - 1;
    PSH( (tmp >> 8) & 0xff);
    PSH(  tmp       & 0xff);
    RS.PC = addr;
}
static inline void BIT(u16 address){
    u8 val = BUS_read(address);
    SETST((val & RS.A) == 0,ST_ZERO);
    SETST(val & (1 << 6),ST_OVERF);
    SETST(val & (1 << 7),ST_NEG);
}
u16 CPU_resolveAddressing(){
    switch (curInst.inst.addrMode)
    {
    case MODE_IMPLIED:             
    case MODE_ACCUMULATOR: return 0;        // implied and accumulator are used inside the operation execution themselves
    case MODE_RELATIVE:
        if (curOps[0] & 0x80) return RS.PC + (0xff00 | ((u16)curOps[0]));
        else return RS.PC + curOps[0];
    case MODE_IMMEDIATE: return 0;
    case MODE_ZEROPAGEX: return (curOps[0] + RS.X) & 0xff; 
    case MODE_ZEROPAGEY: return (curOps[0] + RS.Y) & 0xff;
    case MODE_ZEROPAGE:  return curOps[0];
    case MODE_INDIRECTX: return BUS_readU16(((u8)(curOps[0] + RS.X)));
    case MODE_INDIRECTY: return BUS_readU16(curOps[0]) +  RS.Y;
    case MODE_ABSOLUTE:
        return (((u16)curOps[1]) << 8) | ((u16)curOps[0]); 
    case MODE_ABSOLUTEX:
        return ((((u16)curOps[1]) << 8) | ((u16)curOps[0])) + RS.X;
    case MODE_ABSOLUTEY:
        return ((((u16)curOps[1]) << 8) | ((u16)curOps[0]) )+ RS.Y;
    }
}
static inline void ADD(u8 op){
    //i just copied this but know that it is just a lot of corner case checking
    u8 oldVal = RS.A;
    u8 newVal = oldVal;
    u8 overfFlag = 0;
    newVal += op;
    if (newVal < oldVal) overfFlag = 1;
    u8 oldVal2 = newVal;
    newVal += RS.ST & ST_CARRY;
    if (newVal < oldVal2) overfFlag = 1;
    RS.A = newVal;
    SETST(((oldVal & 0x80) == (op & 0x80)) && ((oldVal & 0x80 ) != (newVal & 0x80)),ST_OVERF);     //Don't worry about it i just copied    
    SETST(overfFlag,ST_CARRY);
    SETST(RS.A == 0,ST_ZERO);
    SETST(RS.A & (1 << 7),ST_NEG);
}
static inline void SBC(u16 addr){
    u8 op;
    if (curInst.inst.addrMode == MODE_IMMEDIATE)  op = curOps[0];
    else op = BUS_read(addr);
    ADD(~op);
}
static inline void ADC(u16 address){
    u8 op;
    if (curInst.inst.addrMode == MODE_IMMEDIATE) op = curOps[0];
    else op = BUS_read(address);
    ADD(op);
}
static inline void RTS(){
    u8 fByte,sByte;
    PUL(&fByte);
    PUL(&sByte);
    u16 tmp = sByte;
    tmp <<= 8;
    tmp += fByte;
    RS.PC = tmp + 1;
}
static inline void RTI(){
    //first part is just like PHP we ignore two of the bits
    u8 by;
    PUL(&by);
    RS.ST |= (by & 0xcf); // cf = 0b11001111
    // and the rest is RTS() withe exception of minus 1 rule
    u8 fByte,sByte;
    PUL(&fByte);
    PUL(&sByte);
    u16 tmp = sByte;
    tmp <<= 8;
    tmp += fByte;
    RS.PC = tmp;
}
static inline void LDR(u8 *reg,u16 address){
    if (curInst.inst.addrMode == MODE_IMMEDIATE) (*reg) = curOps[0];
    else (*reg) = BUS_read(address);
    SETST((*reg) == 0,ST_ZERO);
    SETST((*reg) & (1 << 7),ST_NEG);
}
static inline void ROR(u16 addr){
    u8 res;
    u8 bit0;
    if (curInst.inst.addrMode == MODE_ACCUMULATOR) res = RS.A;
    else res = BUS_read(addr);

    bit0 = res & 1;
    res >>= 1;
    res |= (RS.ST & ST_CARRY) << 7;
    
    if (curInst.inst.addrMode == MODE_ACCUMULATOR) RS.A = res;
    else BUS_writeU8(addr,res);
    
    SETST(bit0 , ST_CARRY);
    SETST(res == 0,ST_ZERO);
    SETST(res & 0x80,ST_NEG);
}
static inline void ROL(u16 addr){
    u8 res;
    u8 bit7;
    if (curInst.inst.addrMode == MODE_ACCUMULATOR) res = RS.A;
    else res = BUS_read(addr);
    bit7 = res & 0x80;
    res <<= 1;
    res |= RS.ST & ST_CARRY;
    if (curInst.inst.addrMode == MODE_ACCUMULATOR) RS.A = res;
    else BUS_writeU8(addr,res);
    SETST(bit7 , ST_CARRY);
    SETST(res == 0,ST_ZERO);
    SETST(res & 0x80,ST_NEG);
}
static u32 cycleCount;
void CPU_checkForAdditionalCycles(u16 addr){
    if (writesToMemory(curInst.inst)) return ; //if op writes to memory there will be  no more addtional cycles
    //check if instructions cross page boundaries
    switch (curInst.inst.addrMode)
    {
    case MODE_ABSOLUTEX:
        if (((addr - RS.X) >> 8) != (addr >> 8)) shouldWait++;
        break;
    case MODE_ABSOLUTEY:
    case MODE_INDIRECTY:
        if (((addr - RS.Y) >> 8) != (addr >> 8)) shouldWait++;
        break;
    }
}

i32 CPU_executeInstruction(){
    if (curInst.inst.op == OP_JMPA){
        u16 address = BUS_readU16((((u16)curOps[1]) << 8) | ((u16)curOps[0]));
        RS.PC = address;
        cycleCount += shouldWait + curInst.eData.cycles;
        return 0;
    }
    u16 address = CPU_resolveAddressing();       
    CPU_checkForAdditionalCycles(address);      //doesn't include additional cycles from branch instructions those are calculated in BRC()
    u8 by;                  // used by OP_PLP
    switch (curInst.inst.op)
    {
    case OP_TYA: TRR(RS.Y,&RS.A);break;
    case OP_TXS: RS.SP = RS.X;break;               //special case doesn't change status flag
    case OP_TXA: TRR(RS.X,&RS.A);break;
    case OP_TSX: TRR(RS.SP,&RS.X);break;
    case OP_TAY: TRR(RS.A,&RS.Y);break;
    case OP_TAX: TRR(RS.A,&RS.X);break;
    case OP_STY: BUS_writeU8(address,RS.Y);break;
    case OP_STX: BUS_writeU8(address,RS.X);break;
    case OP_STA: BUS_writeU8(address,RS.A);break;
    case OP_LDX: LDR(&RS.X,address);break;
    case OP_LDY: LDR(&RS.Y,address);break;
    case OP_LDA: LDR(&RS.A,address);break;
    case OP_SEI: RS.ST |= (ST_INT);break;
    case OP_SED: RS.ST |= (ST_DEC);break;
    case OP_SEC: RS.ST |= (ST_CARRY);break;
    case OP_ADC: ADC(address);break;
    case OP_AND: AND(address);break;
    case OP_ASL: ASL(address);break;
    case OP_BRC: BRC(address);break;
    case OP_BIT: BIT(address);break;
    case OP_BRK: BRK();break;
    case OP_CLC: RS.ST &= (0xff) - (ST_CARRY);break;
    case OP_CLD: RS.ST &= (0xff) - (ST_DEC);break;
    case OP_CLI: RS.ST &= (0xff) - (ST_INT);break;
    case OP_CLV: RS.ST &= (0xff) - (ST_OVERF);break;
    case OP_CMP: CMP(&RS.A,address);break;
    case OP_CPX: CMP(&RS.X,address);break;
    case OP_CPY: CMP(&RS.Y,address);break;
    case OP_DEC: DEC(address);break;
    case OP_DEX: DECR(&RS.X);break;
    case OP_DEY: DECR(&RS.Y);break;
    case OP_EOR: EOR(address);break;
    case OP_INC: INC(address);break;
    case OP_INX: INCR(&RS.X);break;
    case OP_INY: INCR(&RS.Y);break;
    case OP_JMP: RS.PC = address;break;
    case OP_JSRA: JSR(address);break;
    case OP_RTS: RTS();break;
    case OP_NOP: break;
    case OP_LSR: LSR(address);break;
    case OP_ORA: ORA(address);break;
    case OP_PHA: PSH(RS.A);break;
    case OP_PHP: PSH(RS.ST | ST_BRK1 | ST_BRK2);break;
    case OP_PLA: PUL(&RS.A); SETST(RS.A == 0,ST_ZERO);SETST(RS.A & 0x80,ST_NEG);break;
    case OP_SBC: SBC(address);break;
    case OP_PLP:                    //has special pull specdification must not touch ST_BRK1 and ST_BRK2 bits
        PUL(&by);
        RS.ST = (by & 0xcf);        // 0xcf = 0xb11001111
        break;
    case OP_RTI: RTI();break;
    case OP_ROR: ROR(address);break;
    case OP_ROL: ROL(address);break;
    default:
        printf("Not known opcode %d %x\n",RS.PC - 0x8000,curOpCode);
        return -1;
        break;
    }
    cycleCount += shouldWait + curInst.eData.cycles;
    return 0;
}
static u8 nmiInt,irqInt;

//artificially wait for dma to happen ( really one happens instantly in one go)
void CPU_DMA(){ shouldWait += 512; }

// rembers so it can handle it after the current instructions finishes
void CPU_NMI(){   nmiInt = 1; }

void CPU_IRQ(){   irqInt = 1; }

static inline void internalNMI(){
    u16 tmpPc = RS.PC;
    PSH((tmpPc >> 8) & 0xff);
    PSH((tmpPc) & 0xff);
    // sets ST_BRK1 to 1 and ST_BRK2 to 0
    PSH((RS.ST | ST_BRK1) & 0xef);
    RS.PC = BUS_readU16(0xfffA);
}
static inline void internalIRQ(){
    u16 tmpPc = RS.PC;
    PSH((tmpPc >> 8) & 0xff);
    PSH((tmpPc) & 0xff);
    // sets ST_BRK1 to 1 and ST_BRK2 to 0
    PSH((RS.ST | ST_BRK1) & 0xef);
    RS.PC = BUS_readU16(0xfffe);
}
i32 CPU_nextCycle(){


    // used to wait extra cycles if certain stuff happens
    if (shouldWait){
        shouldWait--;
        return 1;
    }
    // Crude way to handle interrupts (just fetch vector in 1 cycle and then wait for 6 more)
    if (nmiInt && curState == STATE_FETCH){
        shouldWait = 6;
        nmiInt = 0;
        internalNMI();
        return 1;
    }
    // Same way for irq interrupts but also checking for interrupt mask
    if (irqInt && curState == STATE_FETCH && (RS.ST & (ST_INT) == 0)){
        shouldWait = 6;
        irqInt = 0;
        internalIRQ();
        return 1;
    }

    if (curState == STATE_FETCH){
        curOpCode = BUS_read(RS.PC++);
        curInst = instructions[curOpCode];
        curCycle = 1;
        curState = STATE_EXECUTE;
      //  fprintf(stderr,"%d : A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%d Opcode:%02X PC:%04X\n",cnt++,RS.A,RS.X,RS.Y , RS.ST,RS.SP,cycleCount + 7,curOpCode,RS.PC - 1);
    }else{
        if (curCycle < curInst.eData.bytes)
            curOps[curCycle - 1] = BUS_read(RS.PC++);
        
        curCycle++;

        if (curCycle > curInst.eData.cycles){
            CPU_executeInstruction();
            curState = STATE_FETCH;
        }
    }
    return 1;   
}
