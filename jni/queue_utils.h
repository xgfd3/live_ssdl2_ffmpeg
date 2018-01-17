//
// Created by xWX371834 on 2017/8/24.
//

#ifndef SIMPLEST_FFMPEG_ANDROID_STREAMER_QUEUE_UTILS_H
#define SIMPLEST_FFMPEG_ANDROID_STREAMER_QUEUE_UTILS_H

#include "SDL_mutex.h"
#include "libavcodec/avcodec.h"

typedef struct CustomFrame {
    unsigned char *data;
    int width;
    int height;
    unsigned int length;

    AVPacket *packet;
    AVFrame *frame;
    int index;
    struct CustomFrame *next;
} CustomFrame;

typedef struct CustomFrameQueue {
    CustomFrame *firstFrame, *lastFrame;
    int nb_frame;

    SDL_mutex *mutex;
    SDL_cond *cond;

} CustomFrameQueue;

void frame_queue_malloc(CustomFrameQueue **queue);

void frame_malloc(CustomFrame **frame);

void frame_queue_put(CustomFrameQueue *queue, CustomFrame *frame, int cache_frame_nb);

int frame_queue_get(CustomFrameQueue *queue, CustomFrame **frame, int isblock);

#endif //SIMPLEST_FFMPEG_ANDROID_STREAMER_QUEUE_UTILS_H
