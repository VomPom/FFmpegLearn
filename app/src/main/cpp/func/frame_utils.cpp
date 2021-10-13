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

std::vector<char *> cache_vector;

char *frame_utils::getImage(int index) {
    return cache_vector[index];
}

/**
 * 保存yuv数据
 * @param data
 * @param frameSize
 * */
void save_frame(AVFrame *pFrame, int width, int height) {

    FILE *pFile;
    char *szFilename = "/storage/emulated/0/test_yuv.yuv";
    int y;

    pFile = fopen(szFilename, "wb");

    if (pFile == NULL)
        return;

    int y_size = width * height;
    int u_size = y_size / 4;
    int v_size = y_size / 4;

    //写入文件
    //首先写入Y，再是U，再是V
    //in_frame_picture->data[0]表示Y
    fwrite(pFrame->data[0], 1, y_size, pFile);
    //in_frame_picture->data[1]表示U
    fwrite(pFrame->data[1], 1, u_size, pFile);
    //in_frame_picture->data[2]表示V
    fwrite(pFrame->data[2], 1, v_size, pFile);

    // Close file
    fclose(pFile);

}

/**
 * 解码AVFrame中的yuv420数据并且转换为rgba数据
 *
 * @param frame 需要解码的帧结构
 * @param src_width 需要转换的帧宽度
 * @param src_height 需要转换的帧高度
 * @param src_pix_fmt 需要转换的帧编码方式
 * @param dst_width 转换后目标的宽度
 * @param dst_height 转换后目标的高度
 * @return
 *
 **/
int decode_frame(AVFrame *frame, int src_width, int src_height,
                 AVPixelFormat src_pix_fmt, int dst_width, int dst_height, int index) {
    char *decode_data;
    int decode_size;

    struct SwsContext *pSwsCtx;
    // 转换后的帧结构对象
    AVFrame *dst_frameRGBA = av_frame_alloc();

    uint8_t *outBuff;
    // 初始化目标帧长度
    int dst_frame_size;
    // 计算RGBA下的目标长度
    dst_frame_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, dst_width, dst_height, 1);
    // 分配转换后输出的内存空间
    outBuff = (uint8_t *) av_malloc(dst_frame_size * sizeof(uint8_t));

    av_image_fill_arrays(dst_frameRGBA->data, dst_frameRGBA->linesize, outBuff, AV_PIX_FMT_RGBA, frame->width,
                         frame->height, 1);
    // 获取缩放上下文
    pSwsCtx = sws_getContext(src_width, src_height, src_pix_fmt, dst_width, dst_height, AV_PIX_FMT_RGBA,
                             SWS_BILINEAR, nullptr, nullptr, nullptr);
    // 缩放，结果保存在目标帧结构的dst_frameRGBA->data中
    sws_scale(pSwsCtx, frame->data,
              frame->linesize, 0, src_height,
              dst_frameRGBA->data, dst_frameRGBA->linesize);
    dst_frameRGBA->format = AV_PIX_FMT_RGBA;
    // 存储帧结果
    decode_data = (char *) (malloc(dst_frame_size * sizeof(char)));// 测试保存成文件

    // 将解码后的数据拷贝到decode_data中
    memcpy(decode_data, dst_frameRGBA->data[0], dst_frame_size * sizeof(char));
//    save_frame(frame, src_width, src_height);
    //        save_rgb(dst_frameRGBA->data[0], dst_frame_size);
    // 计算解码后的帧大小
    decode_size = dst_frame_size * sizeof(char);
    LOGE("decode_size:%d", decode_size);
    // 释放相关内容
    av_free(outBuff);
    av_free(dst_frameRGBA);
//    cache_vector[index] = decode_data;
    return 1;
}


static void savePixel(AVFrame *pFrameRGBA, int index) {
    SwsContext *swsContext = swsContext = sws_getContext(pFrameRGBA->width, pFrameRGBA->height, AV_PIX_FMT_YUV420P,
                                                         pFrameRGBA->width, pFrameRGBA->height, AV_PIX_FMT_BGR24,
                                                         1, nullptr, nullptr, nullptr);

    int linesize[8] = {pFrameRGBA->linesize[0] * 3};
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, pFrameRGBA->width, pFrameRGBA->height, 1);
    auto *p_global_bgr_buffer = (uint8_t *) malloc(num_bytes * sizeof(uint8_t));
    uint8_t *bgr_buffer[8] = {p_global_bgr_buffer};

    sws_scale(swsContext, pFrameRGBA->data, pFrameRGBA->linesize, 0, pFrameRGBA->height, bgr_buffer, linesize);
    LOGE("julis bgr_buffer:%p", bgr_buffer);
    //bgr_buffer[0] is the BGR raw data
}


int frame_utils::fetchFrame(int task_index) {
//    cache_vector.resize(750);
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

    int64_t pts;
    int64_t start_index, end_index;
    clock_t start, end;
    char buf[1024];
    start_index = 750 * task_index / 4;
    end_index = 750 * (task_index + 1) / 4;;
    for (int64_t i = start_index; i < end_index; i++) {
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
//                LOGE("cost per frame:%lf pts:%ld", (double(end - start) / CLOCKS_PER_SEC) * 1000, pFrame->pts);
                LOGE("save pic frame->pts:%ld index:%d", pFrame->pts, i);
//                snprintf(buf, sizeof(buf), "%s/Demo-%d.jpg", "/storage/emulated/0/saveBitmaps", i);
//                saveJpg(pFrame, buf);
                decode_frame(pFrame, pFrame->width, pFrame->height, AV_PIX_FMT_YUV420P, pFrame->width,
                             pFrame->height, i);
                break;


            }
        }


    }
    return RET_SUCCESS;
}
