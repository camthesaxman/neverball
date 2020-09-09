#include "capture.h"
#include "log.h"
#include "video.h"
#include "glext.h"
#include "audio.h"
#include <assert.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

static AVFormatContext *fctx;
static AVCodecContext *vctx;
static AVCodecContext *actx;
static AVStream *vstream;
static AVStream *astream;
static AVFrame *vframe;
static AVFrame *aframe;
static AVPacket *packet;

static uint32_t frameNum;

static int capture_fps;

static int choose_video_bitrate(int width, int height, int fps)
{
    // This seems roughly appropriate
    return width * height * fps / 10;
}

static int add_stream(int type, AVCodecContext **cctx_ret, AVStream **stream_ret, AVFrame **frame_ret)
{
    int res;
    const char *type_str;
    int codec_id;
    const AVCodec *codec;
    AVCodecContext *cctx;
    AVStream *stream;
    AVFrame *frame;

    switch (type)
    {
    case AVMEDIA_TYPE_VIDEO:
        type_str = "video";
        codec_id = AV_CODEC_ID_H264;
        break;
    case AVMEDIA_TYPE_AUDIO:
        type_str = "audio";
        codec_id = AV_CODEC_ID_MP2;
        break;
    }

    codec = avcodec_find_encoder(codec_id);
    if (codec == NULL)
    {
        log_printf("could not find %s codec\n", type_str);
        return 0;
    }

    cctx = avcodec_alloc_context3(codec);
    if (cctx == NULL)
    {
        log_printf("failed to allocate %s encoder context\n", type_str);
        return 0;
    }

    switch (type)
    {
    case AVMEDIA_TYPE_VIDEO:
        // video options
        cctx->pix_fmt = AV_PIX_FMT_YUV420P;
        cctx->bit_rate = choose_video_bitrate(video.window_w, video.window_h, capture_fps);
        cctx->width = video.window_w;
        cctx->height = video.window_h;
        cctx->max_b_frames = 0;  // Don't use B-frames
        cctx->gop_size = 10;  // key frame occurs every 10 frames
        log_printf("video bitrate: %i Mbps\n", cctx->bit_rate / 1000000);
        break;
    case AVMEDIA_TYPE_AUDIO:
        // audio options
        cctx->sample_fmt = AV_SAMPLE_FMT_S16;
        cctx->bit_rate = 128000;
        cctx->sample_rate = 44100;
        cctx->channels = 2;
        cctx->channel_layout = AV_CH_LAYOUT_STEREO;
        log_printf("audio bitrate: %i Kbps\n", cctx->bit_rate / 1000);
        break;
    }
    // options common to both audio and video
    cctx->framerate = (AVRational){capture_fps, 1};
    cctx->time_base = (AVRational){1, capture_fps};  // use 1/fps as the time-base for both audio and video

    res = avcodec_open2(cctx, codec, NULL);
    if (res < 0)
    {
        log_printf("could not open %s codec: %s\n", type_str, av_err2str(res));
        return 0;
    }

    stream = avformat_new_stream(fctx, codec);
    if (stream == NULL)
    {
        log_printf("failed to create %s stream\n", type_str);
        return 0;
    }
    avcodec_parameters_from_context(stream->codecpar, cctx);

    frame = av_frame_alloc();
    if (frame == NULL)
    {
        log_printf("Could not allocate %s frame\n", type_str);
        return 0;
    }

    // Set frame options
    switch (type)
    {
    case AVMEDIA_TYPE_VIDEO:
        frame->format = cctx->pix_fmt;
        frame->width = cctx->width;
        frame->height = cctx->height;
        break;
    case AVMEDIA_TYPE_AUDIO:
        frame->format = cctx->sample_fmt;
        frame->nb_samples = cctx->frame_size;
        frame->channel_layout = cctx->channel_layout;
        break;
    }

    // Allocate buffer
    res = av_frame_get_buffer(frame, 0);
    if (res < 0)
    {
        log_printf("Could not allocate frame buffer\n");
        return 0;
    }

    *cctx_ret = cctx;
    *stream_ret = stream;
    *frame_ret = frame;

    return 1;
}

void capture_init(const char *filename, int fps)
{
    const AVCodec *vcodec;
    const AVCodec *acodec;
    int res;

    capture_fps = fps;

    av_register_all();
    avcodec_register_all();

    AVOutputFormat *outfmt = av_guess_format("mp4", NULL, "video/mp4");
    if (outfmt == NULL)
    {
        log_printf("unable to use MP4\n");
        return;
    }

    res = avformat_alloc_output_context2(&fctx, outfmt, NULL, NULL);
    if (res < 0)
    {
        log_printf("could not alloc output context: %s\n", av_err2str(res));
        return;
    }

    add_stream(AVMEDIA_TYPE_VIDEO, &vctx, &vstream, &vframe);
    add_stream(AVMEDIA_TYPE_AUDIO, &actx, &astream, &aframe);

    // Open output file

    res = avio_open(&fctx->pb, filename, AVIO_FLAG_READ_WRITE);
    if (res < 0)
    {
        log_printf("could not open output file: %s\n", av_err2str(res));
        return;
    }

    res = avformat_write_header(fctx, NULL);
    if (res < 0)
    {
        log_printf("failed to write header: %s\n", av_err2str(res));
        return;
    }

    packet = av_packet_alloc();
    if (packet == NULL)
    {
        log_printf("Failed to allocate packet\n");
        return;
    }

    frameNum = 0; 
}

/*
 * Encodes a single audio or video frame to the stream
 */
static void encode_to_stream(AVFrame *frame, AVStream *stream, AVCodecContext *cctx)
{
    int ret;

    // This doesn't seem to make it sync. Why not?
    if (frame != NULL)
        frame->pts = frameNum;

    ret = avcodec_send_frame(cctx, frame);
    if (ret < 0)
    {
        log_printf("Failed to send frame for encoding\n");
        return;
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_packet(cctx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0)
        {
            fprintf(stderr, "encoding error\n");
            return;
        }

        packet->stream_index = stream->index;

        ret = av_interleaved_write_frame(fctx, packet);
        if (ret < 0)
            log_printf("error writing packet: %s\n", av_err2str(ret));

        av_packet_unref(packet);
    }
}

static void capture_video_frame(void)
{
    int w = video.window_w;
    int h = video.window_h;
    uint8_t *image = malloc(w * h * 4);
    int ret;
    int x, y;

    ret = av_frame_make_writable(vframe);
    if (ret < 0)
    {
        log_printf("Failed to make frame writable\n");
        return;
    }

    // Read image from GL context
    glFinish();
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // Convert from RGBA to YCbCr
    // Y
    for (y = 0; y < vframe->height; y++)
    {
        for (x = 0; x < vframe->width; x++)
        {
            int srcX = x;
            int srcY = h - 1 - y;

            int r = image[(srcY * w + srcX) * 4 + 0];
            int g = image[(srcY * w + srcX) * 4 + 1];
            int b = image[(srcY * w + srcX) * 4 + 2];

            vframe->data[0][y * vframe->linesize[0] + x] = 16 + (65 * r + 129 * g + 25 * b) / 256; 
        }
    }
    // Cb, Cr
    for (y = 0; y < vframe->height / 2; y++)
    {
        for (x = 0; x < vframe->width / 2; x++)
        {
            int srcX = x * 2;
            int srcY = (h - 1 - y * 2);

            int r = image[(srcY * w + srcX) * 4 + 0];
            int g = image[(srcY * w + srcX) * 4 + 1];
            int b = image[(srcY * w + srcX) * 4 + 2];

            vframe->data[1][y * vframe->linesize[1] + x] = 128 + (-38 * r - 74 * g + 112 * b) / 256;
            vframe->data[2][y * vframe->linesize[2] + x] = 128 + (112 * r - 94 * g - 18  * b) / 256;
        }
    }

    free(image);

    encode_to_stream(vframe, vstream, vctx);
}

static void capture_audio_frame(void)
{
    size_t bufsize = actx->frame_size * actx->channels * sizeof(int16_t);

    // Render one frame of audio
    audio_step(NULL, aframe->data[0], bufsize);

    av_frame_make_writable(aframe);

    encode_to_stream(aframe, astream, actx);
}

/*
 * Captures the audio and video of a single frame and encodes it. 
 */
void capture_put_frame(void)
{
    capture_video_frame();
    capture_audio_frame();

    frameNum++;
}

void capture_quit(void)
{
    // flush encoder
    encode_to_stream(NULL, vstream, vctx);
    encode_to_stream(NULL, astream, actx);

    av_write_trailer(fctx);

    // close video codec
    avcodec_free_context(&vctx);
    av_frame_free(&vframe);

    // close audio codec
    avcodec_free_context(&actx);
    av_frame_free(&aframe);

    av_packet_free(&packet);

    // close output file
    avio_closep(&fctx->pb);

    // free stream
    avformat_free_context(fctx);
}
