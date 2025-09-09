/*
 * Copyright (c) 2025 Google LLC
 *
 * This source code is subject to the terms of the BSD 3-Clause Clear License,
 * which you can find in the LICENSE file, and the Open Binaural Renderer
 * Patent License 1.0, which you can find in the PATENTS file.
 */

#ifndef OBR_COMMON_CONSTANTS_H_
#define OBR_COMMON_CONSTANTS_H_

#include <cmath>
#include <cstddef>
#include <numbers>

namespace obr {

// Defines memory alignment of audio buffers. Note that not only the first
// element of the `data_` buffer is memory aligned but also the address of the
// first elements of the `ChannelView`s.
const size_t kMemoryAlignmentBytes = 64;

// Minimum Ambisonic order currently supported by obr.
static const int kMinSupportedAmbisonicOrder = 1;

// Maximum Ambisonic order currently supported by obr (limited by the
// available SH-HRIRs provided via binaural_filters.h).
static const int kMaxSupportedAmbisonicOrder = 7;

// Maximum number of input channels supported by obr.
static const size_t kMaxSupportedNumInputChannels = 128;

// Maximum allowed size of internal buffers.
const size_t kMaxSupportedNumFrames = 16384;

// Number of binaural channels.
static const size_t kNumBinauralChannels = 2;

// Number of mono channels.
static const size_t kNumMonoChannels = 1;

// Number of stereo channels.
static const size_t kNumStereoChannels = 2;

// Negative 120dB in amplitude.
static const float kNegative120dbInAmplitude = 0.000001f;

// Tolerated error margins for floating points.
static const float kEpsilonFloat = 1e-6f;

// Inverse square root of two (equivalent to -3dB audio signal attenuation).
static const float kInverseSqrtTwo = 1.0f / std::sqrt(2.0f);

// Square roots.
static const float kSqrtTwo = std::sqrt(2.0f);

// Pi in radians.
static const float kPi = static_cast<float>(std::numbers::pi_v<float>);
// Half pi in radians.
static const float kHalfPi =
    static_cast<float>(std::numbers::pi_v<float> / 2.0);
// Two pi in radians.
static const float kTwoPi = static_cast<float>(2.0 * std::numbers::pi_v<float>);

// Defines conversion factor from degrees to radians.
static const float kRadiansFromDegrees =
    static_cast<float>(std::numbers::pi_v<float> / 180.0);

// Defines conversion factor from radians to degrees.
static const float kDegreesFromRadians =
    static_cast<float>(180.0 / std::numbers::pi_v<float>);

}  // namespace obr

#endif  // OBR_COMMON_CONSTANTS_H_
