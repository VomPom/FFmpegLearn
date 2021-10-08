//
// Created by julis.wang on 2021/10/8.
//

#ifndef FFMPEGLEARN_FRAME_UTILS_H
#define FFMPEGLEARN_FRAME_UTILS_H

#include "base_func.h"

class frame_utils {
private:
    static const int MAX_CACHED_NUM = 21;
    AVFrame *cachedFrames[MAX_CACHED_NUM];


public:
    int64_t curPts = MAX_CACHED_NUM / 2;//默认初始值
    AVFormatContext *c;
    AVCodecContext *codecCxt;
public:
    /**
     * 获取指定时间(pts)的视频帧
     * @param pCodecCtx  解码上下文
     * @param pts       目标时间戳
     * @param video_index 流索引
     * @param pFrame    返回帧数据
     * @param pFormatCtx         格式上下文
     * @return
     */
    static int fetchFrame(int64_t pts);
};


#endif //FFMPEGLEARN_FRAME_UTILS_H
