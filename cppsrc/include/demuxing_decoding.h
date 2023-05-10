//
// Created by julis.wang on 2021/10/5.
//

#ifndef FFMPEGLEARN_DEMUXING_DECODING_H
#define FFMPEGLEARN_DEMUXING_DECODING_H

#include "string"

extern "C"
{
#include "Common.h"
#include <libavutil/timestamp.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class demuxing_decoding {
private:
    string yuv_dest_path;
    string pcm_dst_path;

public:
    int run(const string &mp4_path, const string &output_dir);
};


#endif //FFMPEGLEARN_DEMUXING_DECODING_H
