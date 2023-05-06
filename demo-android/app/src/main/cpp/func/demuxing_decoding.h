//
// Created by julis.wang on 2021/10/5.
//

#ifndef FFMPEGLEARN_DEMUXING_DECODING_H
#define FFMPEGLEARN_DEMUXING_DECODING_H

#include "base_func.h"
#include <libavutil/timestamp.h>

class demuxing_decoding : base_func {
public:
    int run() override;
};


#endif //FFMPEGLEARN_DEMUXING_DECODING_H
