//
// Created by juliswang on 2023/5/9.
//

#ifndef FFMPEG_LEARN_MUXER_H
#define FFMPEG_LEARN_MUXER_H

#include "string"

extern "C" {
#include "Common.h"
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
};

class muxer {
public:
    void run( const string& output_dst);
};


#endif //FFMPEG_LEARN_MUXER_H
