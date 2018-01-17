//
// Created by xWX371834 on 2017/8/24.
//

#include <SDL.h>
#include "sdl_player.h"
#include "log_android.h"

struct SDL_Window *window = NULL;
struct SDL_Renderer *render = NULL;
struct SDL_Texture *texture = NULL;


int sdl_player_init(int width, int height){
    int ret = 0;

    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER);
    LOGI("SDL_CreateWindow width:%d height:%d ", width, height);
    window = SDL_CreateWindow("SDL Camera!", 0, 0, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_FULLSCREEN);
    render = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING,
                                width,
                                height);

    return ret;
}


int play_video(unsigned char *data, int width){
    SDL_UpdateTexture(texture, NULL, data, width);
    SDL_RenderClear(render);
    SDL_RenderCopy(render, texture, NULL, NULL);
    SDL_RenderPresent(render);
    return 0;
}


