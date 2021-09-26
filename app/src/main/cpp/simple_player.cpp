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
const int yuv_w = 176, yuv_h = 144;       //YUV's width and height

/**
 * 基于：最简单的基于FFMPEG+SDL的音频播放器
 * https://blog.csdn.net/leixiaohua1020/article/details/10528443
 * 实现获取视频的每个 AVFrame
 * @return
 */
int simple_player::extract_av_frame() {
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

/**
 * YUV编码为JPEG
 * https://blog.csdn.net/leixiaohua1020/article/details/25346147
 * @return
 */
int simple_player::yuv_to_jpeg() {
    AVFormatContext *pFormatCtx;
    AVOutputFormat *fmt;
    AVStream *video_st;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;

    AVFrame *av_frame;
    AVPacket av_packet;
    int pic_width_mul_height;
    int got_picture = 0;
    int ret;
    const char *out_file = "/storage/emulated/0/yuv_to_jpeg.jpg";    //Output file

    FILE *in_file = fopen(yuv_file_path, "rb");                 //YUV source

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

    pCodecCtx->width = yuv_w;
    pCodecCtx->height = yuv_h;

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
    auto *m_buf_for_rgb_frame = (uint8_t *) av_malloc(numBytes);
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

    pic_width_mul_height = pCodecCtx->width * pCodecCtx->height;
    av_new_packet(&av_packet, pic_width_mul_height * 3);

    //Read YUV
    if (fread(m_buf_for_rgb_frame, 1, pic_width_mul_height * 3 / 2, in_file) <= 0) {
        LOGE("Could not read input file.");
        return -1;
    }

    av_frame->data[0] = m_buf_for_rgb_frame;                                  // Y
    av_frame->data[1] = m_buf_for_rgb_frame + pic_width_mul_height;           // U
    av_frame->data[2] = m_buf_for_rgb_frame + pic_width_mul_height * 5 / 4;   // V

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


int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
          AV_CODEC_CAP_DELAY))
        return 0;
    while (true) {
        enc_pkt.data = nullptr;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
                                    nullptr, &got_frame);
        av_frame_free(nullptr);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        LOGE("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}


/**
 * YUV编码为H.264
 * https://blog.csdn.net/leixiaohua1020/article/details/25430425
 *
 * 流程图：https://img-blog.csdn.net/20180411181113450
 * @return
 */
int simple_player::yuv_to_h264() {
    AVFormatContext *pFormatCtx;
    AVOutputFormat *fmt;
    AVStream *video_st;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket pkt;
    AVFrame *pFrame;
    int pic_width_mul_height;
    int framecnt = 0;

    FILE *in_file = fopen(yuv_file_path, "rb");   //Input raw YUV data
    int framenum = 150;                                   //Frames to encode

    const char *out_file = "/storage/emulated/0/yuv_to_h264.h264";

    //Method1.
    pFormatCtx = avformat_alloc_context();
    //Guess Format
    fmt = av_guess_format(nullptr, out_file, nullptr);
    pFormatCtx->oformat = fmt;

    //Open output URL
    if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE) < 0) {
        printf("Failed to open output file! \n");
        return -1;
    }

    video_st = avformat_new_stream(pFormatCtx, 0);
    if (video_st == nullptr) {
        return -1;
    }
    //Param that must set
    pCodecCtx = video_st->codec;
    //pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = yuv_w;
    pCodecCtx->height = yuv_h;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size = 250;

    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;

    //H264
    //pCodecCtx->me_range = 16;
    //pCodecCtx->max_qdiff = 4;
    //pCodecCtx->qcompress = 0.6;
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;

    //Optional Param
    pCodecCtx->max_b_frames = 3;

    // Set Option
    AVDictionary *param = 0;
    //H.264
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        av_dict_set(&param, "preset", "slow", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
        //av_dict_set(¶m, "profile", "main", 0);
    }
    //H.265
    if (pCodecCtx->codec_id == AV_CODEC_ID_H265) {
        av_dict_set(&param, "preset", "ultrafast", 0);
        av_dict_set(&param, "tune", "zero-latency", 0);
    }

    //Show some Information
    av_dump_format(pFormatCtx, 0, out_file, 1);

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) {
        LOGE("Can not find encoder! \n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
        LOGE("Failed to open encoder! \n");
        return -1;
    }

    pFrame = av_frame_alloc();
    int numBytes = av_image_get_buffer_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, 1);
    auto *m_buf_for_rgb_frame = (uint8_t *) av_malloc(numBytes);
    av_image_fill_arrays(pFrame->data,
                         pFrame->linesize,
                         m_buf_for_rgb_frame,
                         pCodecCtx->pix_fmt,
                         pCodecCtx->width,
                         pCodecCtx->height, 1);

    //Write File Header
    avformat_write_header(pFormatCtx, nullptr);
    av_new_packet(&pkt, numBytes);
    pic_width_mul_height = pCodecCtx->width * pCodecCtx->height;

    for (int i = 0; i < framenum; i++) {
        //Read raw YUV data
        if (fread(m_buf_for_rgb_frame, 1, pic_width_mul_height * 3 / 2, in_file) <= 0) {
            LOGE("Failed to read raw data! \n");
            return -1;
        } else if (feof(in_file)) {
            break;
        }
        pFrame->data[0] = m_buf_for_rgb_frame;                                    // Y
        pFrame->data[1] = m_buf_for_rgb_frame + pic_width_mul_height;             // U
        pFrame->data[2] = m_buf_for_rgb_frame + pic_width_mul_height * 5 / 4;     // V
        // 为什么 v 需要 *5/4 ?
        //YUV420P中Y分量占w*h，U分量占w*h/4，V分量占w*h/4，故对应的偏移分别是是0，w*h和w*h+w*h/4，所以V分量偏移是w*h*5/4

        //PTS
        //pFrame->pts=i;
        pFrame->pts = i * (video_st->time_base.den) / ((video_st->time_base.num) * 25);
        int got_picture = 0;
        //Encode
        // 编码一帧视频。即将AVFrame（存储YUV像素数据）
        // 编码为AVPacket（存储H.264等格式的码流数据）。
        avcodec_send_frame(pCodecCtx, pFrame);
        got_picture = avcodec_receive_packet(pCodecCtx, &pkt);
        if (got_picture == 0) {
            printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);
            framecnt++;
            pkt.stream_index = video_st->index;
            av_write_frame(pFormatCtx, &pkt);
            av_packet_unref(&pkt);
        }
    }

    //Flush Encoder
    // 为什么在编码完成之后还需要调用flush_encoder呢？
    // 比如说在有IBP帧的情况下，输出的码流帧和输入的像素数据帧的顺序是不一样的，这样就涉及到了帧重排。
    // 这样编码器中就一直包含着几帧数据，最后自然要flush_encoder()
    int ret = flush_encoder(pFormatCtx, 0);
    if (ret < 0) {
        LOGE("Flushing encoder failed\n");
        return -1;
    }

    //Write file trailer
    av_write_trailer(pFormatCtx);

    //Clean
    avcodec_close(video_st->codec);
    av_free(pFrame);
    av_free(m_buf_for_rgb_frame);
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    fclose(in_file);
    return 0;
}
