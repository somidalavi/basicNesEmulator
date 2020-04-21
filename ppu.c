#include "def.h"
#include <stdio.h>
#include <assert.h>
#include "bus.h"
#include "ppu.h"
#include "cpu.h"
#include <memory.h>
typedef struct _pixel{
    union {
        u8 chn[3];//by channels

        struct
        {
            u8 r,g,b;       // by color type
        };
    
    };
} pixel;

#define SCR_MXX 256
#define SCR_MXY 240

#define VBLANKMASK 0x80
#define S0HITMASK 0x40
#define SOVERFMASK 0x20

static enum mirrorType mirtype;
static u8 camX,camY;
static u8 screen[2 * SCR_MXX][2 * SCR_MXY];
static u8 pallete[2][16];
static u8 nameTable[4][0x400];
static u8 OAM[256];

static memMap patternTable;

// internal registers

u8 intLatch;
u8 addrTog;         //address togle used for scroll and ppu adress registers
u16 addrReg;        //used for both addres and scroll registers
u8 vRamInc = 1;     // the value of the address that is incremented with each read of write from memory either 1 (horizontal) or 32 (horizontal)
u8 bgEnable;        // enables background drawing
u8 spEnable;        // enables sprite drawing
u8 oamAddr;         // current address for oam writing
u8 bgPatternTable;  // which pattern table is used for background tiles
u8 spPatternTable;  // Same as previous line this tiem for sprites
static const u8 F_vblank = 0x80;    //since cpu and ppu are synchronized this happens (it stays constant and 1 throughtout);
static u8 F_spr0Hit = 0x40;
static u8 F_sprOverf = 0x20; 


static inline void setByte(u16 *val,u16 val2,u8 tog){
    if (tog){
        (*val) &= 0xff; //first set the second byte to zero
        (*val) |= (val2 << 8);
    }else {
        (*val) &= 0xff00;    //set the first byte to zero
        (*val) |= (val2);
    }   
}


u8 PPU_regRead(u16 addr){
    addr &= 0x7;
    u8 w;   //used for PPUSTATUS
    switch (addr)
    {
    case 0x2:   //PPU Status
        w = (F_vblank) | (F_spr0Hit) | (F_spr0Hit);
        addrTog = 1;    //address and scroll register pairs resset 
        // we may need to add addReg[0] = 0;addReg[1] = 0;
        intLatch = w;    
        return w;
    case 0x4:
        w =  OAM[oamAddr];
        intLatch = w;
        return w;
    case 0x7:
        w =  PPU_memRead(addrReg);
        intLatch = w;
        return w;
    case 0x6:   //PPUADDR   reading from write only registers returns the internal latch
    case 0x5:   //PPUSCROLL
    case 0x0:   //PPUCTRL
    case 0x1:   //PPUMASK
    case 0x3:   //OAMDADDR
        return intLatch;
    default:
        return 0;
        break;
    }
}
void PPU_regWrite(u16 addr,u8 val){
    intLatch |= val;    //writing to any registers sets the internal latch
    addr &= 0x7;
    switch (addr)
    {
    case 0x0:
        if (val & (1 << 2)) vRamInc = 32;
        else vRamInc = 1;
        if (val & (1 << 4)) bgPatternTable = 1;
        else bgPatternTable = 0;
        break;
    case 0x1:
        if (val & (1 << 3)) bgEnable = 1;
        else bgEnable = 0;
        if (val & (1 << 4)) spEnable = 1;
        else spEnable = 0;
        break;
    case 0x3:
        oamAddr = val;
        break;
    case 0x4:
        OAM[oamAddr++] = val;
        break;
    case 0x6:       //PPUADDR
    case 0x5:       //PPUSCROLL    
        setByte(&addrReg,val,addrTog);  //sets the second byte if addrTog = 1 otherwise byte 0
        addrTog ^= 1;
        break;
    case 0x7:       //PPUDATA
        PPU_memWrite(addrReg,val);
        addrReg += vRamInc;
        break;
    default:
        break;
    }
    return ;
}
void PPU_DMA(u16 addr,u8 val){
    CPU_DMA();
    u16 vald = val;
    for (u16 i = 0;i < 0xff;i++){
        OAM[i] = BUS_read((vald << 8) | i);
    }
}
void PPU_Init(){
    pallete[0][2] = 1;
    addrTog = 1;
    memMap ppuRegMap;
    ppuRegMap.st = 0x2000;
    ppuRegMap.en = 0x4000 - 1;
    ppuRegMap.read = PPU_regRead;
    ppuRegMap.write = PPU_regWrite;
    BUS_insertMap(ppuRegMap);
    memMap oamDma;
    oamDma.st = 0x4014;
    oamDma.en = 0x4014;
    oamDma.write = PPU_DMA;
    BUS_insertMap(oamDma);
}

static pixel nes_palette[64] =
{
   {0x80,0x80,0x80}, {0x00,0x00,0xBB}, {0x37,0x00,0xBF}, {0x84,0x00,0xA6},
   {0xBB,0x00,0x6A}, {0xB7,0x00,0x1E}, {0xB3,0x00,0x00}, {0x91,0x26,0x00},
   {0x7B,0x2B,0x00}, {0x00,0x3E,0x00}, {0x00,0x48,0x0D}, {0x00,0x3C,0x22},
   {0x00,0x2F,0x66}, {0x00,0x00,0x00}, {0x05,0x05,0x05}, {0x05,0x05,0x05},

   {0xC8,0xC8,0xC8}, {0x00,0x59,0xFF}, {0x44,0x3C,0xFF}, {0xB7,0x33,0xCC},
   {0xFF,0x33,0xAA}, {0xFF,0x37,0x5E}, {0xFF,0x37,0x1A}, {0xD5,0x4B,0x00},
   {0xC4,0x62,0x00}, {0x3C,0x7B,0x00}, {0x1E,0x84,0x15}, {0x00,0x95,0x66},
   {0x00,0x84,0xC4}, {0x11,0x11,0x11}, {0x09,0x09,0x09}, {0x09,0x09,0x09},

   {0xFF,0xFF,0xFF}, {0x00,0x95,0xFF}, {0x6F,0x84,0xFF}, {0xD5,0x6F,0xFF},
   {0xFF,0x77,0xCC}, {0xFF,0x6F,0x99}, {0xFF,0x7B,0x59}, {0xFF,0x91,0x5F},
   {0xFF,0xA2,0x33}, {0xA6,0xBF,0x00}, {0x51,0xD9,0x6A}, {0x4D,0xD5,0xAE},
   {0x00,0xD9,0xFF}, {0x66,0x66,0x66}, {0x0D,0x0D,0x0D}, {0x0D,0x0D,0x0D},

   {0xFF,0xFF,0xFF}, {0x84,0xBF,0xFF}, {0xBB,0xBB,0xFF}, {0xD0,0xBB,0xFF},
   {0xFF,0xBF,0xEA}, {0xFF,0xBF,0xCC}, {0xFF,0xC4,0xB7}, {0xFF,0xCC,0xAE},
   {0xFF,0xD9,0xA2}, {0xCC,0xE1,0x99}, {0xAE,0xEE,0xB7}, {0xAA,0xF7,0xEE},
   {0xB3,0xEE,0xFF}, {0xDD,0xDD,0xDD}, {0x11,0x11,0x11}, {0x11,0x11,0x11}
};

// return in what quadrant (virtual table name) the pixel in (x,y) is
static inline u8 getVirtualNametable(u16 x,u16 y ){
    u8 virtualNametable = 0;
    virtualNametable |= ((x >= 256));
    virtualNametable |= (y >= 240) << 1;
    return virtualNametable;
}
//calculate which physical namespace maps to a certain virtual addressable
static inline u8 getNameTMirtype(u8 virtualNameT){
    switch (mirtype)
    {
    case MIR_HORIZONTAL:   return virtualNameT >> 1;
    case MIR_VERTICAL:   return virtualNameT & 0x1;
    case MIR_FSCREEN:   return virtualNameT;
    default:    return 0;
    }
}
static inline u8 getPhysicalNametable(u16 i,u16 j){
    u8 virtualNametable = getVirtualNametable(i,j);
    return getNameTMirtype(virtualNametable);
}
static void translatePatternTable(u16 startAddr,u8 resTable[8][8]){
    u8 pTable1[8],pTable2[8];
    for (u8 i = 0;i < 8;i++)
        pTable1[i] = patternTable.read(startAddr + i);

    for (u8 i = 0;i < 8;i++)
        pTable2[i] = patternTable.read(startAddr + 8 + i);
    
    for (u8 i = 0;i < 8;i++){
        for (u8 j = 0;j < 8;j++){
            resTable[i][7 - j] = (pTable1[i] & (1 << j)) > 0;
            resTable[i][7 - j] += ((pTable2[i] & (1 << j)) > 0) << 1;
        }
    }
}
static void drawTile(u16 row,u16 col){
   
    u8 resTable[8][8];
    u8 nameT = getPhysicalNametable(col * 8,row * 8);
    u8 virtNameT = getVirtualNametable(col * 8,row * 8);     
    u16 nRow = row ,nCol = col;
    if (virtNameT & 2) nRow -= 30;
    if (virtNameT & 1) nCol -= 32;

    u16 pTableAddr = nameTable[nameT][nRow * 32 + nCol]; 
    u16 stR = row * 8;
    u16 stC = col * 8;

    // translate patterntables in to a result tile
    translatePatternTable(pTableAddr * 16 + (bgPatternTable * 0x1000),resTable);

    for (u16 x = 0;x < 8;x++){
        for (u16 y = 0;y < 8;y++){

            screen[stC + x][stR + y] =  resTable[y][x];
        }
    }
}
static void drawAttributes(u16 row,u16 col){
    u8 nameT = getPhysicalNametable(col * 32,row * 32);
    u8 virtNameT = getVirtualNametable(col * 32,row * 32);
    u16 nRow = row ,nCol = col;
    if (virtNameT & 2) nRow -= 8;
    if (virtNameT & 1) nCol -= 8;
    u16 stR = row * 32;
    u16 stC = col * 32;
    u8 attrib = nameTable[nameT][0x3c0 + row * 0x08 + col];
    u8 highBits;
    // draw the top left
    highBits = (attrib & 0x03) << 2;
    for (u16 x = stC;x < stC + 16;x++){
        for (u16 y = stR;y < stR + 16;y++){
            screen[x][y] += highBits;
        }
    }
    // draw the top right
    highBits = (attrib & 0xc);
    for (u16 x = stC + 16;x < stC + 32;x++){
        for (u16 y = stR;y < stR + 16;y++){
            screen[x][y] += highBits;
        }
    }

    if (row == 8) return ; //last row doesn't have a bottom half
   
    //draw the bottom left

    highBits = (attrib & 0x30) >> 2;

    for (u16 x = stC;x < stC + 16;x++){
        for (u16 y = stR + 16;y < stR + 32;y++){
            screen[x][y] += highBits;
        }
    }
    //draw the bottom right
    highBits = (attrib & 0xc0) >> 4;
    for (u16 x = stC + 16;x < stC + 32;x++){
        for (u16 y = stR + 16;y < stR + 32;y++){
            screen[x][y] += highBits;
        }
    }
}
static void drawBackground(){
    for (u16 r = 0;r < 60;r++){
        for (u16 c = 0;c < 64;c++){
            drawTile(r,c);
        }
    }
    for (u16 c = 0;c < 8;c++){
        for (u16 r = 0;r < 8;r++){
           drawAttributes(r,c);
        }
    }
}
static void flipH(u8 restT[8][8]){
    u8 tmp[8][8];
    for (int i = 0;i < 8;i++)
        for (int j = 0;j < 8;j++)
            tmp[i][7 - j] = restT[i][j];
    memcpy(restT,tmp,sizeof(tmp));
}
static void flipV(u8 restT[8][8]){
    u8 tmp[8][8];
    for (int i = 0;i < 8;i++)
        for (int j = 0;j < 8;j++)
            tmp[7 - i][j] = restT[i][j];
    memcpy(restT,tmp,sizeof(tmp));
}
static void drawSprites(){
    u8 resTable[8][8];
    for (u8 i = 0;i < 64;i++){
        u8 OAMst = i * 4;
        u16 yCor = OAM[OAMst];yCor++;
        u16 index = OAM[OAMst + 1];
        translatePatternTable(index * 16 + (spPatternTable * 0x1000),resTable);
        u16 attrib = OAM[OAMst + 2];
        u16 xCor   = OAM[OAMst + 3];
        u16 palette = attrib & 3;
        //printf("sprite is at %d %d\n",yCor,xCor);
        if (attrib & (1 << 6)) flipH(resTable);
        if (attrib & (1 << 7)) flipV(resTable);
        for (u16 x = xCor;x < xCor + 8;x++){
            for (u16 y = yCor;y < yCor + 8;y++){
                if (resTable[y - yCor][x - xCor] == 0 || (attrib & ( 1 << 5))) continue; 
                else screen[x][y] = resTable[y- yCor][x -xCor] + 16 + 4 * palette;
            }
        }
    }
    //printf("End of sprite\n");
    return ;
}
void PPU_draw(){
    intLatch = 0;   //reset the internal latch each frame
    if (bgEnable) drawBackground();
    if (spEnable) drawSprites();
    //printf("spEnable is : %d\n",spEnable);
}
void PPU_drawToBuffer(u8 buffer[SCR_MXY * SCR_MXX * 6]){
    for (u16 y = 0;y < 480;y++){
        for (u16 x = 0;x <512;x++){
            *buffer = 255;
            buffer++;
            for (u8 c = 0;c < 3;c++){   
                if (screen[x][y] < 16) *buffer = nes_palette[pallete[0][screen[x][y]]].chn[c];
                else *buffer = nes_palette[pallete[1][screen[x][y] - 16]].chn[c];
                
                buffer++;
            } 
        }
    }
}
u8 PPU_memRead(u16 addr){
    addr &= 0x3fff;
    if (addr < 0x2000 )     // pattern table
        return patternTable.read(addr);
    else if (addr < 0x3f00){      //nametables
        addr &= 0x0fff;
        u16 vNameT = addr / 0x400;
        addr &= 0x3ff;
        return nameTable[getNameTMirtype(vNameT)][addr];
    }
    else if (addr < 0x4000){    //palletes
        addr &= 0x1f;
        u16 palleteType = addr / 0x10;
        addr &= 0x0f;
        return pallete[palleteType][addr];
    }
    return 0;
}
void PPU_memWrite(u16 addr, u8 val){
    addr &= 0x3fff;
    if (addr < 0x2000 )     // pattern table
        return patternTable.write(addr,val);
    else if (addr < 0x3f00){      //nametables
        addr &= 0x0fff;
        u16 vNameT = addr / 0x400;
        addr &= 0x3ff;
        nameTable[getNameTMirtype(vNameT)][addr] = val;
    }
    else if (addr < 0x4000){    // palletes
        addr &= 0x1f;
        u16 palleteType = addr / 0x10;
        addr &= 0x0f;
        
        pallete[palleteType][addr] = val;
    }
    return ;
}
void PPU_setPattenTableMap(memMap mapper){
    patternTable = mapper;
}
u16 PPU_memReadU16(u16 addr){
    u16 f = PPU_memRead(addr);
    u16 s;
    u8 sad = addr & 0xff;
    sad++;
    s = PPU_memRead((addr & 0xff00) + sad);
    return f | (s << 8);
}
