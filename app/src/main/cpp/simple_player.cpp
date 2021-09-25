//
// Created by julis.wang on 2021/9/22.
//

#include "simple_player.h"
#include "utils.h"

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

static int index = 0;

const char *video_file_path = "/storage/emulated/0/test.mp4";
const char *yuv_file_path = "/storage/emulated/0/test_yuv.yuv";

//最简单的 ffmpeg 解析代码逻辑,实现获取视频的每个 AVFrame
int simple_player::extract_av_frame() {

    int i;

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
    for (i = 0; i < pFormatCtx->nb_streams; i++)
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

int simple_player::yuv_to_jpeg() {
    AVFormatContext *pFormatCtx;
    AVOutputFormat *fmt;
    AVStream *video_st;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;

    AVFrame *av_frame;
    AVPacket av_packet;
    int y_size;
    int got_picture = 0;
    int size;
    int ret;
    int in_w = 176, in_h = 144;                                            //YUV's width and height
    const char *out_file = "/storage/emulated/0/yuv_to_jpeg.jpg";    //Output file

    FILE *in_file = fopen(yuv_file_path, "rb");                     //YUV source

    av_register_all(); //TODO:what's this?

    //Method 1
    pFormatCtx = avformat_alloc_context();
    //Guess format
    fmt = av_guess_format("mjpeg", nullptr, nullptr);
    pFormatCtx->oformat = fmt;
    //Output URL
    if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
        LOGE("Couldn't open output file.");
        return -1;
    }

    video_st = avformat_new_stream(pFormatCtx, 0);
    if (video_st == nullptr) {
        return -1;
    }
    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;

    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;

    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;


    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) {
        LOGE("Codec not found.");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
        LOGE("Could not open codec.");
        return -1;
    }

    av_frame = av_frame_alloc();
    int numBytes = av_image_get_buffer_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, 1);
    auto *m_buf_for_rgb_frame = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(av_frame->data,
                         av_frame->linesize,
                         m_buf_for_rgb_frame,
                         pCodecCtx->pix_fmt,
                         pCodecCtx->width,
                         pCodecCtx->height, 1);

    //Write Header
    int result = avformat_write_header(pFormatCtx, nullptr);
    if (result < 0) {
        LOGE("Could not read input file, %d", result);
        return -1;
    }

    y_size = pCodecCtx->width * pCodecCtx->height;
    av_new_packet(&av_packet, y_size * 3);

    //Read YUV
    if (fread(m_buf_for_rgb_frame, 1, y_size * 3 / 2, in_file) <= 0) {
        LOGE("Could not read input file.");
        return -1;
    }

    av_frame->data[0] = m_buf_for_rgb_frame;                    // Y
    av_frame->data[1] = m_buf_for_rgb_frame + y_size;           // U
    av_frame->data[2] = m_buf_for_rgb_frame + y_size * 5 / 4;   // V

    //Encode
    ret = avcodec_send_frame(pCodecCtx, av_frame);
    if (ret < 0) {
        LOGE("Encode Error.\n");
        return -1;
    }
    got_picture = avcodec_receive_packet(pCodecCtx, &av_packet);
    LOGE("got_picture %d", got_picture);
    if (got_picture == 0) {
        av_packet.stream_index = video_st->index;
        av_write_frame(pFormatCtx, &av_packet);
    }

    av_packet_unref(&av_packet);
    //Write Trailer
    av_write_trailer(pFormatCtx);

    LOGE("Encode Successful.\n");

    avcodec_close(video_st->codec);
    av_free(av_frame);
    av_free(m_buf_for_rgb_frame);
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    fclose(in_file);

    return 0;
}
