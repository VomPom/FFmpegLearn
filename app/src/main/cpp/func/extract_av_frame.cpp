//
// Created by julis.wang on 2021/9/26.
//

#include "extract_av_frame.h"

static int index = 0;

/**
 * 基于：最简单的基于FFMPEG+SDL的音频播放器
 * https://blog.csdn.net/leixiaohua1020/article/details/10528443
 * 实现获取视频的每个 AVFrame
 * @return
 */
int extract_av_frame::run() {
    AVCodecParameters *pCodeParameter;
    //AVFormatContext 包含码流参数较多的结构体，Format I/O context. 它是FFMPEG解封装（flv，mp4，rmvb，avi）功能的结构体
    AVFormatContext *pFormatCtx;

    //AVCodecContext main external API structure.
    AVCodecContext *pCodecCtx;

    //AVCodec 是存储编解码器信息的结构体
    AVCodec *pCodec;

    //AVFrame 结构体一般用于存储原始数据（即非压缩数据，例如对视频来说是YUV，RGB，对音频来说是PCM）
    AVFrame *pFrame;

    //AVPacket 是存储压缩编码数据相关信息的结构体
    AVPacket *packet;

    //1，初始化上下文
    pFormatCtx = avformat_alloc_context();

    //2，打开文件
    if (avformat_open_input(&pFormatCtx, video_file_path, nullptr, nullptr) != 0) {
        LOGE("Couldn't open input stream.\n");
        return -1;
    }

    //3，获取音视频流信息
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        LOGE("Couldn't find stream information.\n");
        return -1;
    }

    //4，查找编解码器
    //4.1 获取视频流的索引
    int video_index = -1;//存放视频流的索引
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    if (video_index == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }
    //4.2 获取解码器参数
    pCodeParameter = pFormatCtx->streams[video_index]->codecpar;

    //4.3 获取解码器
    pCodec = avcodec_find_decoder(pCodeParameter->codec_id);

    //4.4 获取解码器上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_parameters_to_context(pCodecCtx, pCodeParameter) < 0) {
        LOGE("Could not avcodec parameters to context.\n");
        return -1;
    }
    if (pCodec == nullptr) {
        LOGE("Codec not found.\n");
        return -1;
    } else {
        LOGE("find pCodec:%s", pCodec->name);
    }

    //5，打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
        LOGE("Could not open codec.\n");
        return -1;
    }

    // 初始化待解码和解码数据结构
    // 1）初始化AVPacket，存放解码前的数据
    packet = av_packet_alloc();
    // 2）初始化AVFrame，存放解码后的数据
    pFrame = av_frame_alloc();

    while (true) {
        if (av_read_frame(pFormatCtx, packet) >= 0) {
            if (packet->stream_index == video_index) {
                int ret = avcodec_send_packet(pCodecCtx, packet);
                if (ret < 0) {
                    LOGE("Decode Error code:%s.\n", av_err2str(AVERROR(ret))); //TODO 前面两帧会失败
                    continue;
                }
                int result = avcodec_receive_frame(pCodecCtx, pFrame);
                if (result == 0) { //成功拿到解码数据，可进行自己的操作
                    if (pFrame->key_frame) {
                        LOGE("--julis got a key frame");
                    }
                    index++;
                    av_packet_unref(packet);
                } else {
                    LOGE("Receive frame error result: %s", av_err2str(AVERROR(result)));
                }
            }
        } else {
            LOGE("//Exit");
            //Exit
            break;
        }
    }

    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    return 0;
}