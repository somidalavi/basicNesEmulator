#include "controller.h"
#include <SDL2/SDL.h>
#include "bus.h"
#include "def.h"
#include "controller.h"
static u8 keys;
static u8 pollingStarted;
static u8 pollingIndex;
u8 CONTROLLER_read(u16 addr){
    if (addr == 0x4016){
        if (pollingIndex++ <= 7){
           if ( keys & (1 << (pollingIndex - 1))) return 1;
           else return 0;
        
        }
        else return 1;

        
    }
    return 0;
}

void CONTROLLER_write(u16 addr,u8 val){
    if (addr == 0x4016){
        if (val == 1) pollingStarted = 1,pollingIndex = 0;
        else if (val == 0 && pollingStarted == 1) pollingIndex = 0,pollingStarted = 0;
    }else return ;
}
void CONTROLLER_keyDown(u8 key){
    keys |= key;
}
void CONTROLLER_keyUp(u8 key){
    keys &= (0xff) - key;
}
void CONTROLLER_Init(){
    memMap cntrMap;
    cntrMap.st = 0x4016;
    cntrMap.en = 0x4017;
    cntrMap.read = CONTROLLER_read;
    cntrMap.write = CONTROLLER_write;
    BUS_insertMap(cntrMap);
}
u8 CONTROLLER_getKeyState(){
    return keys;
}