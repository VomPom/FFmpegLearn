//
// Created by julis.wang on 2021/9/26.
//
#pragma once

#ifndef FFMPEGLEARN_BASE_FUNC_H
#define FFMPEGLEARN_BASE_FUNC_H


#define __STDC_CONSTANT_MACROS

#include "utils.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

static char *video_file_path = "/storage/emulated/0/test.mp4";
static char *yuv_file_path = "/storage/emulated/0/test_yuv.yuv";
static int yuv_w = 176, yuv_h = 144;       //YUV's width and height

class base_func {
public:
    virtual int run() = 0;
};


#endif //FFMPEGLEARN_BASE_FUNC_H
