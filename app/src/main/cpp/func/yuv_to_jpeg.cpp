//
// Created by julis.wang on 2021/9/26.
//

#include "yuv_to_jpeg.h"


/**
 * YUV编码为JPEG
 * https://blog.csdn.net/leixiaohua1020/article/details/25346147
 * @return
 */
int yuv_to_jpeg::run() {
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