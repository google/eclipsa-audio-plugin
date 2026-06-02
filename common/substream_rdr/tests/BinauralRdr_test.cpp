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

#include <cmath>
#include <cstdio>

#include "TestHelper.h"
#include "substream_rdr/bed2bed_rdr/BedToBedRdr.h"

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

// Renders the same 5.1 input through both the stereo downmix (BedToBedRdr) and
// binaural (BinauralRdr) renderers. Prints L/R sample values at four time
// points so results can be compared directly between platforms. Both channels
// must diverge past the HRIR ramp-up region to confirm convolution is active.
TEST(test_binaural_rendering, stereo_and_binaural_outputs_are_unique) {
  const int kNumSamples = 512;
  const int kSampleRate = 48000;

  auto binauralRdr = BinauralRdr::createBinauralRdr(Speakers::k5Point1,
                                                    kNumSamples, kSampleRate);
  ASSERT_NE(binauralRdr, nullptr);

  auto stereoRdr =
      BedToBedRdr::createBedToBedRdr(Speakers::k5Point1, Speakers::kStereo);
  ASSERT_NE(stereoRdr, nullptr);

  FBuffer inputBuff(Speakers::k5Point1.getNumChannels(), kNumSamples);
  populateInput(inputBuff);

  FBuffer binauralOut(Speakers::kBinaural.getNumChannels(), kNumSamples);
  FBuffer stereoOut(Speakers::kStereo.getNumChannels(), kNumSamples);
  binauralOut.clear();
  stereoOut.clear();

  binauralRdr->render(inputBuff, binauralOut);
  stereoRdr->render(inputBuff, stereoOut);

  for (int s : {0, kNumSamples / 4, kNumSamples / 2, kNumSamples - 1}) {
    std::printf(
        "  sample[%3d]  stereo  [L=%9.6f  R=%9.6f]"
        "  binaural [L=%9.6f  R=%9.6f]\n",
        s, stereoOut.getSample(0, s), stereoOut.getSample(1, s),
        binauralOut.getSample(0, s), binauralOut.getSample(1, s));
  }

  const int kLast = kNumSamples - 1;
  const float kMinDiff = 1e-4f;
  EXPECT_GT(
      std::abs(binauralOut.getSample(0, kLast) - stereoOut.getSample(0, kLast)),
      kMinDiff)
      << "Binaural L matches stereo L at sample " << kLast
      << " -- HRIR convolution may not be active";
  EXPECT_GT(
      std::abs(binauralOut.getSample(1, kLast) - stereoOut.getSample(1, kLast)),
      kMinDiff)
      << "Binaural R matches stereo R at sample " << kLast
      << " -- HRIR convolution may not be active";
}

// Parameterized over standard DAW buffer sizes. Tests are run for each size
// to catch buffer-size-dependent failures in the OBR convolution engine.
class BinauralRdrBufferSizeTest : public ::testing::TestWithParam<int> {
 protected:
  static constexpr int kSampleRate = 48000;
};

// Silence indicates AddAudioElement failed silently at runtime.
TEST_P(BinauralRdrBufferSizeTest, output_is_not_silent) {
  const int kNumSamples = GetParam();

  auto rdr = BinauralRdr::createBinauralRdr(Speakers::k5Point1, kNumSamples,
                                            kSampleRate);
  ASSERT_NE(rdr, nullptr);

  FBuffer inputBuff(Speakers::k5Point1.getNumChannels(), kNumSamples);
  populateInput(inputBuff);

  FBuffer outputBuff(Speakers::kBinaural.getNumChannels(), kNumSamples);
  outputBuff.clear();
  rdr->render(inputBuff, outputBuff);

  float maxAbsVal = 0.f;
  for (int ch = 0; ch < outputBuff.getNumChannels(); ++ch)
    for (int s = 0; s < kNumSamples; ++s)
      maxAbsVal = std::max(maxAbsVal, std::abs(outputBuff.getSample(ch, s)));

  EXPECT_GT(maxAbsVal, 1e-6f)
      << "Binaural output is silent at buffer size " << kNumSamples << ". "
      << "AddAudioElement may have failed on this platform -- check the "
      << "EclipsaRenderer log for 'AddAudioElement failed' errors.";
}

// HRIR convolution must produce output distinct from a linear stereo downmix.
TEST_P(BinauralRdrBufferSizeTest, output_differs_from_stereo_downmix) {
  const int kNumSamples = GetParam();

  auto binauralRdr = BinauralRdr::createBinauralRdr(Speakers::k5Point1,
                                                    kNumSamples, kSampleRate);
  ASSERT_NE(binauralRdr, nullptr);

  auto stereoRdr =
      BedToBedRdr::createBedToBedRdr(Speakers::k5Point1, Speakers::kStereo);
  ASSERT_NE(stereoRdr, nullptr);

  FBuffer inputBuff(Speakers::k5Point1.getNumChannels(), kNumSamples);
  populateInput(inputBuff);

  FBuffer binauralOut(Speakers::kBinaural.getNumChannels(), kNumSamples);
  FBuffer stereoOut(Speakers::kStereo.getNumChannels(), kNumSamples);
  binauralOut.clear();
  stereoOut.clear();

  binauralRdr->render(inputBuff, binauralOut);
  stereoRdr->render(inputBuff, stereoOut);

  float maxDiff = 0.f;
  for (int ch = 0; ch < Speakers::kBinaural.getNumChannels(); ++ch)
    for (int s = 0; s < kNumSamples; ++s)
      maxDiff = std::max(maxDiff, std::abs(binauralOut.getSample(ch, s) -
                                           stereoOut.getSample(ch, s)));

  const int kLast = kNumSamples - 1;
  EXPECT_GT(maxDiff, 1e-4f)
      << "Binaural and stereo outputs are identical at buffer size "
      << kNumSamples << ", suggesting HRIR convolution is not being applied.\n"
      << "  Binaural last sample [L=" << binauralOut.getSample(0, kLast)
      << " R=" << binauralOut.getSample(1, kLast) << "]\n"
      << "  Stereo   last sample [L=" << stereoOut.getSample(0, kLast)
      << " R=" << stereoOut.getSample(1, kLast) << "]";
}

INSTANTIATE_TEST_SUITE_P(StandardDawBufferSizes, BinauralRdrBufferSizeTest,
                         ::testing::Values(64, 128, 256, 512, 1024, 2048),
                         [](const ::testing::TestParamInfo<int>& info) {
                           return std::to_string(info.param) + "samples";
                         });