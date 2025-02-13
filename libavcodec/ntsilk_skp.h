#ifndef AVCODEC_NTSILK_SKP_H
#define AVCODEC_NTSILK_SKP_H

#include <stddef.h>
#include <stdint.h>

extern const size_t COMMON_MAX_BYTES_PER_FRAME;
extern const size_t COMMON_FRAME_LENGTH_MS;
extern const size_t COMMON_MAX_API_FS_KHZ;
extern const size_t COMMON_MAX_INPUT_SIZE;
extern const size_t COMMON_MAX_INPUT_SAMPLES;

extern const size_t ENCODER_MAX_INPUT_FRAMES;
extern const int16_t ENCODER_MAX_PAYLOAD_BYTES;
extern const int16_t ENCODER_MAX_OUT_BYTES;

extern const size_t DECODER_MAX_INPUT_FRAMES;

int ff_ntsilk_error_to_averror(int err);

#endif /* AVCODEC_NTSILK_SKP_H */