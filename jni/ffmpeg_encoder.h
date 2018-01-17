//
// Created by xWX371834 on 2017/8/24.
//

#ifndef SIMPLEST_FFMPEG_ANDROID_STREAMER_FFMPEG_ENCODER_H
#define SIMPLEST_FFMPEG_ANDROID_STREAMER_FFMPEG_ENCODER_H

#include "queue_utils.h"

AVCodecContext *video_encode_codec_ctx;
AVCodecContext *audio_encode_codec_ctx;
unsigned int recorder_buf_size;

int ffmpeg_encoder_init(int width, int height);

int encodeVideoFrame(CustomFrame *cameraFrame);

int encodeAudioFrame(CustomFrame *recorderFrame);


#endif //SIMPLEST_FFMPEG_ANDROID_STREAMER_FFMPEG_ENCODER_H
