//
// Created by juliswang on 2023/5/8.
//

#include "include/jpeg_to_video.h"


void jpeg_to_video::jpeg_2_video(const string &out_mp4_path, const string &input_jpeg_dir) {
    int video_index = -1;

    // 创建jpg的解封装上下文
    if (avformat_open_input(&in_fmt, input_jpeg_dir.c_str(), nullptr, nullptr) < 0) {
        LOGD("avformat_open_input fail");
        return;
    }
    if (avformat_find_stream_info(in_fmt, nullptr) < 0) {
        LOGD("avformat_find_stream_info()");
        releaseSources();
        return;
    }

    // 创建解码器及初始化解码器上下文用于对jpg进行解码
    for (int i = 0; i < in_fmt->nb_streams; i++) {
        AVStream *stream = in_fmt->streams[i];
        /** 对于jpg图片来说，它里面就是一路视频流，所以媒体类型就是AVMEDIA_TYPE_VIDEO
         */
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
            if (!codec) {
                LOGD("not find jpg codec");
                releaseSources();
                return;
            }
            de_ctx = avcodec_alloc_context3(codec);
            if (!de_ctx) {
                LOGD("jpg codec_ctx fail");
                releaseSources();
                return;
            }

            // 设置解码参数;文件解封装的AVStream中就包括了解码参数，这里直接流中拷贝即可
            if (avcodec_parameters_to_context(de_ctx, stream->codecpar) < 0) {
                LOGD("set jpg de_ctx parameters fail");
                releaseSources();
                return;
            }

            // 初始化解码器及解码器上下文
            if (avcodec_open2(de_ctx, codec, nullptr) < 0) {
                LOGD("avcodec_open2() fail");
                releaseSources();
                return;
            }
            video_index = i;
            break;
        }
    }

    // 创建mp4文件封装器
    if (avformat_alloc_output_context2(&ou_fmt, nullptr, nullptr, out_mp4_path.c_str()) < 0) {
        LOGD("MP2 muxer fail");
        releaseSources();
        return;
    }

    // 添加视频流
    AVStream *stream = avformat_new_stream(ou_fmt, nullptr);
    video_ou_index = stream->index;

    // 创建h264的编码器及编码器上下文
    AVCodec *en_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!en_codec) {
        LOGD("encodec fail");
        releaseSources();
        return;
    }
    en_ctx = avcodec_alloc_context3(en_codec);
    if (!en_ctx) {
        LOGD("en_codec ctx fail");
        releaseSources();
        return;
    }
    // 设置编码参数
    AVStream *in_stream = in_fmt->streams[video_index];
    en_ctx->width = in_stream->codecpar->width;
    en_ctx->height = in_stream->codecpar->height;
    en_ctx->pix_fmt = (enum AVPixelFormat) in_stream->codecpar->format;
    en_ctx->bit_rate = 0.96 * 1000000;
    en_ctx->framerate = (AVRational) {25, 1};
    en_ctx->time_base = (AVRational) {1, 25};
    // 某些封装格式必须要设置，否则会造成封装后文件中信息的缺失
    if (ou_fmt->oformat->flags & AVFMT_GLOBALHEADER) {
        en_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    // x264编码特有
    if (en_codec->id == AV_CODEC_ID_H264) {
        // 代表了编码的速度级别
        av_opt_set(en_ctx->priv_data, "preset", "slow", 0);
        en_ctx->flags2 = AV_CODEC_FLAG2_LOCAL_HEADER;
    }

    // 初始化编码器及编码器上下文
    if (avcodec_open2(en_ctx, en_codec, nullptr) < 0) {
        LOGD("encodec ctx fail");
        releaseSources();
        return;
    }

    // 设置视频流参数;对于封装来说，直接从编码器上下文拷贝即可
    if (avcodec_parameters_from_context(stream->codecpar, en_ctx) < 0) {
        LOGD("copy en_code parameters fail");
        releaseSources();
        return;
    }

    // 初始化封装器输出缓冲区
    if (!(ou_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&ou_fmt->pb, out_mp4_path.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr) < 0) {
            LOGD("avio_open2 fail");
            releaseSources();
            return;
        }
    }

    // 创建像素格式转换器
    sws_ctx = sws_getContext(de_ctx->width, de_ctx->height, de_ctx->pix_fmt,
                             en_ctx->width, en_ctx->height, en_ctx->pix_fmt,
                             0, nullptr, nullptr, nullptr);
    if (!sws_ctx) {
        LOGD("sws_getContext fail");
        releaseSources();
        return;
    }

    // 写入封装器头文件信息；此函数内部会对封装器参数做进一步初始化
    if (avformat_write_header(ou_fmt, nullptr) < 0) {
        LOGD("avformat_write_header fail");
        releaseSources();
        return;
    }

    // 创建编解码用的AVFrame
    de_frame = av_frame_alloc();
    en_frame = av_frame_alloc();
    en_frame->width = en_ctx->width;
    en_frame->height = en_ctx->height;
    en_frame->format = en_ctx->pix_fmt;
    av_frame_get_buffer(en_frame, 0);
    av_frame_make_writable(en_frame);

    AVPacket *in_pkt = av_packet_alloc();
    while (av_read_frame(in_fmt, in_pkt) == 0) {
        if (in_pkt->stream_index != video_index) {
            continue;
        }
        // 先解码
        doDecode(in_pkt);
        av_packet_unref(in_pkt);
    }

    // 刷新解码缓冲区
    doDecode(nullptr);
    av_write_trailer(ou_fmt);
    LOGD("结束。。。");

    // 释放资源
    releaseSources();
}


void jpeg_to_video::doEncode(AVFrame *en_frame1) {
    avcodec_send_frame(en_ctx, en_frame1);
    while (true) {
        AVPacket *ou_pkt = av_packet_alloc();
        if (avcodec_receive_packet(en_ctx, ou_pkt) < 0) {
            av_packet_unref(ou_pkt);
            break;
        }

        // 成功编码了;写入之前要进行时间基的转换
        AVStream *stream = ou_fmt->streams[video_ou_index];
        av_packet_rescale_ts(ou_pkt, en_ctx->time_base, stream->time_base);
        LOGD("video pts %d(%s)", ou_pkt->pts, av_ts2timestr(ou_pkt->pts, &stream->time_base));
        av_write_frame(ou_fmt, ou_pkt);
    }
}

void jpeg_to_video::doDecode(AVPacket *in_pkt) {
    static int num_pts = 0;
    // 先解码
    avcodec_send_packet(de_ctx, in_pkt);
    while (true) {
        int ret = avcodec_receive_frame(de_ctx, de_frame);
        if (ret == AVERROR_EOF) {
            doEncode(NULL);
            break;
        } else if (ret < 0) {
            break;
        }

        // 成功解码了；先进行格式转换然后再编码
        if (sws_scale(sws_ctx, de_frame->data, de_frame->linesize, 0, de_frame->height, en_frame->data,
                      en_frame->linesize) < 0) {
            LOGD("sws_scale fail");
            releaseSources();
            return;
        }

        // 编码前要设置好pts的值，如果en_ctx->time_base为{1,fps}，那么这里pts的值即为帧的个数值
        en_frame->pts = num_pts++;
        doEncode(en_frame);
    }

}

void jpeg_to_video::releaseSources() {
    if (in_fmt) {
        avformat_close_input(&in_fmt);
        in_fmt = nullptr;
    }
    if (ou_fmt) {
        avformat_free_context(ou_fmt);
        ou_fmt = nullptr;
    }

    if (en_frame) {
        av_frame_unref(en_frame);
        en_frame = nullptr;
    }

    if (de_frame) {
        av_frame_unref(de_frame);
        de_frame = nullptr;
    }

    if (en_ctx) {
        avcodec_free_context(&en_ctx);
        en_ctx = nullptr;
    }

    if (de_ctx) {
        avcodec_free_context(&de_ctx);
        de_ctx = nullptr;
    }
}