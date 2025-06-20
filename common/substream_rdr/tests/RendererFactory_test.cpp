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

#include "substream_rdr/rdr_factory/RendererFactory.h"

#include <gtest/gtest.h>

#include "ear/ear.hpp"
#include "substream_rdr/rdr_factory/Renderer.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

const int kNumSamples = 1;

TEST(test_substream_rdr, create_channelbased) {
  Speakers::AudioElementSpeakerLayout inputLayout = Speakers::kMono;
  Speakers::AudioElementSpeakerLayout outputLayout = Speakers::k5Point1;

  ASSERT_TRUE(createRenderer(inputLayout, outputLayout) != nullptr);
}

TEST(test_substream_rdr, create_scenebased) {
  Speakers::AudioElementSpeakerLayout inputLayout = Speakers::kHOA1;
  Speakers::AudioElementSpeakerLayout outputLayout = Speakers::k5Point1;

  ASSERT_TRUE(createRenderer(inputLayout, outputLayout) != nullptr);
}
