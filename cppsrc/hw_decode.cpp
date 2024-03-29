//
// Created by julis.wang on 2021/10/6.
//

#include <string>
#include "hw_decode.h"

static AVBufferRef *hw_device_ctx = nullptr;
static enum AVPixelFormat hw_pix_fmt;
static FILE *output_file = nullptr;



static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type) {
    int err;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      nullptr, nullptr, 0)) < 0) {
        LOGE("Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts) {
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    LOGE("Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

static int decode_write(AVCodecContext *avctx, AVPacket *packet) {
    AVFrame *frame = nullptr, *sw_frame = nullptr;
    AVFrame *tmp_frame;
    uint8_t *buffer = nullptr;
    int size;
    int ret;

    ret = avcodec_send_packet(avctx, packet);
    if (ret < 0) {
        LOGE("Error during decoding\n");
        return ret;
    }

    while (true) {
        if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
            LOGE("Can not alloc frame\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }

        ret = avcodec_receive_frame(avctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_frame_free(&frame);
            av_frame_free(&sw_frame);
            return 0;
        } else if (ret < 0) {
            LOGE("Error while decoding\n");
            goto fail;
        }
        if (frame->format == hw_pix_fmt) {
            /* retrieve data from GPU to CPU */
            if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
                LOGE("Error transferring the data to system memory\n");
                goto fail;
            }
            tmp_frame = sw_frame;
        } else
            tmp_frame = frame;
        size = av_image_get_buffer_size(static_cast<AVPixelFormat>(tmp_frame->format), tmp_frame->width,
                                        tmp_frame->height, 1);
        buffer = static_cast<uint8_t *>(av_malloc(size));
        if (!buffer) {
            LOGE("Can not alloc buffer\n");
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        ret = av_image_copy_to_buffer(buffer, size,
                                      (const uint8_t *const *) tmp_frame->data,
                                      (const int *) tmp_frame->linesize, static_cast<AVPixelFormat>(tmp_frame->format),
                                      tmp_frame->width, tmp_frame->height, 1);
        if (ret < 0) {
            LOGE("Can not copy image to buffer\n");
            goto fail;
        }

        if ((ret = fwrite(buffer, 1, size, output_file)) < 0) {
            LOGE("Failed to dump raw data.\n");
            goto fail;
        }

        fail:
        av_frame_free(&frame);
        av_frame_free(&sw_frame);
        av_freep(&buffer);
        if (ret < 0)
            return ret;
    }
}

/**
 * 实现利用硬件进行解码
 * 这里测试在 mac 上使用的 videotoolbox 解码出来的 yuv 使用 ffplay 播放，
 * 颜色有一些不一致，是因为硬解出来的像素格式是 nv12，还需做一次转化
 * @param mp4_path  目标 mp4 视频
 * @param output_dir 输出的文件目录
 * @param hw_name 硬解码的名字
 * @return
 */
int hw_decode::run(const string &mp4_path, const string &output_dir, const string &hw_name) {

    AVFormatContext *input_ctx = nullptr;
    int video_stream, ret;
    AVStream *video;
    AVCodecContext *decoder_ctx = nullptr;
    AVCodec *decoder = nullptr;
    AVPacket packet;
    enum AVHWDeviceType type;
    int i;

    type = av_hwdevice_find_type_by_name(hw_name.c_str());
    if (type == AV_HWDEVICE_TYPE_NONE) {
        LOGE("Cannot find hw_codec: '%s'\n", hw_name.c_str());
        return -1;
    }
    /* open the input file */
    if (avformat_open_input(&input_ctx, mp4_path.c_str(), nullptr, nullptr) != 0) {
        LOGE("Cannot open input file '%s'\n", mp4_path.c_str());
        return -1;
    }

    if (avformat_find_stream_info(input_ctx, nullptr) < 0) {
        LOGE("Cannot find input stream information.\n");
        return -1;
    }

    /* find the video stream information */
    ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
    if (ret < 0) {
        LOGE("Cannot find a video stream in the input file\n");
        return -1;
    }
    video_stream = ret;

    for (i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
        if (!config) {
            LOGE("Decoder %s does not support device type %s.\n",
                 decoder->name, av_hwdevice_get_type_name(type));
            return -1;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
            config->device_type == type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

    if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
        return AVERROR(ENOMEM);

    video = input_ctx->streams[video_stream];
    if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
        return -1;

    decoder_ctx->get_format = get_hw_format;

    if (hw_decoder_init(decoder_ctx, type) < 0)
        return -1;

    if ((ret = avcodec_open2(decoder_ctx, decoder, nullptr)) < 0) {
        LOGE("Failed to open codec for stream #%u\n", video_stream);
        return -1;
    }

    string savePath = output_dir + "hw_decode_"
                      + to_string(decoder_ctx->width)
                      + "x" + to_string(decoder_ctx->height)
                      + ".yuv";

    /* open the file to dump raw data */
    output_file = fopen(savePath.c_str(), "w+b");

    /* actual decoding and dump the raw data */
    while (ret >= 0) {
        if ((ret = av_read_frame(input_ctx, &packet)) < 0)
            break;

        if (video_stream == packet.stream_index)
            ret = decode_write(decoder_ctx, &packet);

        av_packet_unref(&packet);
    }

    /* flush the decoder */
    packet.data = nullptr;
    packet.size = 0;
    ret = decode_write(decoder_ctx, &packet);
    av_packet_unref(&packet);

    if (output_file)
        fclose(output_file);
    avcodec_free_context(&decoder_ctx);
    avformat_close_input(&input_ctx);
    av_buffer_unref(&hw_device_ctx);

    return 0;
}
