//
// Created by juliswang on 2023/5/10.
//

#ifndef FFMPEG_LEARN_ENCODE_VIDEO_H
#define FFMPEG_LEARN_ENCODE_VIDEO_H

#include "string"

extern "C" {
#include "Common.h"
#include <libavcodec/avcodec.h>

#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
};

class encode_video {
public:
    void run(const string& output_file_path, const string& codec_name);

};


#endif //FFMPEG_LEARN_ENCODE_VIDEO_H
