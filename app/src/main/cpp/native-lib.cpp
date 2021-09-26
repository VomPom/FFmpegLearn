//
// Created by julis.wang on 2021/9/18.
//
#include <jni.h>
#include <string>
#include <unistd.h>
#include "utils.h"

#include "yuv_to_h264.h"
#include "yuv_to_jpeg.h"
#include "extract_av_frame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/jni.h>


void test_extract_av_frame() {
    extract_av_frame().run();
}
void test_yuv_to_h264() {
    yuv_to_h264().run();
}
void test_yuv_to_jpeg() {
    yuv_to_jpeg().run();
}

jint RegisterNatives(JNIEnv *env) {
    jclass clazz = env->FindClass("julis/wang/ffmpeglearn/MainActivity");
    if (clazz == NULL) {
        LOGE("con't find class: julis/wang/ffmpeglearn/MainActivity");
        return JNI_ERR;
    }
    JNINativeMethod methods_MainActivity[] = {
            {"simple_extract_frame", "()V", (void *) test_extract_av_frame},
            {"yuv_to_jpeg",          "()V", (void *) test_yuv_to_jpeg},
            {"yuv_to_h264",          "()V", (void *) test_yuv_to_h264}
    };
    // int len = sizeof(methods_MainActivity) / sizeof(methods_MainActivity[0]);
    return env->RegisterNatives(clazz, methods_MainActivity,
                                sizeof(methods_MainActivity) / sizeof(methods_MainActivity[0]));
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jint result = RegisterNatives(env);
    LOGD("RegisterNatives result: %d", result);
    return JNI_VERSION_1_6;
}
}