#include <stdio.h>
#include <SDL2/SDL.h>
#include "def.h"
#include "ppu.h"
#include "instdecode.h"
#include <unistd.h>
#include <err.h>
#include "rom.h"
#include "cpu.h"
#include <time.h>
#include "bus.h"
#include "controller.h"

#define CPU_FREQ 21441960
#define CYCLEPERDRAW 357366
// 357366
u8 getKey(SDL_Keycode key){
    switch (key)
    {
    case SDLK_f:    return 1;
    case SDLK_d:    return 2;
    case SDLK_s:    return 4;
    case SDLK_SPACE:return 8;
    case SDLK_UP:   return 16;
    case SDLK_DOWN: return 32;
    case SDLK_LEFT: return 64;
    case SDLK_RIGHT:return 128;
    default:
        return 0;
    }
}
int main(int argc, char *argv[]){
    if (argc != 2)
        err(-1,"bad format\n");

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0 )
    {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            "SDL_Init Error",
            SDL_GetError(),
            NULL);
        return -1;
    }
    SDL_Window *sdl_window;
    SDL_Renderer *sdl_renderer;
    SDL_CreateWindowAndRenderer(SCR_MXX * 3, SCR_MXY * 3, SDL_WINDOW_SHOWN, &sdl_window, &sdl_renderer);
    SDL_SetWindowTitle(sdl_window, "Test emulation");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
    SDL_RenderSetLogicalSize(sdl_renderer, SCR_MXX * 2, SCR_MXY * 2);
    SDL_Texture *sdl_texture = SDL_CreateTexture(
        sdl_renderer,
        SDL_PIXELFORMAT_ARGB32,
        SDL_TEXTUREACCESS_STREAMING,
        SCR_MXX,SCR_MXY
    );
    decodeInstruction(0x6c);
    SDL_Event curEvent;
    u8 shouldQuit = 0;
    static u8 buffer[SCR_MXY * SCR_MXX * 4 * 4];
    FILE *rf = fopen(argv[1],"rb");
    ROM_read(rf);
    fclose(rf);
    BUS_Init();
    CPU_init();
    PPU_Init();
    CONTROLLER_Init();
    i32 lastTime = 0, currentTime ;
    const int frameRate = 60;
    const int ticksPerFrame = 1000 / frameRate;
    while(!shouldQuit){
        while(SDL_PollEvent(&curEvent)){
            switch (curEvent.type)
            {
            case SDL_QUIT:
                shouldQuit = 1;
                break;
            case SDL_KEYDOWN:
                CONTROLLER_keyDown(getKey(curEvent.key.keysym.sym));
                break;
            case SDL_KEYUP:
                CONTROLLER_keyUp(getKey(curEvent.key.keysym.sym));
            }
        }
        
        for (u32 i = 0;i < CYCLEPERDRAW;i++){
            CPU_nextCycle();
        }
        currentTime = SDL_GetTicks();
        i32 delta = currentTime - lastTime;
        if (delta < ticksPerFrame) SDL_Delay(ticksPerFrame - delta);
        static int cnt;
        cnt++;
        lastTime = currentTime;
        
        PPU_draw();
        PPU_drawToBuffer(buffer);

        if (SDL_UpdateTexture(sdl_texture, NULL, buffer, SCR_MXX * 4 * 2) < 0) printf("FUcki\n");
        SDL_RenderClear(sdl_renderer);
        SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
        SDL_RenderPresent(sdl_renderer);
        CPU_NMI();
    }
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyTexture(sdl_texture);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

}
