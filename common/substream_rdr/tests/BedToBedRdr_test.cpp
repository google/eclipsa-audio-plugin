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

#include "substream_rdr/bed2bed_rdr/BedToBedRdr.h"

#include <gtest/gtest.h>

#include "TestHelper.h"
#include "ear/ear.hpp"
#include "substream_rdr/rdr_factory/RendererFactory.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

using Speakers::AudioElementSpeakerLayout;

const int kNumSamples = 1;

const std::vector<AudioElementSpeakerLayout> inputLayouts = {
    Speakers::kMono,    Speakers::kStereo,        Speakers::k3Point1Point2,
    Speakers::k5Point1, Speakers::k5Point1Point2, Speakers::k5Point1Point4,
    Speakers::k7Point1, Speakers::k7Point1Point2, Speakers::k7Point1Point4,
    Speakers::kBinaural};

const std::vector<AudioElementSpeakerLayout> extLayouts = {
    Speakers::kExplLFE,
    Speakers::kExpl5Point1Point4Surround,
    Speakers::kExpl7Point1Point4SideSurround,
    Speakers::kExpl7Point1Point4RearSurround,
    Speakers::kExpl7Point1Point4TopFront,
    Speakers::kExpl7Point1Point4TopBack,
    Speakers::kExpl7Point1Point4Top,
    Speakers::kExpl7Point1Point4Front,
    Speakers::kExpl9Point1Point6,
    Speakers::kExpl9Point1Point6Front,
    Speakers::kExpl9Point1Point6Side,
    Speakers::kExpl9Point1Point6TopSide,
    Speakers::kExpl9Point1Point6Top};

const std::vector<AudioElementSpeakerLayout> playbackLayouts = {
    Speakers::kStereo,        Speakers::k5Point1,
    Speakers::k5Point1Point2, Speakers::k7Point1,
    Speakers::k7Point1Point4, Speakers::k3Point1Point2,
    Speakers::k7Point1Point2, Speakers::kExpl9Point1Point6,
};

// A DS renderer must be constructible for valid input/playback layout
// combinations.
TEST(test_b2b_rdr, construct_rdr) {
  auto rdr_type = AudioElement::AudioElement_t::AUDIO_ELEMENT_CHANNEL_BASED;
  Speakers::FBuffer inputBuff;
  for (const auto in : inputLayouts) {
    for (const auto out : playbackLayouts) {
      // If the input layout is the same as the output layout a passthrough
      // renderer is constructed instead.
      if (in == out) {
        continue;
      }
      ASSERT_TRUE(BedToBedRdr::createBedToBedRdr(in, out) != nullptr);
    }
  }
}

// Iterate over possible layouts, create appropriate renderer and render to
// output buffer.
TEST(test_b2b_rdr, rdr) {
  Speakers::FBuffer inBuff, outBuff;
  for (const auto in : inputLayouts) {
    inBuff = Speakers::FBuffer(in.getNumChannels(), kNumSamples);
    populateInput(inBuff);
    for (const auto out : playbackLayouts) {
      auto rdr = BedToBedRdr::createBedToBedRdr(in, out);

      if (in != out) {
        EXPECT_TRUE(rdr != nullptr);

        outBuff = Speakers::FBuffer(out.getNumChannels(), kNumSamples);
        EXPECT_NO_FATAL_FAILURE(rdr->render(inBuff, outBuff));
      }
    }
  }
}

// Confirm a b2b renderer can be constructed for all extended layouts to all
// base layouts.
TEST(test_b2b_rdr, construct_rdr_ext) {
  for (const auto layout : extLayouts) {
    for (const auto playbackLayout : playbackLayouts) {
      if (layout != playbackLayout &&
          layout.getExplBaseLayout() != playbackLayout) {
        EXPECT_TRUE(BedToBedRdr::createBedToBedRdr(layout, playbackLayout) !=
                    nullptr)
            << "Failed to create a renderer for " << layout.toString()
            << " (underlying layout of "
            << layout.getExplBaseLayout().toString() << ")"
            << " to " << playbackLayout.toString();
      }
    }
  }
}

TEST(test_b2b_rdr, rdr_ext) {
  for (const auto inputLayout : extLayouts) {
    Speakers::FBuffer inBuff =
        Speakers::FBuffer(inputLayout.getNumChannels(), kNumSamples);
    for (const auto playbackLayout : playbackLayouts) {
      auto rdr = BedToBedRdr::createBedToBedRdr(inputLayout, playbackLayout);
      if (inputLayout != playbackLayout &&
          inputLayout.getExplBaseLayout() != playbackLayout) {
        EXPECT_TRUE(BedToBedRdr::createBedToBedRdr(inputLayout,
                                                   playbackLayout) != nullptr)
            << "Failed to create a renderer for " << inputLayout.toString()
            << " (underlying layout of "
            << inputLayout.getExplBaseLayout().toString() << ")"
            << " to " << playbackLayout.toString();

        Speakers::FBuffer outBuff(playbackLayout.getNumChannels(), kNumSamples);
        EXPECT_NO_FATAL_FAILURE(rdr->render(inBuff, outBuff));
      }
    }
  }
}