//
// Created by xWX371834 on 2017/8/24.
//

#ifndef SIMPLEST_FFMPEG_ANDROID_STREAMER_FFMPEG_STREAMER_H
#define SIMPLEST_FFMPEG_ANDROID_STREAMER_FFMPEG_STREAMER_H


#include "queue_utils.h"

int ffmpeg_streamer_init(char *out_url);

int streamAudioFrame(CustomFrame *audioFrame);

int streamVideoFrame(CustomFrame *videoFrame);

int flushAudio();

int flushVideo();

#endif //SIMPLEST_FFMPEG_ANDROID_STREAMER_FFMPEG_STREAMER_H
