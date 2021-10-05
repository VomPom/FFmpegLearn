//
// Created by julis.wang on 2021/9/27.
//

#include "yuv_to_video.h"

static int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index);

int yuv_to_video::run() {
    AVFormatContext *pFormatCtx = nullptr;
    AVOutputFormat *fmt;
    AVStream *video_st;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket pkt;
    uint8_t *picture_buf = nullptr;
    AVFrame *av_frame;

    FILE *in_file = fopen(yuv_file_path, "rb");
    if (!in_file) {
        LOGE("can not open file!");
        return -1;
    }
    int frame_num = 150;
    const char *out_file = "/storage/emulated/0/yuv_to_video.mp4";    //Output file
    avformat_alloc_output_context2(&pFormatCtx, nullptr, nullptr, out_file);
    fmt = pFormatCtx->oformat;

    if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE)) {
        LOGE("output file open fail!");
        return -1;
    }

    video_st = avformat_new_stream(pFormatCtx, 0);
    if (video_st == nullptr) {
        printf("failed allocating output stram\n");
        return -1;
    }
    video_st->time_base.num = 1;
    video_st->time_base.den = 25;

    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = fmt->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = yuv_w;
    pCodecCtx->height = yuv_h;
    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;
    pCodecCtx->bit_rate = 400000;
    pCodecCtx->gop_size = 12;

    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        int *a = NULL;
        pCodecCtx->qmin = 10;
        pCodecCtx->qmax = 51;
        pCodecCtx->qcompress = 0.6;
    }
    if (pCodecCtx->codec_id == AV_CODEC_ID_MPEG2VIDEO)
        pCodecCtx->max_b_frames = 2;
    if (pCodecCtx->codec_id == AV_CODEC_ID_MPEG1VIDEO)
        pCodecCtx->mb_decision = 2;

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) {
        LOGE("no right encoder!");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
        LOGE("open encoder fail!");
        return -1;
    }


    av_frame = av_frame_alloc();
    av_frame->width = pCodecCtx->width;
    av_frame->height = pCodecCtx->height;
    av_frame->format = pCodecCtx->pix_fmt;

    av_frame = av_frame_alloc();
    int numBytes = av_image_get_buffer_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, 1);
    auto *m_buf_for_rgb_frame = (uint8_t *) av_malloc(numBytes);
    av_image_fill_arrays(av_frame->data,
                         av_frame->linesize,
                         m_buf_for_rgb_frame,
                         pCodecCtx->pix_fmt,
                         pCodecCtx->width,
                         pCodecCtx->height, 1);

    avformat_write_header(pFormatCtx, nullptr);
    int y_size = pCodecCtx->width * pCodecCtx->height;
    av_new_packet(&pkt, numBytes);

    for (int i = 0; i < frame_num; i++) {
        if (fread(m_buf_for_rgb_frame, 1, y_size * 3 / 2, in_file) < 0) {
            LOGE("read file fail!");
            break;
        } else if (feof(in_file)) {
            LOGE("read file EOF!");
            break;
        }
        av_frame->data[0] = m_buf_for_rgb_frame; //亮度Y
        av_frame->data[1] = m_buf_for_rgb_frame + y_size; //U
        av_frame->data[2] = m_buf_for_rgb_frame + y_size * 5 / 4; //V
        av_frame->pts = i;

        // 编码一帧视频。即将AVFrame（存储YUV像素数据）
        // 编码为AVPacket（存储H.264等格式的码流数据）。
        avcodec_send_frame(pCodecCtx, av_frame);
        int got_picture = avcodec_receive_packet(pCodecCtx, &pkt);

        if (got_picture == 0) {
            LOGE("encoder success! co:%d", i);
            pkt.stream_index = video_st->index;
            av_packet_rescale_ts(&pkt, pCodecCtx->time_base, video_st->time_base);
            pkt.pos = -1;
            av_interleaved_write_frame(pFormatCtx, &pkt);
            av_packet_unref(&pkt);
        } else {
            LOGE("encoder failed! code:%d", i);
        }
    }

    int ret = flush_encoder(pFormatCtx, 0);
    if (ret < 0) {
        LOGE("flushing encoder failed!");
        return -1;
    }

    av_write_trailer(pFormatCtx);
    avcodec_close(video_st->codec);
    av_free(av_frame);
    av_free(picture_buf);
    if (pFormatCtx) {
        avio_close(pFormatCtx->pb);
        avformat_free_context(pFormatCtx);
    }

    fclose(in_file);

    return 0;
}

int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;
    while (1) {
        printf("Flushing stream #%u encoder\n", stream_index);
        enc_pkt.data = NULL;
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
        // parpare packet for muxing
        enc_pkt.stream_index = stream_index;
        av_packet_rescale_ts(&enc_pkt,
                             fmt_ctx->streams[stream_index]->codec->time_base,
                             fmt_ctx->streams[stream_index]->time_base);
        ret = av_interleaved_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}
