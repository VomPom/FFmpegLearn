//
// Created by julis.wang on 2022/1/8.
//

#ifndef FFMPEGLEARN_YUV_PLAYER_H
#define FFMPEGLEARN_YUV_PLAYER_H

#include "base_func.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window_jni.h>

class yuv_player : base_func {
public:
    static void load_yuv(JNIEnv *env, jobject thiz, jstring url, jobject surface);

    static GLint initShader(const char *source, int type);

    int run() override;
};


#endif //FFMPEGLEARN_YUV_PLAYER_H
