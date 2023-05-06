//
// Created by juliswang on 2023/5/6.
//

#ifndef FFMPEG_LEARN_VIDEO_TO_JPEG_H
#define FFMPEG_LEARN_VIDEO_TO_JPEG_H

#include "string"

extern "C"
{
#include "Common.h"
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

}

class video_to_jpeg {
public:

    int saveJpg(AVFrame *pFrame, char *out_name);
    int run(string mp4_path, string output_dir);
};


#endif //FFMPEG_LEARN_VIDEO_TO_JPEG_H
