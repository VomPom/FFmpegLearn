//
// Created by julis.wang on 2021/10/7.
//

#include "filtering_video.h"

#define _XOPEN_SOURCE 600 /* for usleep */

#ifdef __cplusplus
extern "C"
{
#endif
#include <unistd.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/avfilter.h>
#include <libavutil/opt.h>
#ifdef __cplusplus
}
#endif


const char *filter_descr = "movie=/storage/emulated/0/test_png.png[wm];[in][wm]overlay=1:1[out]";;

static AVFormatContext *pFormatCtx;
static AVCodecContext *pCodecCtx;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int video_stream_index = -1;


static int open_input_file(const char *filename) {
    int ret;
    AVCodec *dec;

    if ((ret = avformat_open_input(&pFormatCtx, filename, nullptr, nullptr)) < 0) {
        LOGE("Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(pFormatCtx, nullptr)) < 0) {
        LOGE("Cannot find stream information\n");
        return ret;
    }

    /* select the video stream */
    ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
    if (ret < 0) {
        LOGE("Cannot find a video stream in the input file\n");
        return ret;
    }
    video_stream_index = ret;
    pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;

    /* init the video decoder */
    if ((ret = avcodec_open2(pCodecCtx, dec, nullptr)) < 0) {
        LOGE("Cannot open video decoder\n");
        return ret;
    }

    return 0;
}


static int init_filters(const char *filters_descr) {
    char args[512];
    int ret;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};
    AVBufferSinkParams *buffersink_params;

    filter_graph = avfilter_graph_alloc();

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
             pCodecCtx->time_base.num, pCodecCtx->time_base.den,
             pCodecCtx->sample_aspect_ratio.num, pCodecCtx->sample_aspect_ratio.den);


    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, nullptr, filter_graph);
    if (ret < 0) {
        LOGE("Cannot create buffer source\n");
        return ret;
    }

    /* buffer video sink: to terminate the filter chain. */
    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       nullptr, buffersink_params, filter_graph);
    av_free(buffersink_params);
    if (ret < 0) {
        LOGE("Cannot create buffer sink\n");
        return ret;
    }

    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = nullptr;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = nullptr;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs, nullptr)) < 0)
        return ret;

    if ((ret = avfilter_graph_config(filter_graph, nullptr)) < 0)
        return ret;
    return 0;
}


int filtering_video::run() {
    int ret;
    AVPacket packet;
    AVFrame *pFrame;
    AVFrame *pFrame_out;
    FILE *fp_yuv = fopen("/storage/emulated/0/filter_yuv.yuv", "wb+");

    int got_frame;

    if ((ret = open_input_file(mp4_file_path)) < 0)
        goto end;
    if ((ret = init_filters(filter_descr)) < 0)
        goto end;

    pFrame = av_frame_alloc();
    pFrame_out = av_frame_alloc();

    /* read all packets */
    while (1) {

        ret = av_read_frame(pFormatCtx, &packet);
        if (ret < 0)
            break;

        if (packet.stream_index == video_stream_index) {
            got_frame = 0;
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, &packet);
            if (ret < 0) {
                LOGE("Error decoding video\n");
                break;
            }

            if (got_frame) {
                pFrame->pts = av_frame_get_best_effort_timestamp(pFrame);

                /* push the decoded frame into the filtergraph */
                if (av_buffersrc_add_frame(buffersrc_ctx, pFrame) < 0) {
                    LOGE("Error while feeding the filtergraph\n");
                    break;
                }

                /* pull filtered pictures from the filtergraph */
                while (true) {

                    ret = av_buffersink_get_frame(buffersink_ctx, pFrame_out);
                    if (ret < 0)
                        break;

                    LOGE("Process 1 frame!\n");

                    if (pFrame_out->format == AV_PIX_FMT_YUV420P) {
                        //Y, U, V
                        for (int i = 0; i < pFrame_out->height; i++) {
                            fwrite(pFrame_out->data[0] + pFrame_out->linesize[0] * i, 1, pFrame_out->width, fp_yuv);
                        }
                        for (int i = 0; i < pFrame_out->height / 2; i++) {
                            fwrite(pFrame_out->data[1] + pFrame_out->linesize[1] * i, 1, pFrame_out->width / 2, fp_yuv);
                        }
                        for (int i = 0; i < pFrame_out->height / 2; i++) {
                            fwrite(pFrame_out->data[2] + pFrame_out->linesize[2] * i, 1, pFrame_out->width / 2, fp_yuv);
                        }

                    }
                    av_frame_unref(pFrame_out);
                }
            }
            av_frame_unref(pFrame);
        }
        av_packet_unref(&packet);
    }
    fclose(fp_yuv);

    end:
    avfilter_graph_free(&filter_graph);
    if (pCodecCtx)
        avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);


    if (ret < 0 && ret != AVERROR_EOF) {
        char buf[1024];
        av_strerror(ret, buf, sizeof(buf));
        LOGE("Error occurred: %s\n", buf);
        return -1;
    }

    return 0;
}
