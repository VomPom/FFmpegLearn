//
// Created by julis.wang on 2021/10/6.
//

#ifndef FFMPEGLEARN_HW_DECODE_H
#define FFMPEGLEARN_HW_DECODE_H

#include "string"

extern "C"
{
#include "Common.h"
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class hw_decode {
public:
    static int run(const string &mp4_path, const string &output_dir, const string &hw_name);
};


#endif //FFMPEGLEARN_HW_DECODE_H
