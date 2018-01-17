//
// Created by xWX371834 on 2017/8/24.
//

#ifndef SIMPLEST_FFMPEG_ANDROID_STREAMER_LOG_ANDROID_H
#define SIMPLEST_FFMPEG_ANDROID_STREAMER_LOG_ANDROID_H

#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , "mylive", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , "mylive", __VA_ARGS__)

void log_ffmpeg_error(char *prefix, int code);

void log_sdl_error(char *prefix, int code);

#endif //SIMPLEST_FFMPEG_ANDROID_STREAMER_LOG_ANDROID_H
