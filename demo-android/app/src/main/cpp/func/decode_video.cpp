//
// Created by julis.wang on 2021/10/6.
//

#include "decode_video.h"

#define INBUF_SIZE 4096
#define SAVE_PGM_PATH "/storage/emulated/0/ffmpegLearn/pgm";

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename) {
    FILE *f;
    int i;

    f = fopen(filename, "wb");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}

static void decode(AVCodecContext *dec_ctx, AVFrame *frame, AVPacket *pkt,
                   const char *filename) {
    char buf[1024];
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        LOGE("Error sending a packet for decoding:%d", AVERROR(ret));
        return;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            LOGE("Error during decoding\n");
            return;
        }

        LOGE("saving frame %3d\n", dec_ctx->frame_number);
        fflush(stdout);

        /* the picture is allocated by the decoder. no need to
           free it */
        snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
        pgm_save(frame->data[0], frame->linesize[0],
                 frame->width, frame->height, buf);
    }
}

int decode_video::run() {
    const char *filename, *outfilename;
    const AVCodec *codec;
    AVCodecParserContext *parser;
    AVCodecContext *av_codec_context = nullptr;
    FILE *f;
    AVFrame *frame;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t *data;
    size_t data_size;
    int ret;
    AVPacket *pkt;

    filename = h264_file_path;
    outfilename = SAVE_PGM_PATH;

    pkt = av_packet_alloc();
    if (!pkt)
        return -1;

    /* set end of buffer to 0 (this ensures that no overreading happens for damaged MPEG streams) */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    /* find the h264 video decoder */
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        LOGE("Codec not found\n");
        return -1;
    }

    parser = av_parser_init(codec->id);
    if (!parser) {
        LOGE("parser not found\n");
        return -1;
    }

    av_codec_context = avcodec_alloc_context3(codec);
    if (!av_codec_context) {
        LOGE("Could not allocate video codec context\n");
        return -1;
    }

    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */

    /* open it */
    if (avcodec_open2(av_codec_context, codec, nullptr) < 0) {
        LOGE("Could not open codzxlec\n");
        return -1;
    }

    f = fopen(filename, "rb");
    if (!f) {
        LOGE("Could not open %s\n", filename);
        return -1;
    }

    frame = av_frame_alloc();
    if (!frame) {
        LOGE("Could not allocate video frame\n");
        return -1;
    }

    while (!feof(f)) {
        /* read raw data from the input file */
        data_size = fread(inbuf, 1, INBUF_SIZE, f);
        if (!data_size)
            break;

        /* use the parser to split the data into frames */
        data = inbuf;
        while (data_size > 0) {
            ret = av_parser_parse2(parser, av_codec_context, &pkt->data, &pkt->size,
                                   data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (ret < 0) {
                LOGE("Error while parsing\n");
                return -1;
            }
            data += ret;
            data_size -= ret;

            if (pkt->size)
                decode(av_codec_context, frame, pkt, outfilename);
        }
    }

    /* flush the decoder */
    decode(av_codec_context, frame, nullptr, outfilename);

    fclose(f);

    av_parser_close(parser);
    avcodec_free_context(&av_codec_context);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    return 0;
}
