//
// Created by juliswang on 2023/5/6.
//


#include "video_to_jpeg.h"


int video_to_jpeg::run(const string &mp4_path, string output_dir) {
    const AVCodec *pCodec;
    AVCodecContext *pCodeCtx;
    AVStream *stream;
    AVPacket *pPacket;
    AVFrame *pFrame;

    int frame_count;
    int video_stream_index;
    int ret;

    // 初始化上下文
    AVFormatContext *fmt_ctx = avformat_alloc_context();

    // 打开文件
    ret = avformat_open_input(&fmt_ctx, mp4_path.c_str(), nullptr, nullptr);
    if (ret != 0) {
        LOGE("Could not open source file %s ret:%d\n", mp4_path.c_str(), ret);
        return ret;
    }

    // 获取音视频流信息
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        LOGE("Could not find stream information\n");
        return ret;
    }

    //获取视频流的索引
    video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (ret < 0) {
        LOGE("Could not find %s stream in input file '%s'\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO), mp4_path.c_str());
        return ret;
    }

    //获取视频流
    stream = fmt_ctx->streams[video_stream_index];

    //查找解码器
    pCodec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (pCodec == nullptr) {
        return ret;
    }

    //获取解码器context
    pCodeCtx = avcodec_alloc_context3(nullptr);
    if (!pCodeCtx) {
        LOGE("Could not allocate video pCodec context\n");
        return ret;
    }

    if ((ret = avcodec_parameters_to_context(pCodeCtx, stream->codecpar)) < 0) {
        LOGE("Failed to copy %s pCodec parameters to decoder context\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return ret;
    }

    //打开解码器
    avcodec_open2(pCodeCtx, pCodec, nullptr);

    //初始化frame，解码后数据
    pFrame = av_frame_alloc();
    pPacket = av_packet_alloc();

    frame_count = 0;
    char buf[1024];

    //开始循环解码
    while (true) {
        ret = av_read_frame(fmt_ctx, pPacket);
        // Return the next pFrame of a stream.
        if (ret >= 0) {
            if (pPacket->stream_index == video_stream_index) {
                // Supply raw packet data as input to a decoder.
                avcodec_send_packet(pCodeCtx, pPacket);
                // Return decoded output data from a decoder.
                while (avcodec_receive_frame(pCodeCtx, pFrame) == 0) {
                    snprintf(buf, sizeof(buf), "%s/frame-pts-%lld.jpg", output_dir.c_str(), pFrame->pts);
                    decode_frame(pFrame, buf);
                    frame_count++;
                }
            }
            av_packet_unref(pPacket);
        } else {
            LOGE("av_read_frame error:%s", av_err2str(ret));
            break;
        }
    }
    avcodec_close(pCodeCtx);
    avformat_close_input(&fmt_ctx);
    return 0;
}

int video_to_jpeg::decode_frame(AVFrame *pFrame, char *out_name) {
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
        return 0;
    }

    pCodeCtx = avcodec_alloc_context3(pCodec);
    if (!pCodeCtx) {
        LOGE("Could not allocate video codec context\n");
        return 0;
    }

    if ((avcodec_parameters_to_context(pCodeCtx, pAVStream->codecpar)) < 0) {
        LOGE("Failed to copy %s codec parameters to decoder context\n",
             av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return 0;
    }

    pCodeCtx->time_base = (AVRational) {1, 25};

    if (avcodec_open2(pCodeCtx, pCodec, nullptr) < 0) {
        LOGE("Could not open codec.");
        return 0;
    }

    int ret = avformat_write_header(pFormatCtx, nullptr);
    if (ret < 0) {
        LOGE("write_header fail\n");
        return 0;
    }

    int y_size = width * height;

    // Encode
    // 给AVPacket分配足够大的空间
    AVPacket pkt;
    av_new_packet(&pkt, y_size * 3);

    // 编码数据
    ret = avcodec_send_frame(pCodeCtx, pFrame);
    if (ret < 0) {
        LOGE("Could not avcodec_send_frame.");
        return 0;
    }

    // 得到编码后数据
    ret = avcodec_receive_packet(pCodeCtx, &pkt);
    if (ret < 0) {
        LOGE("Could not avcodec_receive_packet");
        return 0;
    }

    ret = av_write_frame(pFormatCtx, &pkt);

    LOGI("video to jpeg to:%s", out_name);
    if (ret < 0) {
        LOGE("Could not av_write_frame");
        return 0;
    }

    //Write Trailer
    av_write_trailer(pFormatCtx);

    av_packet_unref(&pkt);
    avcodec_close(pCodeCtx);
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    return 0;
}
