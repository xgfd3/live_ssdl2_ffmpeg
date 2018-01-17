//
// Created by xWX371834 on 2017/8/24.
//
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#include "log_android.h"
#include "ffmpeg_encoder.h"

AVCodecContext *video_encode_codec_ctx;
AVCodecContext *audio_encode_codec_ctx;

static AVFrame *video_frame_container, *audio_frame_container;

unsigned int recorder_buf_size;

static int64_t start_time;
static double calculate_duration;
static int audio_frame_index, video_frame_index;

int video_encode_init(int width, int height);
int audio_encode_init();

int ffmpeg_encoder_init(int width, int height){
    int ret = 0;
    av_register_all();
    avformat_network_init();

    start_time = 0;
    audio_frame_index = video_frame_index = 0;
    video_encode_init(width, height);
    audio_encode_init();
    return ret;
}

int audio_encode_init(){
    int ret = 0;
    AVCodec *encode_codec = avcodec_find_encoder(AV_CODEC_ID_ADPCM_SWF);

    audio_encode_codec_ctx = avcodec_alloc_context3(encode_codec);

    audio_encode_codec_ctx->codec = encode_codec;
    audio_encode_codec_ctx->codec_id = encode_codec->id;
    audio_encode_codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    audio_encode_codec_ctx->sample_fmt = AV_SAMPLE_FMT_S16;
    audio_encode_codec_ctx->sample_rate = 44100;
    audio_encode_codec_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    audio_encode_codec_ctx->channels = 2;
    audio_encode_codec_ctx->bit_rate = 64000;

    if ((ret = avcodec_open2(audio_encode_codec_ctx, encode_codec, NULL)) < 0) {
        char error_info[1024];
        av_strerror(ret, error_info, 1024);
        LOGI("avcodec_open2 error: %s", error_info);
    }

    audio_frame_container = av_frame_alloc();
    audio_frame_container->nb_samples = audio_encode_codec_ctx->frame_size;
    audio_frame_container->format = audio_encode_codec_ctx->sample_fmt;

    recorder_buf_size = (unsigned int)av_samples_get_buffer_size(NULL, audio_encode_codec_ctx->channels,
                                              audio_encode_codec_ctx->frame_size,
                                              audio_encode_codec_ctx->sample_fmt, 1);
    LOGI("audio_frame_container size: %d", recorder_buf_size);
    uint8_t *audio_frame_buf = av_malloc(recorder_buf_size);
    avcodec_fill_audio_frame(audio_frame_container, audio_encode_codec_ctx->channels, audio_encode_codec_ctx->sample_fmt,
                             audio_frame_buf, recorder_buf_size, 1);

    LOGI("FFmpeg Audio Encoder Init Success!");

    return ret;
}

int video_encode_init(int width, int height){
    int ret;
    // Video编码相关初始化
    LOGI("encodeFrame start");
    AVCodec *encode_codec = avcodec_find_encoder(AV_CODEC_ID_H264);

    video_encode_codec_ctx = avcodec_alloc_context3(encode_codec);


    LOGI("encodeFrame param set start!");

    //Param that must set
    LOGI("encodeFrame param set codec_id!");
    video_encode_codec_ctx->codec = encode_codec;
    video_encode_codec_ctx->codec_id = encode_codec->id;
    video_encode_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    video_encode_codec_ctx->width = width;
    video_encode_codec_ctx->height = height;
    video_encode_codec_ctx->time_base.num = 1;
    video_encode_codec_ctx->time_base.den = 20;
    //video_encode_codec_ctx->bit_rate = 400000;
    //video_encode_codec_ctx->gop_size = 250;
    //H264
    video_encode_codec_ctx->qmin = 10;
    video_encode_codec_ctx->qmax = 51;

    //Optional Param
    video_encode_codec_ctx->max_b_frames = 0;

    //Set Options
    av_opt_set(video_encode_codec_ctx->priv_data, "preset", "ultrafast", 0);
    av_opt_set(video_encode_codec_ctx->priv_data, "tune", "zerolatency", 0);

    LOGI("encodeFrame Param set success!");

    ret = avcodec_open2(video_encode_codec_ctx, encode_codec, NULL);
    LOGI("encodeFrame avcodec_open2 ret: %d", ret);

    // 准备接收数据的容器
    video_frame_container = av_frame_alloc();

    int picture_size = avpicture_get_size(video_encode_codec_ctx->pix_fmt, video_encode_codec_ctx->width, video_encode_codec_ctx->height);
    uint8_t *picture_buf = (uint8_t *) av_malloc(picture_size);
    //avpicture_fill((AVPicture *)av_frame, picture_buf, PIX_FMT_YUV420P, video_encode_codec_ctx->width, video_encode_codec_ctx->height);
    av_image_fill_arrays(video_frame_container->data, video_frame_container->linesize, picture_buf,
                         video_encode_codec_ctx->pix_fmt, video_encode_codec_ctx->width,
                         video_encode_codec_ctx->height, 1);
    LOGI("av_frame for encoding packet init success!!");

    calculate_duration = 1000 * av_q2d(video_encode_codec_ctx->time_base); // 1000 * (1/20)

    return ret;
}

int encodeVideoFrame(CustomFrame *cameraFrame){
    int ret, got_packet;
    AVPacket *av_packet;
    LOGI(" cameraFrame ecoded start......!!");

    if(cameraFrame->data == NULL || video_frame_container->data == NULL){
        return -2;
    }

    av_packet = av_mallocz(sizeof(AVPacket));
    av_init_packet(av_packet);
    av_packet->data = NULL;
    av_packet->size = 0;
    LOGI(" cameraFrame ecoded av_packet malloced !!");
    video_frame_container->data[0] = cameraFrame->data; // Y
    video_frame_container->data[2] = cameraFrame->data + cameraFrame->width * cameraFrame->height; // U
    video_frame_container->data[1] =
            cameraFrame->data + cameraFrame->width * cameraFrame->height * 5 / 4;  //V
    LOGI(" cameraFrame ecoded data setted!!");
    ret = avcodec_encode_video2(video_encode_codec_ctx, av_packet, video_frame_container, &got_packet);
    LOGI(" cameraFrame ecoded avcodec_encode_video2 called!!");
    if (got_packet) {
        LOGI(" cameraFrame ecoded got_packet!!");
        if( start_time == 0 ){
            start_time = av_gettime();
        }
        av_packet->pts = video_frame_index * calculate_duration;
        av_packet->dts = av_packet->pts;
        av_packet->duration = calculate_duration;
        av_packet->pos = -1;

        int64_t pts_time = av_packet->pts * 1000;
        int64_t now_time = av_gettime() - start_time;
        if( pts_time > now_time ){
            av_free_packet(av_packet);
        }else{
            LOGI("frame encode video nowtime:%lld", now_time / 1000);
            cameraFrame->packet = av_packet;
            cameraFrame->index = video_frame_index;
            video_frame_index ++;
        }
    }else{
        LOGI(" cameraFrame ecoded not got_packet!!");
        av_free_packet(av_packet);
    }
    LOGI(" cameraFrame ecoded end..........!!");

    return ret;
}


int encodeAudioFrame(CustomFrame *recorderFrame){
    int got_packet, ret = 0;
    AVPacket *packet;

    packet = av_mallocz(sizeof(AVPacket));
    packet->data = NULL;
    packet->size = 0;
    av_init_packet(packet);

    audio_frame_container->data[0] = recorderFrame->data;
    LOGI("bgRecorderCallback recorderBuffer:%d recorder_buf_size : %d", audio_frame_container->data[0], recorderFrame->length);
    ret = avcodec_encode_audio2(audio_encode_codec_ctx, packet, audio_frame_container, &got_packet);
    LOGI("avcodec_encode_audio2 packet: ret:%d got_packet:%d size:%d", ret, got_packet,
         packet->size);
    if (got_packet) {
        if (start_time == 0) {
            start_time = av_gettime();
        }
        packet->pts = (av_gettime() - start_time) / 1000; // ms
        LOGI("frame encode audio nowtime:%lld", packet->pts);
        recorderFrame->packet = packet;
        recorderFrame->index = audio_frame_index;
        audio_frame_index ++;
    } else {
        av_free_packet(packet);
    }
    return ret;
}