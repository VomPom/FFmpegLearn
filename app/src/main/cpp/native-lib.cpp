//
// Created by julis.wang on 2021/9/18.
//
#include <jni.h>
#include <string>
#include <unistd.h>
#include "base/log.h"

#include "func/yuv_to_h264.h"
#include "func/yuv_to_jpeg.h"
#include "func/extract_av_frame.h"
#include "func/yuv_to_video.h"
#include "func/demuxing_decoding.h"
#include "func/decode_video.h"
#include "func/hw_decode.h"
#include "func/filtering_video.h"
#include "func/frame_utils.h"
#include "func/video_to_jpeg.h"

const char *j_class = "julis/wang/ffmpeglearn/MainActivity";
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/jni.h>

void test_extract_av_frame(JNIEnv *env, jobject thiz) {
    extract_av_frame().run();
}
void test_yuv_to_h264(JNIEnv *env, jobject thiz) {
    yuv_to_h264().run();
}
void test_yuv_to_jpeg(JNIEnv *env, jobject thiz) {
    yuv_to_jpeg().run();
}
void test_yuv_to_video(JNIEnv *env, jobject thiz) {
    yuv_to_video().run();
}
void test_extract_yuv_pcm(JNIEnv *env, jobject thiz) {
    demuxing_decoding().run();
}
void test_video_decode(JNIEnv *env, jobject thiz) {
    decode_video().run();
}
void test_hardware_decode(JNIEnv *env, jobject thiz) {
    hw_decode().run();
}
void test_filtering_video(JNIEnv *env, jobject thiz) {
    filtering_video().run();
}
void test_frame_seek(JNIEnv *env, jobject thiz, jint index) {
    frame_utils::fetchFrame(index);
}

void test_video_to_jpeg(JNIEnv *env, jobject thiz) {
    video_to_jpeg().run();
}

jint RegisterNatives(JNIEnv *env) {
    jclass clazz = env->FindClass(j_class);
    if (clazz == nullptr) {
        LOGE("caon't find class: %s", j_class);
        return JNI_ERR;
    }
    JNINativeMethod methods_MainActivity[] = {
            {"video_to_jpeg",        "()V",  (void *) test_video_to_jpeg},
            {"frame_seek",           "(I)V", (void *) test_frame_seek},
            {"filtering_video",      "()V",  (void *) test_filtering_video},
            {"hardware_decode",      "()V",  (void *) test_hardware_decode},
            {"video_decode",         "()V",  (void *) test_video_decode},
            {"demuxing",             "()V",  (void *) test_extract_yuv_pcm},
            {"simple_extract_frame", "()V",  (void *) test_extract_av_frame},
            {"yuv_to_jpeg",          "()V",  (void *) test_yuv_to_jpeg},
            {"yuv_to_h264",          "()V",  (void *) test_yuv_to_h264},
            {"yuv_to_video",         "()V",  (void *) test_yuv_to_video},
    };
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