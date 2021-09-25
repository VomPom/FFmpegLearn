//
// Created by julis.wang on 2021/9/18.
//
#include <jni.h>
#include <string>
#include <unistd.h>
#include "utils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/jni.h>
#include "simple_player.h"

jstring test_getinfo(JNIEnv *env) {
    char info[40000] = {0};
    AVCodec *c_temp = av_codec_next(NULL);
    while (c_temp != NULL) {
        if (c_temp->decode != NULL) {
            sprintf(info, "%sdecode:", info);
        } else {
            sprintf(info, "%sencode:", info);
        }
        switch (c_temp->type) {
            case AVMEDIA_TYPE_VIDEO:
                sprintf(info, "%s(video):", info);
                break;
            case AVMEDIA_TYPE_AUDIO:
                sprintf(info, "%s(audio):", info);
                break;
            default:
                sprintf(info, "%s(other):", info);
                break;
        }
        sprintf(info, "%s[%s]\n", info, c_temp->name);
        c_temp = c_temp->next;
    }
    return env->NewStringUTF(info);

}

JNIEXPORT jstring JNICALL
Java_julis_wang_ffmpeglearn_MainActivity_simple_1extract_1frame__(JNIEnv *env, jobject  /* this */) {
    simple_player::extract_av_frame();
    return env->NewStringUTF("test return");
}


}extern "C"
JNIEXPORT void JNICALL
Java_julis_wang_ffmpeglearn_MainActivity_yuv_1to_1jpeg(JNIEnv *env, jobject thiz) {
    simple_player::yuv_to_jpeg();
}