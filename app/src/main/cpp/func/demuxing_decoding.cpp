//
// Created by julis.wang on 2021/10/5.
//

#include "demuxing_decoding.h"

static AVFormatContext *fmt_ctx = nullptr;
static AVCodecContext *video_dec_ctx = nullptr, *audio_dec_ctx;
static int width, height;
static enum AVPixelFormat pix_fmt;
static AVStream *video_stream = nullptr, *audio_stream = nullptr;
static const char *src_filename = nullptr;

static const char *video_dst_filename = "/storage/emulated/0/video_extract_yuv.yuv";
static const char *audio_dst_filename = "/storage/emulated/0/video_extract_pcm.pcm";
static FILE *video_dst_file = nullptr;
static FILE *audio_dst_file = nullptr;

static uint8_t *video_dst_data[4] = {nullptr};
static int video_dst_line_size[4];
static int video_dst_buf_size;

static int video_stream_idx = -1, audio_stream_idx = -1;
static AVFrame *av_frame = nullptr;
static AVPacket *pkt = nullptr;
static int video_frame_count = 0;
static int audio_frame_count = 0;

static int output_video_frame(AVFrame *frame) {
    if (frame->width != width || frame->height != height ||
        frame->format != pix_fmt) {
        /* To handle this change, one could call av_image_alloc again and
         * decode the following frames into another rawvideo file. */
        LOGE("Error: Width, height and pixel format have to be "
             "constant in a raw video file, but the width, height or "
             "pixel format of the input video changed:"
             "old: width = %d, height = %d, format = %s"
             "new: width = %d, height = %d, format = %s",
             width, height, av_get_pix_fmt_name(pix_fmt),
             frame->width, frame->height,
             av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format)));
        return -1;
    }

    LOGE("video_frame n:%d coded_n:%d",
         video_frame_count++, frame->coded_picture_number);

    /* copy decoded frame to destination buffer:
     * this is required since raw video expects non aligned data */
    av_image_copy(video_dst_data, video_dst_line_size,
                  (const uint8_t **) (frame->data), frame->linesize,
                  pix_fmt, width, height);

    /* write to rawvideo file */
    fwrite(video_dst_data[0], 1, video_dst_buf_size, video_dst_file);
    return 0;
}

static int output_audio_frame(AVFrame *frame) {
    size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(static_cast<AVSampleFormat>(frame->format));
    LOGE("audio_frame n:%d nb_samples:%d pts:%s",
         audio_frame_count++, frame->nb_samples,
         av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));

    /* Write the raw audio data samples of the first plane. This works
     * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
     * most audio decoders output planar audio, which uses a separate
     * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
     * In other words, this code will write only the first audio channel
     * in these cases.
     * You should use libswresample or libavfilter to convert the frame
     * to packed data. */
    fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);

    return 0;
}

static int decode_packet(AVCodecContext *dec, const AVPacket *avPacket) {
    int ret;

    // submit the packet to the decoder
    ret = avcodec_send_packet(dec, avPacket);
    if (ret < 0) {
        LOGE("Error submitting a packet for decoding (%s)", av_err2str(ret));
        return ret;
    }

    // get all the available frames from the decoder
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec, av_frame);
        if (ret < 0) {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return 0;

            LOGE("Error during decoding (%s)", av_err2str(ret));
            return ret;
        }

        // write the frame data to output file
        if (dec->codec->type == AVMEDIA_TYPE_VIDEO)
            ret = output_video_frame(av_frame);
        else
            ret = output_audio_frame(av_frame);

        av_frame_unref(av_frame);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *format_context, enum AVMediaType type) {
    int ret, stream_index;
    AVStream *st;
    const AVCodec *dec;

    ret = av_find_best_stream(format_context, type, -1, -1, nullptr, 0);
    if (ret < 0) {
        LOGE("Could not find %s stream in input file '%s'",
             av_get_media_type_string(type), src_filename);
        return ret;
    } else {
        stream_index = ret;
        st = format_context->streams[stream_index];

        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            LOGE("Failed to find %s codec",
                 av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            LOGE("Failed to allocate the %s codec context",
                 av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            LOGE("Failed to copy %s codec parameters to decoder context",
                 av_get_media_type_string(type));
            return ret;
        }

        /* Init the decoders */
        if ((ret = avcodec_open2(*dec_ctx, dec, nullptr)) < 0) {
            LOGE("Failed to open %s codec",
                 av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

//把一个音视频文件分解为原始视频流YUV文件和原始音频流PCM文件
int demuxing_decoding::run() {
    src_filename = video_file_path;
    int ret = 0;

    /* open input file, and allocate format context */
    if (avformat_open_input(&fmt_ctx, src_filename, nullptr, nullptr) < 0) {
        LOGE("Could not open source file %s", video_file_path);
        exit(1);
    }

    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        LOGE("Could not find stream information");
        exit(1);
    }

    if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];

        video_dst_file = fopen(video_dst_filename, "wb");
        if (!video_dst_file) {
            LOGE("Could not open destination file %s", video_dst_filename);
            ret = 1;
            goto end;
        }

        /* allocate image where the decoded image will be put */
        width = video_dec_ctx->width;
        height = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;
        ret = av_image_alloc(video_dst_data, video_dst_line_size,
                             width, height, pix_fmt, 1);
        if (ret < 0) {
            LOGE("Could not allocate raw video buffer");
            goto end;
        }
        video_dst_buf_size = ret;
    }

    if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
        audio_stream = fmt_ctx->streams[audio_stream_idx];
        audio_dst_file = fopen(audio_dst_filename, "wb");
        if (!audio_dst_file) {
            LOGE("Could not open destination file %s", audio_dst_filename);
            ret = 1;
            goto end;
        }
    }

    if (!audio_stream && !video_stream) {
        LOGE("Could not find audio or video stream in the input, aborting");
        ret = 1;
        goto end;
    }

    av_frame = av_frame_alloc();
    if (!av_frame) {
        LOGE("Could not allocate frame");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        LOGE("Could not allocate packet");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    if (video_stream)
    LOGE("Demuxing video from file '%s' into '%s'", src_filename, video_dst_filename);
    if (audio_stream)
    LOGE("Demuxing audio from file '%s' into '%s'", src_filename, audio_dst_filename);

    /* read frames from the file */
    while (av_read_frame(fmt_ctx, pkt) >= 0) {
        // check if the packet belongs to a stream we are interested in, otherwise
        // skip it
        if (pkt->stream_index == video_stream_idx)
            ret = decode_packet(video_dec_ctx, pkt);
        else if (pkt->stream_index == audio_stream_idx)
            ret = decode_packet(audio_dec_ctx, pkt);
        av_packet_unref(pkt);
        if (ret < 0)
            break;
    }

    end:
    avcodec_free_context(&video_dec_ctx);
    avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&fmt_ctx);
    if (video_dst_file)
        fclose(video_dst_file);
    if (audio_dst_file)
        fclose(audio_dst_file);
    av_packet_free(&pkt);
    av_frame_free(&av_frame);
    av_free(video_dst_data[0]);

    return ret < 0;
}
