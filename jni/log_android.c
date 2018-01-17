//
// Created by xWX371834 on 2017/8/30.
//
#include "libavutil/error.h"
#include "SDL_error.h"
#include "log_android.h"

void log_ffmpeg_error(char *prefix, int code) {
    char info[1024];
    av_strerror(code, info, 1024);
    LOGI("ffmpeg prefix:%s code:%d error:%s ", prefix, code, info);
}

void log_sdl_error(char *prefix, int code) {
    if (code < 0) {
        LOGI("sdl prefix:%s code:%d error:%s ", prefix, code, SDL_GetError());
    }
}
