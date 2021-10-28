//
// Created by julis.wang on 2021/10/8.
//

#include "frame_utils.h"

static const int RET_ERROR = -1;
static const int RET_SUCCESS = 1;
using namespace cv;

static int saveJpg(AVFrame *pFrame, char *out_name) {
    int width = pFrame->width;
    int height = pFrame->height;
    AVCodecContext *pCodeCtx;

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

/**
 * 保存yuv数据
 * @param data
 * @param frameSize
 * */
void save_frame(AVFrame *pFrame, int width, int height) {
    FILE *pFile;
    const char *szFilename = "/storage/emulated/0/test_yuv.yuv";
    pFile = fopen(szFilename, "wb");
    if (pFile == nullptr) {
        return;
    }

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
static void decode_frame(AVFrame *src_frame, char *out_name) {
    int src_width = src_frame->width;
    int src_height = src_frame->height;
    AVPixelFormat target_pixel_format = AV_PIX_FMT_RGBA;
    AVPixelFormat src_pixel_format = AV_PIX_FMT_YUV420P;

    SwsContext *pSwsCtx;
    AVFrame *dst_RGBA_frame;

    dst_RGBA_frame = av_frame_alloc();
    int dst_frame_size = av_image_get_buffer_size(target_pixel_format, src_width, src_height, 1);
    auto *outBuff = (uint8_t *) av_malloc(dst_frame_size);
    av_image_fill_arrays(dst_RGBA_frame->data, dst_RGBA_frame->linesize, outBuff, target_pixel_format, src_width,
                         src_height, 1);
    pSwsCtx = sws_getContext(src_width, src_height, src_pixel_format, src_width, src_height, target_pixel_format,
                             SWS_BILINEAR, nullptr, nullptr, nullptr);

    sws_scale(pSwsCtx, src_frame->data, src_frame->linesize, 0, src_frame->height, dst_RGBA_frame->data,
              dst_RGBA_frame->linesize);

    Mat image = Mat::zeros(src_height, src_width, CV_8UC4);
    memcpy(image.data, outBuff, dst_frame_size);

    //   写入图片到本地磁盘
    cv::imwrite(out_name, image);

    // 释放相关内容
    sws_freeContext(pSwsCtx);
    av_free(outBuff);
    av_frame_unref(dst_RGBA_frame);
}

std::vector<int64_t> get_key_frame_list(AVFormatContext *formatContext, AVPacket *avPacket, int video_index) {
    std::vector<int64_t> keyFrames;
    while (av_read_frame(formatContext, avPacket) >= 0) {
        if (video_index == avPacket->stream_index) {
            int ret = av_seek_frame(formatContext, video_index, avPacket->pts + 1, 0);
            if (ret == 0) {
                int64_t value = av_rescale_q(avPacket->pts, formatContext->streams[video_index]->time_base,
                                             AV_TIME_BASE_Q);
                LOGE("avPacket->pts:%ld pkt_pts_time:%ld", avPacket->pts, value);
                keyFrames.push_back(value);
            }
        }
        av_packet_unref(avPacket);
    }

    return keyFrames;

}

int frame_utils::fetch_frame(int thread_size, int task_index,
                             int start_ms, int end_ms, int step_ms,
                             char *video_file_path, char *save_path,
                             bool accurate_seek) {

    if (start_ms > end_ms) {
        LOGE("start time must > end time");
        return RET_ERROR;
    }
    AVCodecParameters *pCodeParameter;
    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;

    //1，初始化上下文
    pFormatCtx = avformat_alloc_context();

    //2，打开文件
    if (avformat_open_input(&pFormatCtx, video_file_path, nullptr, nullptr) != 0) {
        LOGE("Couldn't open input stream.\n");
        return RET_ERROR;
    }

    //3，获取音视频流信息
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        LOGE("Couldn't find stream information.\n");
        return RET_ERROR;
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
        LOGE("Didn't find a video stream.\n");
        return RET_ERROR;
    }
    //4.2 获取解码器参数
    pCodeParameter = pFormatCtx->streams[video_index]->codecpar;

    //4.3 获取解码器
    pCodec = avcodec_find_decoder(pCodeParameter->codec_id);

    //4.4 获取解码器上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_parameters_to_context(pCodecCtx, pCodeParameter) < 0) {
        LOGE("Could not avcodec parameters to context.\n");
        return RET_ERROR;
    }
    if (pCodec == nullptr) {
        LOGE("Codec not found.\n");
        return RET_ERROR;
    } else {
        LOGE("find pCodec:%s", pCodec->name);
    }

    //5，打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
        LOGE("Could not open codec.\n");
        return RET_ERROR;
    }
    AVRational time_base = pFormatCtx->streams[video_index]->time_base;

    AVPacket *pPacket = av_packet_alloc();
    av_init_packet(pPacket);
    AVFrame *pFrame = av_frame_alloc();
    int ret, save_index;


    int64_t pts, task_start_index_ms, task_end_index_ms;
    int64_t per_task_time = (end_ms - start_ms) / thread_size;
    task_start_index_ms = start_ms + per_task_time * task_index;
    task_end_index_ms = task_start_index_ms + per_task_time;

    LOGE("task_start_index_ms:%ld task_end_index_ms:%ld", task_start_index_ms, task_end_index_ms);
    //150004
    for (int index = task_start_index_ms; index < task_end_index_ms; index += step_ms) {
        save_index = index / step_ms;
        LOGE("save_index:%d task_index:%d", save_index, task_index);

        pts = index / av_q2d(time_base) / 1000; //25fps
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
                    av_packet_unref(pPacket);
                    break;
                }
                ret = avcodec_receive_frame(pCodecCtx, pFrame);
                if (ret == AVERROR(EAGAIN)) {
                    LOGE("ret == AVERROR(EAGAIN) index:%d", save_index);
                    index -= step_ms; //retry.
                    av_packet_unref(pPacket);
                    break;
                }
                if (accurate_seek && pPacket->pts < pts) {
                    av_packet_unref(pPacket);
                    continue;
                }
                if (ret < 0) {
                    LOGE("Error during decoding:%s", av_err2str(ret));
                    av_packet_unref(pPacket);
                    return RET_ERROR;
                }
                char out_name[1024];
                LOGE("save :%d", save_index);
                snprintf(out_name, sizeof(out_name), "%s/fetch-frame-%03d.jpeg", save_path, save_index);
                decode_frame(pFrame, out_name);
                av_frame_unref(pFrame);
                av_packet_unref(pPacket);
                break;
            }
        }
    }

    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    return RET_SUCCESS;
}