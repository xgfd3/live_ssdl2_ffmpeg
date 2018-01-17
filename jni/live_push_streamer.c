//
// Created by xWX371834 on 2017/8/24.
//
#include <stdio.h>
#include <stdlib.h>

#include <jni.h>
#include <SDL_thread.h>
#include "ffmpeg_encoder.h"
#include "ffmpeg_streamer.h"
#include "sdl_player.h"
#include "audio_collector.h"
#include "video_collector.h"

//extern CustomFrameQueue *audioFrameQueue;
extern CustomFrameQueue *videoFrameQueue;
extern CustomFrameQueue *audioFrameQueue;

int streaming_thread(void *arg){
    CustomFrame *audioFrame, *videoFrame;
    int64_t video_pts = 0, audio_pts = 0;
    while (1){
        videoFrame = NULL;
        audioFrame = NULL;

        if (frame_queue_get(videoFrameQueue, &videoFrame, 1) < 0) {
            break;
        }
        // play_video(videoFrame->data, videoFrame->width);
        if( !videoFrame->packet ){
            av_free(videoFrame->data);
            av_free(videoFrame);
            continue;
        }
        video_pts = videoFrame->packet->pts;
        streamVideoFrame(videoFrame);
        av_free(videoFrame->data);
        av_free(videoFrame);

        while (1){
            if (frame_queue_get(audioFrameQueue, &audioFrame, 1) < 0) {
                break;
            }
            audio_pts = audioFrame->packet->pts;
            streamAudioFrame(audioFrame);
            av_free(audioFrame->data);
            av_free(audioFrame);
            if( audio_pts >= video_pts){
                break;
            }
        }
    }
    return 0;
}

void Java_org_libsdl_app_PushStreamActivity_pushStart(JNIEnv *env, jobject instance, jint width, jint height, jstring url){
    //char *out_url = "/storage/emulated/0/testffmpeg.flv";
    const char *out_url = (*env)->GetStringUTFChars(env,url, 0);

    ffmpeg_encoder_init(width, height);
    ffmpeg_streamer_init(out_url);
    //sdl_player_init(width, height);
    video_collector_init(env, width, height);
    audio_collector_init();

    SDL_CreateThread(streaming_thread, "streaming", NULL);
}
