#include "queue_utils.h"//
// Created by xWX371834 on 2017/8/24.
//

#ifndef SIMPLEST_FFMPEG_ANDROID_STREAMER_VIDEO_COLLECTOR_H
#define SIMPLEST_FFMPEG_ANDROID_STREAMER_VIDEO_COLLECTOR_H

CustomFrameQueue *videoFrameQueue;

int video_collector_init(JNIEnv *env, int width, int height);

#endif //SIMPLEST_FFMPEG_ANDROID_STREAMER_VIDEO_COLLECTOR_H
