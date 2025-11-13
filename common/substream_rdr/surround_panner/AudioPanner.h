/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "ear/common_types.hpp"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class AudioPanner {
 public:
  AudioPanner(const Speakers::AudioElementSpeakerLayout pannedLayout,
              int samplesPerBlock, int sampleRate)
      : kPannedLayout_(pannedLayout),
        kSamplesPerBlock_(samplesPerBlock),
        kSampleRate_(sampleRate) {}

  virtual ~AudioPanner() = default;

  void setPosition(const float x, const float y, const float z) {
    // Convert to polar coordinates
    // The polar coordinates follow the convention described on page 4 of the
    // ITU-R BS.2051-3 documentation

    currPos_ = convertCartToPolar(x, y, z);
    positionUpdated();
  }

  // this function converts the cartesian coordinates to polar coordinates
  // The polar coordinates follow the convention descrbied on page 4 of the
  // ITU-R BS.2051-3 documentation
  ear::PolarPosition convertCartToPolar(const float x, const float y,
                                        const float z) const {
    const float magnitude = 50.f;

    // normalize each coordinate w.r.t to the maximum distance along each axis
    // clamps the value to be between -1 and 1
    const float normX = x / magnitude;
    const float normY = y / magnitude;
    const float normZ = z / magnitude;

    const float radius = std::sqrt(normX * normX + normY * normY);

    // tolerance for floating point comparison
    const float epsilon = 1e-6f;

    // boilerplate code to handle edge cases
    if (radius < epsilon && std::abs(normZ) < epsilon) {
      return {0, 0.f, 0.f};
    } else if (radius < epsilon && std::abs(normZ) >= epsilon) {
      return {0, normZ > 0.f ? 90.f : -90.f, 0.f};
    }

    // calculate azimuth
    // azimuth is 0 degrees along the +Y axis and increases CCW
    // convert to degrees
    float azimuth = -1.f * std::atan2(normX, normY) * 180.f / M_PI;

    // ensure azimuth is in the range -180 to 180
    if (azimuth > 180.f) {
      azimuth -= 360.f;
    } else if (azimuth <= -180.f) {
      azimuth += 360.f;
    }

    // calculate elevation
    // elevation is measured from the XY plane
    // convert to degrees
    float elevation = std::atan(normZ / radius) * 180.f / M_PI;

    return {azimuth, elevation, radius};
  }

  ear::PolarPosition getPosition() const { return currPos_; }

  virtual void process(juce::AudioBuffer<float>& inputBuffer,
                       juce::AudioBuffer<float>& outputBuffer) = 0;

 protected:
  virtual void positionUpdated() = 0;

  ear::PolarPosition currPos_ = {0.0f, 0.0f, 0.0f};
  const Speakers::AudioElementSpeakerLayout kPannedLayout_;
  int kSamplesPerBlock_, kSampleRate_;
};