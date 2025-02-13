#include <SKP_Silk_errors.h>

#include "ntsilk_skp.h"
#include "libavutil/error.h"

// Max avctx->bit_rate: 320 kbps, means 320 Kb data in 1s
// Max FRAME_LENGTH_MS: 20ms    , means 50 frames   in 1s
// => 320/50 = 6.4 Kb = 6400 b = 800 Bytes in one frame
const size_t COMMON_MAX_BYTES_PER_FRAME = 800; // Equals peak bitrate of 320 kbps
const size_t COMMON_FRAME_LENGTH_MS     = 20;
const size_t COMMON_MAX_API_FS_KHZ      = 48;
// Bake these values for compatibility with gcc < 8.1
const size_t COMMON_MAX_INPUT_SIZE      = 960; // COMMON_FRAME_LENGTH_MS * COMMON_MAX_API_FS_KHZ
const size_t COMMON_MAX_INPUT_SAMPLES   = 960; // COMMON_FRAME_LENGTH_MS * COMMON_MAX_API_FS_KHZ

const size_t ENCODER_MAX_INPUT_FRAMES   = 1;
const int16_t ENCODER_MAX_PAYLOAD_BYTES = 800; // COMMON_MAX_BYTES_PER_FRAME * ENCODER_MAX_INPUT_FRAMES
const int16_t ENCODER_MAX_OUT_BYTES     = 802; // ENCODER_MAX_PAYLOAD_BYTES  2 // sizeof(int16_t)

const size_t DECODER_MAX_INPUT_FRAMES   = 5;

int ff_ntsilk_error_to_averror(int err)
{
    switch (err) {
    default:
        return AVERROR(EINVAL);
    }
}