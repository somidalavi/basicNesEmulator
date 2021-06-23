#ifndef TEST6502_DEFS
#define TEST6502_DEFS

#define ST_CARRY 0x01
#define ST_ZERO  0x02
#define ST_INT   0x04
#define ST_DEC   0x08
#define ST_BRK2  0x10
#define ST_BRK1  0X20
#define ST_OVERF 0x40
#define ST_NEG   0x80   


#define SCR_MXX 256
#define SCR_MXY 240

//identifying operation type
enum AddressMode {
    MODE_IMMEDIATE,
    MODE_IMPLIED,
    MODE_ABSOLUTE,
    MODE_RELATIVE,
    MODE_ZEROPAGE,
    MODE_ZEROPAGEX,
    MODE_ABSOLUTEX,
    MODE_ABSOLUTEY,
    MODE_INDIRECTX,
    MODE_INDIRECTY,
    MODE_ACCUMULATOR,
    MODE_ZEROPAGEY,
    MODE_NONE           // used for undefined opcodes
};

//nametable mirroring type
enum mirrorType{ 
    MIR_VERTICAL,
    MIR_HORIZONTAL,
    MIR_FSCREEN
};

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;


// identifying operation type
enum Operation{
    OP_ASL,
    OP_ROL,
    OP_LSR,
    OP_ROR,
    OP_STX,
    OP_LDX,
    OP_DEC,
    OP_INC,
    OP_ORA,
    OP_AND,
    OP_EOR,
    OP_ADC,
    OP_STA,
    OP_LDA,
    OP_CMP,
    OP_SBC,
    OP_BIT,
    OP_JMP,
    OP_JMPA,
    OP_STY,
    OP_LDY,
    OP_CPY,
    OP_CPX,
    OP_PHP,
    OP_PLP,
    OP_PHA,
    OP_PLA,
    OP_DEY,
    OP_TAY,
    OP_INY,
    OP_INX,
    OP_CLC,
    OP_SEC,
    OP_CLI,
    OP_SEI,
    OP_TYA,
    OP_CLV,
    OP_CLD,
    OP_SED,
    OP_TXA,
    OP_TXS,
    OP_TAX,
    OP_TSX,
    OP_DEX,
    OP_NOP, //no Operation
    OP_BRK,
    OP_JSRA,
    OP_RTI,
    OP_RTS,
    OP_BRC, //used for all branch operations
    OP_NON  //No Operation Defined
};
// used for conditional branching
enum BranchOperation {
    B_NOBRANCH,
    B_NEG_1,
    B_OVERF_1,
    B_CARRY_1,
    B_ZERO_1,
    B_NEG_0,
    B_OVERF_0,
    B_CARRY_0,
    B_ZERO_0
};

typedef struct _Instruction{
    enum BranchOperation Btype;
    enum Operation op;
    enum AddressMode addrMode;
} Instruction;

typedef struct _ExecData{
    u8 bytes;
    u8 cycles;
} eData;        //Data for Execution
typedef struct _CompleteInstruction{
    struct _Instruction inst;
    struct _ExecData eData;
}   cInstruction;


typedef struct _memMap{
    u16 st,en;
    void  (*write) (u16 addr, u8 val);
    u8 (*read) (u16 addr); 
} memMap;
#endif