#include <SKP_Silk_SDK_API.h>

#include "libavutil/channel_layout.h"
#include "libavutil/mem.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include "codec_internal.h"
#include "encode.h"
#include "ntsilk_skp.h"

// typedef struct NTSilkSKPEncoderOptions {
// } NTSilkSKPEncoderOptions;

typedef struct NTSilkSKPEncoderContext
{
    const AVClass *class;
    SKP_SILK_SDK_EncControlStruct enc_control;
    SKP_SILK_SDK_EncControlStruct enc_status;

    void *enc;

    // uint8_t in[COMMON_MAX_INPUT_SIZE];
    // uint8_t out[ENCODER_MAX_PAYLOAD_BYTES];

    // AudioFrameQueue afq;

    // NTSilkSKPEncoderOptions opts;

    // int32_t samples_since_last_packet;
} NTSilkSKPEncoderContext;

static int ntsilk_encode_s16le(struct AVCodecContext *avctx, struct AVPacket *avpkt,
                               const struct AVFrame *frame, int *got_packet_ptr)
{
    int err;
    NTSilkSKPEncoderContext *sctx = avctx->priv_data;
    size_t counter;
    int16_t n_bytes;

    // frame is not null
    counter = frame->nb_samples; // 20ms * 48000Hz / 1000ms/s = 960 samples

    // // Cut the last audio samples that < 20ms
    // if (counter < ((20 * sctx->enc_control.API_sampleRate) / 1000)) {
    //     break;
    // }

    n_bytes = ENCODER_MAX_PAYLOAD_BYTES;

    // 2 = s16 header
    if ((err = ff_alloc_packet(avctx, avpkt, ENCODER_MAX_OUT_BYTES)) < 0)
        return err;

    err = SKP_Silk_SDK_Encode(
        sctx->enc,
        &sctx->enc_control,
        (const int16_t *)frame->data[0],
        counter,
        avpkt->data + 2, // sizeof(int16_t)
        &n_bytes);       // Must use sctx.n_bytes and not avpkt->size as it's I/O par
    if (err)
    {
        av_log(avctx, AV_LOG_ERROR,
               "Error encoding frame: %d\n", err);
        return ff_ntsilk_error_to_averror(err);
    }

    // sctx->samples_since_last_packet += sctx->counter;

    // if( ( ( 1000 * sctx->samples_since_last_packet ) / sctx->enc_control.API_sampleRate ) == 20 ) {
    // Always true
    memcpy(avpkt->data, &n_bytes, 2);

    // sctx->samples_since_last_packet = 0;
    // }

    av_shrink_packet(avpkt, n_bytes + 2);

    *got_packet_ptr = 1;

    return 0;
}

// static av_cold void ntsilk_encode_flush(AVCodecContext *ctx)
// {
// }

static av_cold int ntsilk_encode_init(AVCodecContext *avctx)
{
    int err;
    NTSilkSKPEncoderContext *sctx = avctx->priv_data;
    int32_t enc_size;

    // Set encoder par
    sctx->enc_control.API_sampleRate = avctx->sample_rate;                                                 // Input sample rate
    sctx->enc_control.maxInternalSampleRate = 24000;                                                       // Output sample rate
    sctx->enc_control.packetSize = avctx->frame_size = 20 /* ms */ * avctx->sample_rate / 1000 /* s/ms */; // Input frame(packet) size
    sctx->enc_control.packetLossPercentage = 0;
    sctx->enc_control.useInBandFEC = 0;
    sctx->enc_control.useDTX = 0;
    sctx->enc_control.complexity = 2;

    // if (!avctx->bit_rate) {
    //     sctx->enc_control.bitRate = avctx->bit_rate       = 25000;
    //     av_log(avctx, AV_LOG_WARNING,
    //            "No bit rate set. Defaulting to %"PRId64" bps.\n", avctx->bit_rate);
    // }
    // else {
    //     sctx->enc_control.bitRate = avctx->bit_rate;
    // }

    // AF limit: 150 bytes
    // Max bitrate:
    // 150 (bytes) * 1000ms/s / 20ms = 7500byte = 60000bps
    // To prevent data overflow, set target bitrate to 30000bps
    sctx->enc_control.bitRate = avctx->bit_rate = 30000;

    // Init encoder
    err = SKP_Silk_SDK_Get_Encoder_Size(&enc_size);
    if (err)
    {
        av_log(avctx, AV_LOG_ERROR,
               "Failed to create encoder: %d\n", err);
        return ff_ntsilk_error_to_averror(err);
    }

    sctx->enc = av_malloc(enc_size);
    if (!sctx->enc)
    {
        av_log(avctx, AV_LOG_FATAL,
               "Failed to alloc memory for encoder.\n");
        return AVERROR(ENOMEM);
    }

    err = SKP_Silk_SDK_InitEncoder(sctx->enc, &sctx->enc_status);
    if (err)
    {
        av_log(avctx, AV_LOG_FATAL,
               "Failed to create encoder: %d\n", err);
        return ff_ntsilk_error_to_averror(err);
    }

    // sctx->samples_since_last_packet = 0;

    return 0;
}

static av_cold int ntsilk_encode_close(AVCodecContext *avctx)
{
    NTSilkSKPEncoderContext *sctx = avctx->priv_data;

    av_freep(&sctx->enc);

    return 0;
}

static const AVOption ntsilkenc_options[] = {
    {0},
};

static const AVClass ntsilkenc_class = {
    .class_name = "NTSilk Encoder",
    .item_name = av_default_item_name,
    .option = ntsilkenc_options,
    .version = LIBAVUTIL_VERSION_INT,
};

static const int ntsilk_supported_samplerates[] = {
    48000,
    44100,
    32000,
    24000,
    16000,
    12000,
    8000,
    0,
};

static const AVChannelLayout ntsilk_ch_layouts[] = {
    AV_CHANNEL_LAYOUT_MONO,
    {0},
};

static const enum AVSampleFormat ntsilk_sample_fmts[] = {
    AV_SAMPLE_FMT_S16,
    AV_SAMPLE_FMT_NONE,
};

static const FFCodecDefault ntsilkenc_defaults[] = {
    {0},
};

const FFCodec ff_ntsilk_skp_s16le_encoder = {
    .p.name = "ntsilk_s16le",
    CODEC_LONG_NAME("NTSilk (s16le) Encoder using SILK SDK"),

    .p.priv_class = &ntsilkenc_class,
    .priv_data_size = sizeof(NTSilkSKPEncoderContext),

    .p.type = AVMEDIA_TYPE_AUDIO,
    .p.id = AV_CODEC_ID_NTSILK_S16LE,

    .defaults = ntsilkenc_defaults,

    .init = ntsilk_encode_init,
    .close = ntsilk_encode_close,
    FF_CODEC_ENCODE_CB(ntsilk_encode_s16le),

    .p.capabilities = // Must use get_buffer() or get_encode_buffer()
                      // for allocating buffers.
                      //
    AV_CODEC_CAP_DR1
    //
    // NTSilk groups data as 20ms slices and will drop the last slice
    // when data size less than 20ms.
    //
    // The ntsilk_encode_xxxxx() calling
    // is very likely to get AVFrame with size less than 20ms.
    //
    // We use AFQ to temporarily store data in memory.
    // Each ntsilk_encode_xxxxx() calling will encode afq to <20ms.
    //
    // As a result, we automatically support variable AVFrame size.
    //
    // | AV_CODEC_CAP_SMALL_LAST_FRAME
    // | AV_CODEC_CAP_VARIABLE_FRAME_SIZE
    //
    // Since we have fixed the size of each AVPacket to 20ms,
    // another possible situation is -
    // if the size of the input AVFrame is larger than 20ms,
    // the data in AFQ will accumulate.
    //
    // To solve this problem, we must enable DELAY.
    // When DELAY is set,
    // libavcodec will repeatedly call ntsilk_encode_xxxxx()
    // without AVFrame, until there is no AVPacket output.
    // We use DELAY to encode the data accumulated in the AFQ.
    //
    // Note that this does not actually solve
    // the overflow problem.
    //
    // | AV_CODEC_CAP_DELAY
    ,

    .caps_internal = FF_CODEC_CAP_NOT_INIT_THREADSAFE | FF_CODEC_CAP_INIT_CLEANUP,

    .p.supported_samplerates = ntsilk_supported_samplerates,
    .p.ch_layouts = ntsilk_ch_layouts,
    .p.sample_fmts = ntsilk_sample_fmts,

    .p.wrapper_name = "skp",
};