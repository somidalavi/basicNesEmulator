#include "def.h"
#include <stdio.h>

static memMap maps[20];
static u8 nMaps;
static u8 ram[0x10000];
int BUS_insertMap(memMap map){
    maps[nMaps++] = map;
}
u8 BUS_read(u16 address){
    //printf("Reading : %X = ",address);
    for (u8 i = 0;i < nMaps;i++){
        if (address <= maps[i].en && address >= maps[i].st){
            u8 ret = maps[i].read(address);
      //      printf("%X\n",ret);
            return ret;
        }
    }
    return 0;
}
u16 BUS_readU16(u16 address){
    /*u16 f = BUS_read(address);
    u16 s = BUS_read(address + 1);
    return f | (s << 8);*/
    u16 f = BUS_read(address);
    u16 s;
    if ((address & 0xff) == 0xff)
        s = BUS_read(address - 0xff);
    else 
        s = BUS_read(address + 1);
    return f | (s << 8);
}
u16 BUS_readU16WrapAround(u16 address){
    u16 f = BUS_read(address);
    u16 s;
    u8 sad = address & 0xff;
    sad++;
    s = BUS_read((address & 0xff00) + sad);
    return f | (s << 8);
}
i32 BUS_writeU8(u16 address,u8 val){
    for (u8 i = 0;i < nMaps;i++){
        if (address <= maps[i].en && address >= maps[i].st){
            maps[i].write(address,val);
            return 1;
        }
    }
    return 1;
}


i32 BUS_writeU16(u16 address,u16 val){
    ram[address] = (short)(val & 0xFFFF);
    ram[address] = (short) ((val >> 16) & 0xFFFF);
}


static u8 nesRam[0x800];
u8 RAM_read(u16 addr){
    addr &= 0x7ff;
    return nesRam[addr];
}
void RAM_write(u16 addr,u8 val){
    addr &= 0x7ff;
    nesRam[addr] = val;
}
i32 BUS_DUMP(){
    for (int i = 0;i < 0xff;i++)
        fprintf(stderr,"%X : %X\n",i,nesRam[i]);

    printf("\n");
}
void BUS_Init(){
    memMap normalRam;
    normalRam.st = 0x0000;
    normalRam.en = 0x2000 - 1;
    normalRam.read = RAM_read;
    normalRam.write = RAM_write;
    BUS_insertMap(normalRam);
}