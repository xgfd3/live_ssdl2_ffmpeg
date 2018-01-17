
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include "log_android.h"
#include "ffmpeg_streamer.h"
#include "ffmpeg_encoder.h"

extern AVCodecContext *video_encode_codec_ctx;
extern AVCodecContext *audio_encode_codec_ctx;

static AVFormatContext *out_format_ctx;
static AVStream *video_out_stream, *audio_out_stream;

int video_stream_init();

int audio_stream_init();

int ffmpeg_streamer_init(char *out_url) {
    int ret = 0;

    // 输出初始化
    ret = avformat_alloc_output_context2(&out_format_ctx, NULL, "flv", out_url);
    if( ret < 0){
        log_ffmpeg_error("avformat_alloc_output_context2", ret);
        return ret;
    }

    video_stream_init();

    audio_stream_init();


    if (!(out_format_ctx->flags & AVFMT_NOFILE)) {
        ret = avio_open(&out_format_ctx->pb, out_url, AVIO_FLAG_WRITE);
        if(ret < 0){
            log_ffmpeg_error("avio_open", ret);
            return ret;
        }
    }

    ret = avformat_write_header(out_format_ctx, NULL);
    if(ret < 0){
        log_ffmpeg_error("avformat_write_header", ret);
    }
    return ret;
}

int audio_stream_init(){
    int ret;
    audio_out_stream = avformat_new_stream(out_format_ctx, audio_encode_codec_ctx->codec);
    if( audio_out_stream == NULL ){
        ret = -2;
        LOGI("audio avformat_new_stream error ret: %d", ret);
        return ret;
    }
    ret = avcodec_copy_context(audio_out_stream->codec, audio_encode_codec_ctx);
    if( ret < 0 ){
        log_ffmpeg_error("avcodec_copy_context", ret);
        return ret;
    }

    LOGI("audio output_format_context init ret: %d", ret);
    return ret;
}

int video_stream_init(){
    int ret;
    video_out_stream = avformat_new_stream(out_format_ctx, video_encode_codec_ctx->codec);
    if( video_out_stream == NULL ){
        ret = -2;
        LOGI("video avformat_new_stream error ret: %d", ret);
        return ret;
    }

    video_out_stream->time_base.num = 1;
    video_out_stream->time_base.den = 20;
    //Copy the settings of AVCodecContext
    ret = avcodec_copy_context(video_out_stream->codec, video_encode_codec_ctx);
    if(ret < 0){
        log_ffmpeg_error("avcodec_copy_context", ret);
        return ret;
    }

    video_out_stream->codec->codec_tag = 0;
    if (out_format_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        video_out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    LOGI("video output_format_context init ret: %d", ret);
    return ret;
}


int streamAudioFrame(CustomFrame *audioFrame) {
    int ret = 0;
    AVPacket *audio_packet;
    if (audioFrame == NULL || audioFrame->packet == NULL) {
        ret = -1;
        goto end;
    }

    audio_packet = audioFrame->packet;
    audio_packet->stream_index = audio_out_stream->index;

    LOGI("av_write_frame audio packet: index:%d pts:%lld, size:%d", audioFrame->index,
         audio_packet->pts, audio_packet->size);
    if ((ret = av_write_frame(out_format_ctx, audio_packet)) < 0) {
        char error_info[1024];
        av_strerror(ret, error_info, 1024);
        LOGI("av_write_frame audio error: %s", error_info);
    }
    av_free_packet(audio_packet);
    audioFrame->packet = NULL;

    end:
    return ret;
}

int streamVideoFrame(CustomFrame *cameraFrame) {
    int ret = 0;
    AVPacket *av_packet;
    if (cameraFrame == NULL || cameraFrame->packet == NULL) {
        goto end;
    }

    av_packet = cameraFrame->packet;
    av_packet->stream_index = video_out_stream->index;


    LOGI("av_write_frame video_frame_index:%d calc_duration:%d pts: %lld, dts: %lld, duration:%d, size:%d",
         cameraFrame->index, av_packet->duration, av_packet->pts, av_packet->dts,
         av_packet->duration, av_packet->size);

    ret = av_write_frame(out_format_ctx, av_packet);

    LOGI("encodeFrame av_interleaved_write_frame ret: %d", ret);
    if (ret < 0) {
        char err[1024];
        av_strerror(ret, err, 1024);
        LOGI("encodeFrame av_interleaved_write_frame error: %s", err);
    }
    av_free_packet(av_packet);
    cameraFrame->packet = NULL;
    end:
    return ret;
}

int flushAudio() {
    int ret = 0;
    int got_pakcet;
    AVPacket enc_pkt;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_audio2(audio_out_stream->codec, &enc_pkt, NULL,
                                    &got_pakcet);
        if (ret < 0) {
            break;
        }
        if (!got_pakcet) {
            ret = 0;
            break;
        }
        LOGI("Flush Audio Encode Success!");
        ret = av_write_frame(out_format_ctx, &enc_pkt);
        if (ret < 0) {
            break;
        }

    }
    return ret;
}

int flushVideo() {
    int ret = 0;
    int got_frame;
    AVPacket enc_pkt;
    if (!(video_encode_codec_ctx->codec->capabilities &
          CODEC_CAP_DELAY))
        return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(video_encode_codec_ctx, &enc_pkt,
                                    NULL, &got_frame);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        LOGI("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);

        //Write PTS
        AVRational time_base = video_out_stream->time_base;//{ 1, 1000 };
        AVRational r_framerate1 = {60, 2};
        AVRational time_base_q = {1, AV_TIME_BASE};
        //Duration between 2 frames (us)
        int64_t calc_duration = (double) (AV_TIME_BASE) * (1 / av_q2d(r_framerate1));    //内部时间戳
        //Parameters
        //enc_pkt.pts = av_rescale_q(video_frame_index * calc_duration, time_base_q, time_base);
        enc_pkt.dts = enc_pkt.pts;
        enc_pkt.duration = av_rescale_q(calc_duration, time_base_q, time_base);

        //转换PTS/DTS（Convert PTS/DTS）
        enc_pkt.pos = -1;
        // video_frame_index++;
        //out_format_ctx->duration = enc_pkt.duration * video_frame_index;

        /* mux encoded frame */
        ret = av_write_frame(out_format_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    //Write file trailer
    av_write_trailer(out_format_ctx);
    return ret;
}