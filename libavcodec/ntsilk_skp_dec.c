#include <SKP_Silk_SDK_API.h>

#include "libavutil/mem.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include "codec_internal.h"
#include "decode.h"
#include "ntsilk_skp.h"

const size_t MAX_BYTES_PER_FRAME = 1024;
const size_t FRAME_LENGTH_MS = 20;
const size_t MAX_API_FS_KHZ = 48;

typedef struct NTSilkSKPDecoderContext
{
    const AVClass *class;
    SKP_SILK_SDK_DecControlStruct dec_control;

    void *dec;

    // uint8_t payload[    MAX_BYTES_PER_FRAME * DECODER_MAX_INPUT_FRAMES * ( MAX_LBRR_DELAY + 1 ) ];
    // uint8_t *payloadEnd = NULL;
    // uint8_t *payloadToDec = NULL;
    // uint8_t FECpayload[ MAX_BYTES_PER_FRAME * DECODER_MAX_INPUT_FRAMES ], *payloadPtr;
    // int16_t nBytesPerPacket[ MAX_LBRR_DELAY + 1 ];
    // int16_t totBytes;
    // int16_t out[ ( ( FRAME_LENGTH_MS * MAX_API_FS_KHZ ) << 1 ) * DECODER_MAX_INPUT_FRAMES ], *outPtr;
} NTSilkSKPDecoderContext;

static int ntsilk_decode_s16le(AVCodecContext *avctx, AVFrame *frame,
                               int *got_frame_ptr, AVPacket *avpkt)
{
    int err;
    NTSilkSKPDecoderContext *sctx = avctx->priv_data;
    int16_t len;
    // int16_t n_bytes;
    int16_t *out;
    // int32_t frames = 0;

    frame->nb_samples = COMMON_MAX_INPUT_SAMPLES; // 20ms * 48000Hz / 1000ms/s = 960 samples
    err = ff_get_buffer(avctx, frame, 0);
    if (err < 0)
    {
        return err;
    }

    // n_bytes = *(int16_t *)avpkt->data;

    // nBytesPerPacket[ MAX_LBRR_DELAY ] = n_bytes;
    // payloadEnd += n_bytes;
    // lost = 0;
    // n_bytes = nBytesPerPacket[ 0 ];
    // payloadToDec = payload;
    // outPtr = out;
    out = (int16_t *)frame->data[0];
    frame->nb_samples = 0;
    // frames = 0;
    do
    {
        /* Decode 20 ms */
        err = SKP_Silk_SDK_Decode(
            sctx->dec,
            &sctx->dec_control,
            0,
            avpkt->data,
            avpkt->size,
            out,
            &len);
        if (err)
        {
            av_log(avctx, AV_LOG_ERROR,
                   "Error decoding frame: %d\n", err);
            return ff_ntsilk_error_to_averror(err);
        }

        // frames++;

        // Unlike memcpy() etc., add len (offset) directly as data type's int16_t
        out += len;
        frame->nb_samples += len;

        // if (frames > DECODER_MAX_INPUT_FRAMES) {
        //     av_log(avctx, AV_LOG_WARNING,
        //            "Corrupt stream detected\n");
        //     out = (int16_t *)frame->data[0];
        //     frame->nb_samples = 0;
        //     frames = 0;
        // }

    } while (sctx->dec_control.moreInternalDecoderFrames);

    // memcpy(frame->data[0], out, frame->nb_samples * 2);

    // // Update buffer
    // totBytes = 0;
    // for (i = 0; i < MAX_LBRR_DELAY; i++) {
    //     totBytes += nBytesPerPacket[i + 1];
    // }
    // memmove(payload, &payload[nBytesPerPacket[0]], totBytes * sizeof(uint8_t));
    // payloadEnd -= nBytesPerPacket[0];
    // memmove(nBytesPerPacket, &nBytesPerPacket[1], MAX_LBRR_DELAY * sizeof(int16_t));

    *got_frame_ptr = 1;

    return avpkt->size;
}

static av_cold int ntsilk_decode_init(AVCodecContext *avctx)
{
    int err;
    NTSilkSKPDecoderContext *sctx = avctx->priv_data;
    int32_t dec_size;

    avctx->sample_rate = 24000;
    avctx->sample_fmt = AV_SAMPLE_FMT_S16;

    sctx->dec_control.API_sampleRate = 24000;
    // Initialize to one frame per packet,
    // for proper concealment before first packet arrives
    sctx->dec_control.framesPerPacket = 1;

    err = SKP_Silk_SDK_Get_Decoder_Size(&dec_size);
    if (err)
    {
        av_log(avctx, AV_LOG_FATAL,
               "Failed to create decoder: %d\n", err);
        return ff_ntsilk_error_to_averror(err);
    }

    sctx->dec = av_malloc(dec_size);
    if (!sctx->dec)
    {
        av_log(avctx, AV_LOG_FATAL,
               "Failed to alloc memory for encoder.\n");
        return AVERROR(ENOMEM);
    }

    err = SKP_Silk_SDK_InitDecoder(sctx->dec);
    if (err)
    {
        av_log(avctx, AV_LOG_FATAL,
               "Failed to create decoder: %d\n", err);
        return ff_ntsilk_error_to_averror(err);
    }

    // sctx->totBytes = 0;
    // sctx->payloadEnd = sctx->payload;

    return 0;
}

static av_cold int ntsilk_decode_close(AVCodecContext *avctx)
{
    NTSilkSKPDecoderContext *sctx = avctx->priv_data;

    av_freep(&sctx->dec);

    return 0;
}

static const AVOption ntsilkdec_options[] = {
    {0},
};

static const AVClass ntsilkdec_class = {
    .class_name = "NTSilk Decoder",
    .item_name = av_default_item_name,
    .option = ntsilkdec_options,
    .version = LIBAVUTIL_VERSION_INT,
};

static const enum AVSampleFormat ntsilk_sample_fmts[] = {
    AV_SAMPLE_FMT_S16,
    AV_SAMPLE_FMT_NONE,
};

static const FFCodecDefault ntsilkdec_defaults[] = {
    {0},
};

const FFCodec ff_ntsilk_skp_s16le_decoder = {
    .p.name = "ntsilk_s16le",
    CODEC_LONG_NAME("NTSilk (s16le) Decoder using SILK SDK"),

    .p.priv_class = &ntsilkdec_class,
    .priv_data_size = sizeof(NTSilkSKPDecoderContext),

    .p.type = AVMEDIA_TYPE_AUDIO,
    .p.id = AV_CODEC_ID_NTSILK_S16LE,

    .defaults = ntsilkdec_defaults,

    .init = ntsilk_decode_init,
    .close = ntsilk_decode_close,
    FF_CODEC_DECODE_CB(ntsilk_decode_s16le),

    .p.capabilities = AV_CODEC_CAP_DR1,
    .caps_internal = FF_CODEC_CAP_NOT_INIT_THREADSAFE | FF_CODEC_CAP_INIT_CLEANUP,

    .p.sample_fmts = ntsilk_sample_fmts,

    .p.wrapper_name = "skp",
};