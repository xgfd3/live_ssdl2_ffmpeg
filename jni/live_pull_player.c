#include <jni.h>
#include <android/log.h>
#include <libswresample/swresample.h>
#include "log_android.h"

#include "SDL.h"
#include "SDL_main.h"
#include "SDL_events.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "queue_utils.h"

#define FF_EVENT_REFRESH SDL_USEREVENT +1
#define FF_EVENT_BREAK SDL_USEREVENT +2


typedef struct AVState {
    CustomFrameQueue *audioPacketQueue, *videoPacketQueue, *videoDataQueue;
    char *input_url;
    AVFormatContext *avFormatCtx;
    AVCodecContext *videoCodecCtx, *audioCodecCtx;
    struct SwsContext *videoSwsCtx;
    AVFrame *videoFrameContainer, *audioFrameContainer;

    uint8_t *audioBuffer;
    int audioBufferSize;
    struct SwrContext *audioSwrCtx;

    SDL_Window *videoWindow;
    SDL_Renderer *videoRenderer;
    SDL_Texture *videoTexture;
    SDL_Rect *videoRect;
    int isQuit;

} AVState;


int audio_decode_frame(AVState *avState, uint8_t **data, int *size) {
    int got_frame, ret;
    CustomFrame *customFrame;
    frame_queue_get(avState->audioPacketQueue, &customFrame, 1);

    if (customFrame->packet == NULL) {
        goto end;
    }

    if ((ret = avcodec_decode_audio4(avState->audioCodecCtx, avState->audioFrameContainer,
                                     &got_frame, customFrame->packet)) < 0) {
        log_ffmpeg_error("avcodec_decode_audio4", ret);
        return -1;
    }

    if ((ret = swr_convert(avState->audioSwrCtx,
            /*out*/&avState->audioBuffer, avState->audioBufferSize,
            /*in*/avState->audioFrameContainer->data, avState->audioFrameContainer->nb_samples)) <
        0) {
        log_ffmpeg_error("swr_convert", ret);
        return -2;
    }

    *data = avState->audioBuffer;
    *size = avState->audioBufferSize;

    av_free_packet(customFrame->packet);
    end:
    av_free(customFrame);
    return 0;
}

void audio_callback(void *userdata, Uint8 *stream, int len) {
    uint8_t *data;
    int size;
    AVState *avState = userdata;

    LOGI("audio audio_callback start len: %d nb:%d", len, avState->audioPacketQueue->nb_frame);

    if (audio_decode_frame(avState, &data, &size) >= 0) {
        LOGI("audio audio_callback size:%d len:%d nb:%d", size, len,
             avState->audioPacketQueue->nb_frame);
        if (size > len) {
            size = len;
        }
        memcpy(stream, data, size);
    } else {
        SDL_PauseAudio(1);
    }
}

void yuv_frame_create(AVState *avState, AVFrame **frame) {
    AVFrame *_frame = av_frame_alloc();
    int size = avpicture_get_size(AV_PIX_FMT_YUV420P, avState->videoCodecCtx->width,
                                  avState->videoCodecCtx->height);
    uint8_t *buff = av_mallocz(size);
    avpicture_fill((AVPicture *) _frame, buff, AV_PIX_FMT_YUV420P, avState->videoCodecCtx->width,
                   avState->videoCodecCtx->height);
    *frame = _frame;
}

int video_decode_thread(void *arg) {
    AVState *avState = arg;
    CustomFrame *customFrame = NULL;
    int got_frame = 0;
    if (avState->videoCodecCtx == NULL) {
        return -2;
    }

    // 初始化视频解码后数据的队列
    frame_queue_malloc(&avState->videoDataQueue);

    LOGI("video_decode_thread start.....");

    for (;;) {
        if (avState->isQuit) {
            break;
        }

        if (frame_queue_get(avState->videoPacketQueue, &customFrame, 1) < 0) {
            break;
        }

        if (customFrame == NULL) {
            continue;
        }
        avcodec_decode_video2(avState->videoCodecCtx, avState->videoFrameContainer, &got_frame,
                              customFrame->packet);

        if (got_frame) {
            yuv_frame_create(avState, &customFrame->frame);
            LOGI("video_decode_thread yuv_frame_create created");
            sws_scale(avState->videoSwsCtx,
                    /*src*/ avState->videoFrameContainer->data,
                      avState->videoFrameContainer->linesize, 0, avState->videoCodecCtx->height,
                    /*des*/ customFrame->frame->data, customFrame->frame->linesize);

            frame_queue_put(avState->videoDataQueue, customFrame, 0);
        }

        av_free_packet(customFrame->packet);
        customFrame->packet = NULL;
    }

    return 0;
}

void stream_component_open(int stream_index, AVState *avState) {
    int ret;
    if (stream_index < 0) {
        return;
    }
    LOGI("stream_component_open stream_index:%d", stream_index);
    AVStream *avStream = avState->avFormatCtx->streams[stream_index];
    AVCodecContext *avCodecContext = avStream->codec;
    if (avCodecContext == NULL) {
        LOGI("stream_component_open avCodecContext == NULL ");
        return;
    }

    AVCodec *avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    if (avCodec == NULL) {
        LOGI("stream_component_open avCodec == NULL ");
        return;
    }

    if ((ret = avcodec_open2(avCodecContext, avCodec, NULL)) < 0) {
        log_ffmpeg_error("avcodec_open2", ret);
        return;
    }


    if (avCodecContext->codec_type == AVMEDIA_TYPE_VIDEO) {
        avState->videoCodecCtx = avCodecContext;
        // 初始化SDL配置
        /*avState->videoWindow = SDL_CreateWindow("SDL HelloWorld!", 0, 0, avCodecContext->width,
                                                avCodecContext->height,
                                                SDL_WINDOW_SHOWN);
        LOGI("stream_component_open video open success");
        avState->videoRenderer = SDL_CreateRenderer(avState->videoWindow, -1,
                                                    SDL_RENDERER_ACCELERATED |
                                                    SDL_RENDERER_PRESENTVSYNC);
        avState->videoTexture = SDL_CreateTexture(avState->videoRenderer, SDL_PIXELFORMAT_IYUV,
                                                  SDL_TEXTUREACCESS_STREAMING,
                                                  avCodecContext->width,
                                                  avCodecContext->height);*/
        avState->videoRect = av_mallocz(sizeof(SDL_Rect));
        avState->videoRect->x = avState->videoRect->y = 0;
        avState->videoRect->w = avCodecContext->width;
        avState->videoRect->h = avCodecContext->height;

        // 准备存放frame的容器
        avState->videoFrameContainer = av_frame_alloc();

        // 转换压缩的上下文
        avState->videoSwsCtx = sws_getContext(
                /*src*/ avCodecContext->width, avCodecContext->height, avCodecContext->pix_fmt,
                /*des*/ avCodecContext->width, avCodecContext->height, PIX_FMT_YUV420P,
                        SWS_FAST_BILINEAR, NULL, NULL, NULL);
        // 初始化存放队列
        frame_queue_malloc(&avState->videoPacketQueue);
        // 开始从视频队列读取并解码, 然后定时器定时发SDLEvent来把视频显示到界面上
        SDL_CreateThread(video_decode_thread, "video_decode_thread", avState);

        LOGI("stream_component_open video open success, width:%d height:%d", avCodecContext->width,
             avCodecContext->height);

    } else if (avCodecContext->codec_type == AVMEDIA_TYPE_AUDIO) {
        avState->audioCodecCtx = avCodecContext;

        // 初始化SDL配置
        SDL_AudioSpec audioSpec;
        audioSpec.freq = avCodecContext->sample_rate;
        audioSpec.format = AUDIO_S16SYS;
        audioSpec.channels = avCodecContext->channels;
        audioSpec.silence = 0;
        audioSpec.samples = avCodecContext->frame_size;
        audioSpec.userdata = avState;
        audioSpec.callback = audio_callback;

        if ((ret = SDL_OpenAudio(&audioSpec, NULL)) < 0) {
            log_sdl_error("SDL_OpenAudio", ret);
            return;
        }

        // 准备存放frame的容器
        avState->audioFrameContainer = av_frame_alloc();
        avState->audioBufferSize = avCodecContext->sample_rate * 16 / 8 *
                                   avCodecContext->channels; // 一段音频的大小（byte） = 采样率 * 位数 / 8 * 通道数
        avState->audioBuffer = av_mallocz(avState->audioBufferSize);


        // 转换压缩的上下文
        avState->audioSwrCtx = swr_alloc();
        avState->audioSwrCtx = swr_alloc_set_opts(avState->audioSwrCtx,
                /*out*/ avCodecContext->channel_layout, avCodecContext->sample_fmt,
                                                  avCodecContext->sample_rate,
                /*int*/ avCodecContext->channel_layout, avCodecContext->sample_fmt,
                                                  avCodecContext->sample_rate,
                                                  0, NULL);
        swr_init(avState->audioSwrCtx);

        // 初始化存放队列
        frame_queue_malloc(&avState->audioPacketQueue);

        // 开始播放,在audio_callback中取包并解码播放
        SDL_PauseAudio(0);

        LOGI("stream_component_open audio open success");
    }
}

int decode_thread(void *arg) {
    AVState *avState = arg;
    CustomFrame *customFrame = NULL;
    int ret;

    LOGI("decode_thread start......");

    // FFmpeg..............
    av_register_all();
    avformat_network_init();

    avState->avFormatCtx = avformat_alloc_context();
    // 解封装
    if ((ret = avformat_open_input(&avState->avFormatCtx, avState->input_url, NULL, NULL)) < 0) {
        log_ffmpeg_error("avformat_open_input", ret);
        return -1;
    }
    if ((ret = avformat_find_stream_info(avState->avFormatCtx, NULL)) < 0) {
        log_ffmpeg_error("avformat_find_stream_info", ret);
        return -2;
    }
    // 获取视频解码器
    int i = 0, audioIndex = -1, videoIndex = -1;
    for (i = 0; i < avState->avFormatCtx->nb_streams; i++) {
        if (avState->avFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioIndex = i;
        } else if (avState->avFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoIndex = i;
        }
        if (audioIndex != -1 && videoIndex != -1) {
            break;
        }
    }

    LOGI("decode_thread audioIndex:%d videoIndex:%d ", audioIndex, videoIndex);

    stream_component_open(videoIndex, avState);
    stream_component_open(audioIndex, avState);

    while (1) {
        customFrame = av_mallocz(sizeof(CustomFrame));
        customFrame->packet = av_mallocz(sizeof(AVPacket));
        customFrame->packet->data = NULL;
        customFrame->packet->size = 0;
        av_init_packet(customFrame->packet);

        if ((ret = av_read_frame(avState->avFormatCtx, customFrame->packet)) < 0) {
            log_ffmpeg_error("av_read_frame", ret);
            if (ret == -11) {
                // Try again
                continue;
            } else {
                break;
            }
        }
        LOGI("decode_thread get packet stream_index:%d", customFrame->packet->stream_index);
        if (customFrame->packet->stream_index == videoIndex) {
            frame_queue_put(avState->videoPacketQueue, customFrame, 0);
        } else if (customFrame->packet->stream_index == audioIndex) {
            frame_queue_put(avState->audioPacketQueue, customFrame, 100);
        } else {
            av_free_packet(customFrame->packet);
            av_free(customFrame);
        }
    }

    while (!avState->isQuit) {
        SDL_Delay(50);
    }

    return 0;
}

Uint32 schedule_refresh_timer_cb(Uint32 interval, void *param) {
    SDL_Event event;
    event.type = FF_EVENT_REFRESH;
    SDL_PushEvent(&event);
    return 0;
}

void schedule_refresh(AVState *avState, int duration) {
    SDL_AddTimer(duration, schedule_refresh_timer_cb, avState);
}

void video_refresh_timer(AVState *avState) {
    int ret;
    CustomFrame *customFrame;

    if (avState->videoDataQueue) {
        LOGI("video_refresh_timer nb:%d", avState->videoDataQueue->nb_frame);

        frame_queue_get(avState->videoDataQueue, &customFrame, 1);
        schedule_refresh(avState, 30);

        if (avState->videoWindow == NULL) {
            LOGI("SDL_CreateWindow width:%d height:%d ", avState->videoCodecCtx->width, avState->videoCodecCtx->height);
            avState->videoWindow = SDL_CreateWindow("SDL HelloWorld!", 0, 0,
                                                    avState->videoCodecCtx->width,
                                                    avState->videoCodecCtx->height,
                                                    SDL_WINDOW_SHOWN);
            avState->videoRenderer = SDL_CreateRenderer(avState->videoWindow, -1, 0);
            avState->videoTexture = SDL_CreateTexture(avState->videoRenderer, SDL_PIXELFORMAT_IYUV,
                                                      SDL_TEXTUREACCESS_STREAMING,
                                                      avState->videoCodecCtx->width,
                                                      avState->videoCodecCtx->height);
        }

        ret = SDL_UpdateYUVTexture(avState->videoTexture, NULL,
                                   customFrame->frame->data[0], customFrame->frame->linesize[0],
                                   customFrame->frame->data[1], customFrame->frame->linesize[1],
                                   customFrame->frame->data[2], customFrame->frame->linesize[2]);
        log_sdl_error("SDL_UpdateYUVTexture", ret);

        ret = SDL_RenderClear(avState->videoRenderer);

        log_sdl_error("SDL_RenderClear", ret);

        ret = SDL_RenderCopy(avState->videoRenderer, avState->videoTexture, NULL, NULL);

        log_sdl_error("SDL_RenderCopy", ret);

        SDL_RenderPresent(avState->videoRenderer);
    } else {
        schedule_refresh(avState, 1);
    }
}

int pull_start(char *url) {
    AVState *avState;
    SDL_Event event;

    avState = av_mallocz(sizeof(AVState));
    avState->input_url = url;
    // avState->input_url = "/storage/emulated/0/testffmpeg.flv";
    avState->isQuit = 0;
    avState->videoCodecCtx = NULL;

    LOGI("main start........");

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        return -1;
    }

    SDL_CreateThread(decode_thread, "decode_thread", avState);

    schedule_refresh(avState, 30);

    while (1) {
        SDL_WaitEvent(&event);
        switch (event.type) {
            case FF_EVENT_BREAK:
                goto end;
            case FF_EVENT_REFRESH:
                video_refresh_timer(avState);
                break;
        }
    }
    end:
    LOGI("main end........");
    return 0;
}

int main(int argc, char *argv[], int type, JNIEnv *env) {
    char *url = argv[1];

    LOGI("type : %d  url:%s", type, url);
    return pull_start(url);
}
