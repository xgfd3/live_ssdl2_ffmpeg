//
// Created by xWX371834 on 2017/8/21_
//
#include <stdio.h>
#include <stdlib.h>
#include <jni.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <pthread.h>

#include "SDL_main.h"

#include "queue_utils.h"
#include "audio_collector.h"
#include "ffmpeg_encoder.h"
#include "log_android.h"

CustomFrameQueue *audioFrameQueue;
extern unsigned int recorder_buf_size;

static pthread_mutex_t audioEngineLock = PTHREAD_MUTEX_INITIALIZER;
static SLRecordItf recorder;

static Uint8 *recorderBuffer;
static SLAndroidSimpleBufferQueueItf recorder_buffer_queue;

void resumeRecorder() {
    if (pthread_mutex_trylock(&audioEngineLock)) {
        return;
    }
    SLuint32 state_old;
    (*recorder)->GetRecordState(recorder, &state_old);
    if (state_old == SL_RECORDSTATE_STOPPED) {
        (*recorder_buffer_queue)->Clear(recorder_buffer_queue);
        (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_RECORDING);
    }

    // the data is not valid for playback yet
    recorderBuffer = malloc(recorder_buf_size);
    memset(recorderBuffer, 0, recorder_buf_size);
    LOGI("resumeRecorder recorderBuffer:%d recorder_buf_size : %d", recorderBuffer, recorder_buf_size);
    (*recorder_buffer_queue)->Enqueue(recorder_buffer_queue, recorderBuffer,
                                      recorder_buf_size);
}


void bgRecorderCallback(SLAndroidSimpleBufferQueueItf caller, void *pContext) {
    SLresult result;
    CustomFrame *audioFrame;

    audioFrame = malloc(sizeof(CustomFrame));
    memset(audioFrame, 0, sizeof(CustomFrame));

    audioFrame->data = recorderBuffer;

    audioFrame->length = recorder_buf_size;

    LOGI("bgRecorderCallback recorderBuffer:%d recorder_buf_size : %d", recorderBuffer, recorder_buf_size);
    encodeAudioFrame(audioFrame);
    frame_queue_put(audioFrameQueue, audioFrame, 0);

    LOGI("bgRecorderCallback recorder: size: %d", recorder_buf_size);
    pthread_mutex_unlock(&audioEngineLock);
    //SDL_PauseAudio(0);
    resumeRecorder();
}



void stopRecorder(JNIEnv *env, jclass clazz) {
    (*recorder)->SetRecordState(recorder, SL_RECORDSTATE_STOPPED);
    (*recorder_buffer_queue)->Clear(recorder_buffer_queue);
}


int audio_collector_init(){
    SLObjectItf engineObject, mixObject, recorderObject;
    SLEngineItf engine;
    SLEnvironmentalReverbItf environmental_reverb;
    SLEnvironmentalReverbSettings environmentalReverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
    SLresult result;

    // ===== Init Engine =====
    slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engine);

    SLInterfaceID ids[] = {SL_IID_ENVIRONMENTALREVERB};
    SLboolean ir[] = {SL_BOOLEAN_FALSE};
    (*engine)->CreateOutputMix(engine, &mixObject, 1, ids, ir);
    (*mixObject)->Realize(mixObject, SL_BOOLEAN_FALSE);
    result = (*mixObject)->GetInterface(mixObject, SL_IID_ENVIRONMENTALREVERB,
                                        &environmental_reverb);

    (*environmental_reverb)->SetEnvironmentalReverbProperties(environmental_reverb,
                                                              &environmentalReverbSettings);

    LOGI("Init Engine Success!");

    // ==== Create AudioRecorder ====
    // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
                                      SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,
                                   2,  // 双通道
                                   SL_SAMPLINGRATE_44_1, // 44.1KHz
                                   SL_PCMSAMPLEFORMAT_FIXED_16, // 16位
                                   SL_PCMSAMPLEFORMAT_FIXED_16,
                                   SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT, // 两个通关
                                   SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    (*engine)->CreateAudioRecorder(engine, &recorderObject, &audioSrc, &audioSnk, 1, id,
                                   req);

    (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
    (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorder);

    (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                    &recorder_buffer_queue);
    (*recorder_buffer_queue)->RegisterCallback(recorder_buffer_queue, bgRecorderCallback, NULL);

    LOGI("Create AudioRecorder Success!");

    frame_queue_malloc(&audioFrameQueue);

    resumeRecorder();

    return result;
}