#include "def.h"
#include <stdio.h>
#include <string.h>
#include <err.h>
#include "bus.h"
#include "ppu.h"
#include <unistd.h>

#define PRGROM_SZ 16384
#define CHRROME_SZ 8192
#define MX_PRGROM 4
#define MX_CHRROM 4
int areEqual(char *b1,char *b2,int n){
    for (int i = 0;i < n;i++)
        if ((*b1) != (*b2)) return 0;
    return 1;
}

typedef struct _iNES_HEADER{
    u8 magicNum[4];
    u8 nPrgRom;
    u8 nChrRom;
    u8 romControl1;
    u8 romControl2;
    u8 nRamBank;
    u8 reserved[7];
} iNES_HEADER;

typedef struct {
    u8 nPrgRom;
    u8 nChrRom;
    u8 nRamBank;
    enum mirrorType mirType;
    u8 trainerFlag;
    u8 ramFlag;
    u8 mapper;
    u8 prgRom[MX_PRGROM][PRGROM_SZ];
    u8 chrRom[MX_CHRROM][CHRROME_SZ];
    u8 curPrg;
    u8 curChr;
} ROM_DETAILS;

static ROM_DETAILS gRom;
static iNES_HEADER gHeader;


u8 mp0Read(u16 addr){
    return gRom.prgRom[gRom.curPrg][addr - 0x8000];
}
void mp0Write(u16 addr,u8 val){
    gRom.prgRom[gRom.curPrg][addr - 0x8000] = val;
}
u8 DUMMYREAD(u16 addr){
    return 0;
}
void DUMMYWRITE(u16 addr,u8 val){
    return ;
}

u8 chrmp0Read(u16 addr){
    u8 temp =  gRom.chrRom[gRom.curChr][addr];
    
         //printf("reading from :%d\n",addr);

    return temp;
}
void chrmp0Write(u16 addr,u8 val){
    gRom.chrRom[gRom.curChr][addr] = val;
}
i32 ROM_read(FILE *rf){
    fread(&gHeader,sizeof(iNES_HEADER),1,rf);
    static char mNumbers[4]  = {0x4e,0x45,0x53,0x1a};
    for (int i = 0;i < 4;i++)
        if (gHeader.magicNum[i] != mNumbers[i]){
            err(-1,"bad file\n");
            return -1;
        }
        
    /*for (int i = 9;i < 16;i++)
        if (gHeader.reserved[i - 9] != 0){
            err(-1,"bad file\n");
            return -1;
        }*/

    if (gHeader.romControl1 & 0x01) gRom.mirType = MIR_VERTICAL;
    else gRom.mirType = MIR_HORIZONTAL;

    if (gHeader.romControl1 & 0x08) gRom.mirType = MIR_FSCREEN;

    if (gHeader.romControl1 & 0x02) gRom.ramFlag = 1;

    if (gHeader.romControl1 & 0x04) gRom.trainerFlag = 1;
    

    // turned of because seom files are bad even thought they shouldn't have this
    //if (gHeader.romControl2 << 4)
    //    err(-1,"bad file\n");
    
    gRom.mapper = ((gHeader.romControl1 & 0xf0) >> 4) + gHeader.romControl2;

    gRom.nChrRom = gHeader.nChrRom;
    gRom.nPrgRom = gHeader.nPrgRom;
    gRom.nRamBank = gHeader.nRamBank;
    if (gRom.nRamBank == 0) gRom.nRamBank = 1;

    if (gRom.trainerFlag){
        // Do trainer Stuff
        // Nothing for now
    }
    if (gRom.ramFlag){
        // Set up Ram
        // Nothing for now
    }

    if (gRom.nPrgRom > 1)
        fread(gRom.prgRom,PRGROM_SZ,gRom.nPrgRom,rf);
    else
        fread(gRom.prgRom[1],PRGROM_SZ,1,rf); 

    fread(gRom.chrRom,CHRROME_SZ,gRom.nChrRom,rf);
    memMap prgCart;    //address space  mapper for prgCartridge
    prgCart.st = 0x8000;
    prgCart.en = 0xffff;
    memMap chrCart;     //address space mapper for chr rom
    switch (gRom.mapper)
    {
    case 0 :            //NROM mapper 0
        prgCart.read = mp0Read;
        prgCart.write = mp0Write;
        chrCart.read = chrmp0Read;
        chrCart.write = chrmp0Write;
        break;
    default:
        prgCart.write = DUMMYWRITE;
        prgCart.read = DUMMYREAD;
    }
    BUS_insertMap(prgCart);
    PPU_setPattenTableMap(chrCart);

    return -1;
}