//
// Created by julis.wang on 2021/10/8.
//

#include "frame_utils.h"

static const int RET_ERROR = -1;
static const int RET_SUCCESS = 1;

static int saveJpg(AVFrame *pFrame, char *out_name) {
    int width = pFrame->width;
    int height = pFrame->height;
    AVCodecContext *pCodeCtx = nullptr;


    AVFormatContext *pFormatCtx = avformat_alloc_context();
    // 设置输出文件格式
    pFormatCtx->oformat = av_guess_format("mjpeg", nullptr, nullptr);

    // 创建并初始化输出AVIOContext
    if (avio_open(&pFormatCtx->pb, out_name, AVIO_FLAG_READ_WRITE) < 0) {
        LOGE("Couldn't open output file.");
        return -1;
    }

    // 构建一个新stream
    AVStream *pAVStream = avformat_new_stream(pFormatCtx, 0);
    if (pAVStream == nullptr) {
        return -1;
    }

    AVCodecParameters *parameters = pAVStream->codecpar;
    parameters->codec_id = pFormatCtx->oformat->video_codec;
    parameters->codec_type = AVMEDIA_TYPE_VIDEO;
    parameters->format = AV_PIX_FMT_YUVJ420P;
    parameters->width = pFrame->width;
    parameters->height = pFrame->height;

    AVCodec *pCodec = avcodec_find_encoder(pAVStream->codecpar->codec_id);

    if (!pCodec) {
        LOGE("Could not find encoder\n");
        return -1;
    }

    pCodeCtx = avcodec_alloc_context3(pCodec);
    if (!pCodeCtx) {
        LOGE("Could not allocate video codec context\n");
        exit(1);
    }

    if ((avcodec_parameters_to_context(pCodeCtx, pAVStream->codecpar)) < 0) {
        LOGE("Failed to copy %s codec parameters to decoder context\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return -1;
    }

    pCodeCtx->time_base = (AVRational) {1, 25};

    if (avcodec_open2(pCodeCtx, pCodec, nullptr) < 0) {
        LOGE("Could not open codec.");
        return -1;
    }

    int ret = avformat_write_header(pFormatCtx, nullptr);
    if (ret < 0) {
        LOGE("write_header fail\n");
        return -1;
    }

    int y_size = width * height;

    //Encode
    // 给AVPacket分配足够大的空间
    AVPacket pkt;
    av_new_packet(&pkt, y_size * 3);

    // 编码数据
    ret = avcodec_send_frame(pCodeCtx, pFrame);
    if (ret < 0) {
        LOGE("Could not avcodec_send_frame.");
        return -1;
    }

    // 得到编码后数据
    ret = avcodec_receive_packet(pCodeCtx, &pkt);
    if (ret < 0) {
        LOGE("Could not avcodec_receive_packet");
        return -1;
    }

    ret = av_write_frame(pFormatCtx, &pkt);

    if (ret < 0) {
        LOGE("Could not av_write_frame");
        return -1;
    }

    av_packet_unref(&pkt);

    //Write Trailer
    av_write_trailer(pFormatCtx);


    avcodec_close(pCodeCtx);
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    return 0;
}

int frame_utils::fetchFrame(int64_t pts) {

    AVCodecParameters *pCodeParameter;
    //AVFormatContext 包含码流参数较多的结构体，Format I/O context. 它是FFMPEG解封装（flv，mp4，rmvb，avi）功能的结构体
    AVFormatContext *pFormatCtx;

    //AVCodecContext main external API structure.
    AVCodecContext *pCodecCtx;

    //AVCodec 是存储编解码器信息的结构体
    AVCodec *pCodec;

    //AVPacket 是存储压缩编码数据相关信息的结构体

    //1，初始化上下文
    pFormatCtx = avformat_alloc_context();

    //2，打开文件
    if (avformat_open_input(&pFormatCtx, "/storage/emulated/0/25min.mp4", nullptr, nullptr) != 0) {
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
    AVFrame *pFrameRGB;
    pFrameRGB = av_frame_alloc();

    AVPacket *pPacket;
    pPacket = av_packet_alloc();
    if (pPacket == nullptr) {
        LOGE("fetchFrame AVPacket申请失败");
        return RET_ERROR;
    }
    av_init_packet(pPacket);

    AVFrame *pFrame;
    pFrame = av_frame_alloc();
    if (pFrame == nullptr) {
        LOGE("fetchFrame AVFrame申请失败");
        return RET_ERROR;
    }
    clock_t start, end;
    char buf[1024];
    for (int64_t i = 0; i < 750; i++) {
        start = clock();
        pts = i * 1000 * 25 * 2;

        int ret = RET_ERROR;
        //定位到I帧位置
        ret = av_seek_frame(pFormatCtx, video_index, pts, AVSEEK_FLAG_BACKWARD);
        if (ret < 0) {
            LOGE("Error av_seek_frame:%s", av_err2str(ret));
            return RET_ERROR;
        }

        while (av_read_frame(pFormatCtx, pPacket) >= 0) {
            if (pPacket->stream_index == video_index) {
                ret = avcodec_send_packet(pCodecCtx, pPacket);
                if (ret < 0) {
                    LOGE("Error sending a packet for decoding:%s", av_err2str(ret));
                    break;
                }
                ret = avcodec_receive_frame(pCodecCtx, pFrame);
                if (ret == AVERROR(EAGAIN)) {
                    LOGE("ret == AVERROR(EAGAIN)");
                    break;
                }
                if (ret < 0) {
                    LOGE("Error during decoding:%s", av_err2str(ret));
                    return RET_ERROR;
                }
                end = clock();
                LOGE("cost per frame:%lf pts:%lld", (double(end - start) / CLOCKS_PER_SEC) * 1000, pFrame->pts);

//                if (pFrame->pts >= pts)
                LOGE("isFind = true; pts:%lld", pFrame->pts);
                snprintf(buf, sizeof(buf), "%s/Demo-%d.jpg", "/storage/emulated/0/saveBitmaps2", i);
                LOGE("save map index:%d", i);
                saveJpg(pFrame, buf);

                break;

            }
        }


    }
    return RET_SUCCESS;
}
