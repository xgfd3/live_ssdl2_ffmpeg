
#include <jni.h>
#include "log_android.h"
#include "video_collector.h"
#include "queue_utils.h"
#include "ffmpeg_encoder.h"

CustomFrameQueue *videoFrameQueue;

void rotateYUV420P(unsigned char *src, unsigned char *des, int width, int height) {
    int wh = width * height;
    int i, j, k = 0;
    unsigned char *tmp;

    // 旋转Y
    tmp = src;
    for (i = 0; i < width; i++) {
        for (j = height - 1; j >= 0; j--) {
            des[k] = tmp[width * j + i];
            k++;
        }
    }
    // 旋转V
    tmp += wh;
    for (i = 0; i < width / 2; i++) {
        for (j = height / 2 - 1; j >= 0; j--) {
            des[k] = tmp[width / 2 * j + i];
            k++;
        }
    }

    // 旋转U
    tmp += wh / 4;
    for (i = 0; i < width / 2; i++) {
        for (j = height / 2 - 1; j >= 0; j--) {
            des[k] = tmp[width / 2 * j + i];
            k++;
        }
    }
}

void Java_org_libsdl_app_PushStreamActivity_onPreviewFrame(JNIEnv *env, jobject instance,
        jbyteArray data_, jint width, jint height){
    jbyte *data = (*env)->GetByteArrayElements(env, data_, NULL);
    int dataLen = width * height * 1.5, i;

    LOGI("onPreviewFrame width:%d, height:%d", width, height);

    CustomFrame *cameraFrame;

    frame_malloc(&cameraFrame);
    cameraFrame->data = malloc(dataLen);
    cameraFrame->width = height;
    cameraFrame->height = width;

    rotateYUV420P(data, cameraFrame->data, width, height);
    // play_video(data, width);
    // for(i = 0; i < dataLen; i++){
    //     cameraFrame->data[i] = data[i];
    // }

    encodeVideoFrame(cameraFrame);

    frame_queue_put(videoFrameQueue, cameraFrame, 0);

    (*env)->ReleaseByteArrayElements(env, data_, data, 0);
}


int video_collector_init(JNIEnv *env, int width, int height){
    int ret = 0;
    /*jmethodID startPreviewMethod, stopPreviewMethod;

    // Camera camera = Camera.open()
    jclass cameraClass = (*env)->FindClass(env, "android/hardware/Camera");
    jmethodID camOpenMethod = (*env)->GetStaticMethodID(env, cameraClass, "open",
                                                        "()Landroid/hardware/Camera;");
    jobject camearObj = (*env)->CallStaticObjectMethod(env, cameraClass, camOpenMethod);

    // new CameraViewCallBack()
    jclass callBackClass = (*env)->FindClass(env, "org/libsdl/app/CameraViewCallBack");
    jmethodID constructMethod = (*env)->GetMethodID(env, callBackClass, "<init>", "()V");
    jobject callBackObj = (*env)->NewObject(env, callBackClass, constructMethod);

    // camera.setPreviewCallback(new CameraViewCallBack());
    jmethodID setCallBackMethod = (*env)->GetMethodID(env, cameraClass, "setPreviewCallback",
                                                      "(Landroid/hardware/Camera$PreviewCallback;)V");
    (*env)->CallVoidMethod(env, camearObj, setCallBackMethod, callBackObj);

    //Camera.Parameters parameters = mCamera.getParameters();
    //parameters.setPreviewSize(640, 1024);
    //parameters.setPreviewFormat(ImageFormat.YV12);
    //mCamera.setParameters(parameters);
    jmethodID getParamMethod = (*env)->GetMethodID(env, cameraClass, "getParameters",
                                                   "()Landroid/hardware/Camera$Parameters;");
    jobject paramObj = (*env)->CallObjectMethod(env, camearObj, getParamMethod);

    jclass paramClass = (*env)->FindClass(env, "android/hardware/Camera$Parameters");
    jmethodID setPreSizeMethod = (*env)->GetMethodID(env, paramClass, "setPreviewSize", "(II)V");
    (*env)->CallVoidMethod(env, paramObj, setPreSizeMethod, height, width);

    jclass imageFormatClass = (*env)->FindClass(env, "android/graphics/ImageFormat");
    jfieldID yv12Field = (*env)->GetStaticFieldID(env, imageFormatClass, "YV12", "I");
    jint yv12Value = (*env)->GetStaticIntField(env, imageFormatClass, yv12Field);

    jmethodID setPreForMethod = (*env)->GetMethodID(env, paramClass, "setPreviewFormat", "(I)V");
    (*env)->CallVoidMethod(env, paramObj, setPreForMethod, yv12Value);

    //parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
    jfieldID FOCUS_MODE_CONTINUOUS_PICTURE_ID = (*env)->GetStaticFieldID(env, paramClass, "FOCUS_MODE_CONTINUOUS_PICTURE", "Ljava/lang/String;");
    jobject FOCUS_MODE_CONTINUOUS_PICTURE = (*env)->GetStaticObjectField(env, paramClass, FOCUS_MODE_CONTINUOUS_PICTURE_ID);
    jmethodID setFocusModeMethod = (*env)->GetMethodID(env, paramClass, "setFocusMode", "(Ljava/lang/String;)V");
    (*env)->CallVoidMethod(env, paramObj, setFocusModeMethod, FOCUS_MODE_CONTINUOUS_PICTURE);

    jmethodID setParametersMethod = (*env)->GetMethodID(env, cameraClass, "setParameters",
                                                        "(Landroid/hardware/Camera$Parameters;)V");
    (*env)->CallVoidMethod(env, camearObj, setParametersMethod, paramObj);

    startPreviewMethod = (*env)->GetMethodID(env, cameraClass, "startPreview", "()V");
    stopPreviewMethod = (*env)->GetMethodID(env, cameraClass, "stopPreview", "()V");

    LOGI("open camera success!");*/

    frame_queue_malloc(&videoFrameQueue);

    /*(*env)->CallVoidMethod(env, camearObj, startPreviewMethod);*/

    return ret;
}

