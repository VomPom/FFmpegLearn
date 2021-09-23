//
// Created by julis.wang on 2021/9/22.
//

#include "simple_player.h"
#include "utils.h"
#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
};
#endif

static int index = 0;

int simple_player::play() {
    char filepath[] = "/storage/emulated/0/test2.mp4";

    AVFormatContext *pFormatCtx;
    int i, video_index;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameYUV;
    AVPacket *packet;
    struct SwsContext *img_convert_ctx;
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, filepath, nullptr, nullptr) != 0) {
        LOGE("Couldn't open input stream.\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
        LOGE("Couldn't find stream information.\n");
        return -1;
    }
    video_index = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_index = i;
            break;
        }
    if (video_index == -1) {
        printf("Didn't find a video stream.\n");
        return -1;
    }
    pCodecCtx = pFormatCtx->streams[video_index]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == nullptr) {
        printf("Codec not found.\n");
        return -1;
    } else {
        LOGD("find pCodec:%s", pCodec->name);
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }

    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    printf("---------------File Information------------------\n");
    av_dump_format(pFormatCtx, 0, filepath, 0);
    printf("-------------------------------------------------\n");

    img_convert_ctx = sws_getContext(pCodecCtx->width,
                                     pCodecCtx->height,
                                     pCodecCtx->pix_fmt,
                                     pCodecCtx->width,
                                     pCodecCtx->height,
                                     AV_PIX_FMT_YUV420P,
                                     SWS_BICUBIC, nullptr, nullptr, nullptr);

    while (true) {
        if (av_read_frame(pFormatCtx, packet) >= 0) {
            if (packet->stream_index == video_index) {
                //ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet); //old version
                switch (avcodec_send_packet(pCodecCtx, packet)) { //new version
                    case AVERROR_EOF: {
                        LOGE("Decode error: %s", av_err2str(AVERROR_EOF));
                        return NULL; //解码结束
                    }
                    case AVERROR(EAGAIN):
                    LOGE("Decode error: %s", av_err2str(AVERROR(EAGAIN)));
                        break;
                    case AVERROR(EINVAL):
                    LOGE("Decode error: %s", av_err2str(AVERROR(EINVAL)));
                        break;
                    case AVERROR(ENOMEM):
                    LOGE("Decode error: %s", av_err2str(AVERROR(ENOMEM)));
                        break;
                    default:
                        break;
                }
                int result = avcodec_receive_frame(pCodecCtx, pFrame);
                if (result == 0) {
                    if (index % 25 == 0) {
                        LOGE("--julis got_picture:%d", index / 25);
                    }
                    index++;
                    av_packet_unref(packet);
                } else {
                    LOGE("Receive frame error result: %s", av_err2str(AVERROR(result)));
                }
            }
        } else {
            //Exit
            break;
        }
    }
    sws_freeContext(img_convert_ctx);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    return 0;
}
