//
// Created by juliswang on 2023/5/8.
//

#ifndef FFMPEG_LEARN_JPEG_TO_VIDEO_H
#define FFMPEG_LEARN_JPEG_TO_VIDEO_H

#include "string"

extern "C"
{
#include "Common.h"
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
}

class jpeg_to_video {
private:
    AVFrame *de_frame;
    AVFrame *en_frame;
    // 用于视频像素转换
    SwsContext *sws_ctx;
    // 用于读取视频
    AVFormatContext *in_fmt;
    // 用于解码
    AVCodecContext *de_ctx;
    // 用于编码
    AVCodecContext *en_ctx;
    // 用于封装jpg
    AVFormatContext *ou_fmt;
    int video_ou_index;

    void releaseSources();

    void doDecode(AVPacket *in_pkt);

    void doEncode(AVFrame *en_frame);

public:
    void jpeg_2_video(const string& out_mp4_path, const string& input_jpeg_dir);
};


#endif //FFMPEG_LEARN_JPEG_TO_VIDEO_H
