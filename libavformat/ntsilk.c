
#include "libavutil/opt.h"
#include "config_components.h"
#include "avformat.h"
#include "avio_internal.h"
#include "demux.h"
#include "internal.h"
#include "mux.h"
#include "rawdec.h"
#include "rawenc.h"

static const uint8_t NTSILK_SKP_HEADER[9] = "#!SILK_V3";
static const uint8_t NTSILK_TCT_HEADER[10] = "\x02#!SILK_V3";

typedef struct NTSilkDemuxerContext
{
    // int64_t pos;
} NTSilkDemuxerContext;

static int ntsilk_probe_skp(const AVProbeData *p)
{
    if (!memcmp(p->buf, NTSILK_SKP_HEADER, sizeof(NTSILK_SKP_HEADER)))
        return AVPROBE_SCORE_MAX;
    else
        return 0;
}

static int ntsilk_probe_tct(const AVProbeData *p)
{
    if (!memcmp(p->buf, NTSILK_TCT_HEADER, sizeof(NTSILK_TCT_HEADER)))
        return AVPROBE_SCORE_MAX;
    else
        return 0;
}

static int ntsilk_read_header_skp(AVFormatContext *s)
{
    NTSilkDemuxerContext *sctx = s->priv_data;
    AVIOContext *pb = s->pb;
    AVStream *st;
    int64_t pos;

    pos = avio_skip(pb, sizeof(NTSILK_SKP_HEADER));
    if (pos < 0)
    {
        return pos;
    }

    st = avformat_new_stream(s, NULL);
    if (!st)
        return AVERROR(ENOMEM);

    st->codecpar->codec_id = AV_CODEC_ID_NTSILK_S16LE;
    st->codecpar->sample_rate = 24000;
    st->codecpar->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_MONO;
    st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    ffstream(st)->need_parsing = AVSTREAM_PARSE_FULL_RAW;
    st->start_time = 0;
    avpriv_set_pts_info(st, 64, 1, st->codecpar->sample_rate);

    // sctx->pos = 0;

    return 0;
}

static int ntsilk_read_header_tct(AVFormatContext *s)
{
    NTSilkDemuxerContext *sctx = s->priv_data;
    AVIOContext *pb = s->pb;
    AVStream *st;
    int64_t pos;

    pos = avio_skip(pb, sizeof(NTSILK_TCT_HEADER));
    if (pos < 0)
    {
        return pos;
    }

    st = avformat_new_stream(s, NULL);
    if (!st)
        return AVERROR(ENOMEM);

    st->codecpar->codec_id = AV_CODEC_ID_NTSILK_S16LE;
    st->codecpar->sample_rate = 24000;
    st->codecpar->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_MONO;
    st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    ffstream(st)->need_parsing = AVSTREAM_PARSE_FULL_RAW;
    st->start_time = 0;
    avpriv_set_pts_info(st, 64, 1, st->codecpar->sample_rate);

    // sctx->pos = 0;

    return 0;
}

static int ntsilk_read_packet_s16le(AVFormatContext *s, AVPacket *pkt)
{
    NTSilkDemuxerContext *sctx = s->priv_data;
    int err;
    AVIOContext *pb = s->pb;
    int16_t n_bytes;

    err = avio_read(pb, (uint8_t *)&n_bytes, 2);
    if (err < 0)
    {
        return err;
    }

    if (n_bytes <= 0)
    {
        return 0;
    }

    err = av_new_packet(pkt, n_bytes);
    if (err < 0)
    {
        return err;
    }

    pkt->pos = avio_tell(pb);
    pkt->stream_index = 0;
    err = avio_read(pb, pkt->data, n_bytes);
    if (err < 0)
    {
        av_packet_unref(pkt);
        return err;
    }

    return 0;
}

static int ntsilk_write_header_skp(AVFormatContext *s)
{
    AVIOContext *pb = s->pb;
    avio_write(pb, NTSILK_SKP_HEADER, sizeof(NTSILK_SKP_HEADER));
    return 0;
}

static int ntsilk_write_header_tct(AVFormatContext *s)
{
    AVIOContext *pb = s->pb;
    avio_write(pb, NTSILK_TCT_HEADER, sizeof(NTSILK_TCT_HEADER));
    return 0;
}

static int ntsilk_write_trailer_s16le(AVFormatContext *s)
{
    AVIOContext *pb = s->pb;
    int16_t n = -1;
    avio_write(pb, (const unsigned char *)&n, sizeof(int16_t));
    return 0;
}

static const AVOption ntsilk_demuxer_options[] = {
    {0},
};

static const AVClass ntsilk_demuxer_class = {
    .class_name = "NTSilk Demuxer",
    .item_name = av_default_item_name,
    .option = ntsilk_demuxer_options,
    .version = LIBAVUTIL_VERSION_INT,
};

#if CONFIG_NTSILK_SKP_S16LE_DEMUXER

const FFInputFormat ff_ntsilk_skp_s16le_demuxer = {
    .p.name = "silk_s16le",
    .p.long_name = NULL_IF_CONFIG_SMALL("SILK (Skype)"),
    .p.mime_type = "audio/SILK",
    .p.extensions = "silk",

    .p.priv_class = &ntsilk_demuxer_class,
    .priv_data_size = sizeof(NTSilkDemuxerContext),

    .p.flags = AVFMT_NOTIMESTAMPS | AVFMT_GENERIC_INDEX,
    .flags_internal = FF_OFMT_FLAG_MAX_ONE_OF_EACH | FF_OFMT_FLAG_ONLY_DEFAULT_CODECS,

    .read_probe = ntsilk_probe_skp,
    .read_header = ntsilk_read_header_skp,
    .read_packet = ntsilk_read_packet_s16le,
    // .read_packet       = ff_raw_read_partial_packet,
    // .raw_codec_id      = AV_CODEC_ID_NTSILK_S16LE,
};

#endif /* CONFIG_NTSILK_SKP_S16LE_DEMUXER */

#if CONFIG_NTSILK_TCT_S16LE_DEMUXER

const FFInputFormat ff_ntsilk_tct_s16le_demuxer = {
    .p.name = "ntsilk_s16le",
    .p.long_name = NULL_IF_CONFIG_SMALL("NTSilk (Tencent)"),
    .p.mime_type = "audio/SILK",
    .p.extensions = "ntsilk",

    .p.priv_class = &ntsilk_demuxer_class,
    .priv_data_size = sizeof(NTSilkDemuxerContext),

    .p.flags = AVFMT_NOTIMESTAMPS | AVFMT_GENERIC_INDEX,
    .flags_internal = FF_OFMT_FLAG_MAX_ONE_OF_EACH | FF_OFMT_FLAG_ONLY_DEFAULT_CODECS,

    .read_probe = ntsilk_probe_tct,
    .read_header = ntsilk_read_header_tct,
    .read_packet = ntsilk_read_packet_s16le,
    // .read_packet       = ff_raw_read_partial_packet,
    // .raw_codec_id      = AV_CODEC_ID_NTSILK_S16LE,
};

#endif /* CONFIG_NTSILK_TCT_S16LE_DEMUXER */

#if CONFIG_NTSILK_SKP_S16LE_MUXER

const FFOutputFormat ff_ntsilk_skp_s16le_muxer = {
    .p.name = "silk_s16le",
    .p.long_name = NULL_IF_CONFIG_SMALL("SILK (Skype)"),
    .p.mime_type = "audio/SILK",
    .p.extensions = "silk",

    .p.audio_codec = AV_CODEC_ID_NTSILK_S16LE,
    .p.video_codec = AV_CODEC_ID_NONE,
    .p.subtitle_codec = AV_CODEC_ID_NONE,

    .p.flags = AVFMT_NOTIMESTAMPS | AVFMT_GENERIC_INDEX,
    .flags_internal = FF_OFMT_FLAG_MAX_ONE_OF_EACH | FF_OFMT_FLAG_ONLY_DEFAULT_CODECS,

    .write_header = ntsilk_write_header_skp,
    .write_trailer = ntsilk_write_trailer_s16le,
    .write_packet = ff_raw_write_packet,
};

#endif /* CONFIG_NTSILK_SKP_S16LE_MUXER */

#if CONFIG_NTSILK_TCT_S16LE_MUXER

const FFOutputFormat ff_ntsilk_tct_s16le_muxer = {
    .p.name = "ntsilk_s16le",
    .p.long_name = NULL_IF_CONFIG_SMALL("NTSilk (Tencent)"),
    .p.mime_type = "audio/SILK",
    .p.extensions = "ntsilk",

    .p.audio_codec = AV_CODEC_ID_NTSILK_S16LE,
    .p.video_codec = AV_CODEC_ID_NONE,
    .p.subtitle_codec = AV_CODEC_ID_NONE,

    .p.flags = AVFMT_NOTIMESTAMPS | AVFMT_GENERIC_INDEX,
    .flags_internal = FF_OFMT_FLAG_MAX_ONE_OF_EACH | FF_OFMT_FLAG_ONLY_DEFAULT_CODECS,

    .write_header = ntsilk_write_header_tct,
    .write_packet = ff_raw_write_packet,
};

#endif /* CONFIG_NTSILK_TCT_S16LE_MUXER */