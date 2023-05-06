//
// Created by julis.wang on 2021/9/26.
//

#ifndef FFMPEGLEARN_YUV_TO_H264_H
#define FFMPEGLEARN_YUV_TO_H264_H

#include "string"

extern "C"
{
#include "Common.h"
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

}

class YuvConverter {
public:
    int yuv2mp4(string yuv_file_path, string des_file_path, int yuv_w, int yuv_h);
};

#endif //FFMPEGLEARN_YUV_TO_H264_H
