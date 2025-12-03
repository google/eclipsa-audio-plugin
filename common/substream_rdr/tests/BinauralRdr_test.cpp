// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "substream_rdr/bin_rdr/BinauralRdr.h"

#include <gtest/gtest.h>

using namespace Speakers;

const std::vector<AudioElementSpeakerLayout> kInputLayouts = {
    Speakers::kMono,          Speakers::kStereo,
    Speakers::k3Point1Point2, Speakers::k5Point1,
    Speakers::k5Point1Point2, Speakers::k7Point1,
    Speakers::k7Point1Point2, Speakers::k7Point1Point4,
    Speakers::kHOA1,          Speakers::kHOA2,
    Speakers::kHOA3,          Speakers::kHOA4,
    Speakers::kBinaural};

// OBR only supports binaural rendering for 5.1, 7.1.4, 3OA, and 7OA layouts
// currently.
TEST(test_binaural_rendering, construct_renderer) {
  for (const auto& layout : kInputLayouts) {
    auto renderer = BinauralRdr::createBinauralRdr(layout, 32, 48000);
    // Current valid layouts.
    EXPECT_NE(renderer, nullptr);
  }
}