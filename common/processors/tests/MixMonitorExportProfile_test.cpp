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

#include <gtest/gtest.h>

#include "data_structures/src/SpeakerMonitorData.h"
#include "processors/mix_monitoring/MixMonitorProcessor.h"

namespace {
class MixMonitorExportProfileTest : public ::testing::Test {
 protected:
  MixMonitorExportProfileTest()
      : repo_(juce::ValueTree{"test"}), proc_(repo_, data_) {}

  // Simulates a measurement in progress.
  void seedStats() {
    data_.resetStats = false;
    MeasureEBU128::LoudnessStats stats{};
    stats.loudnessIntegrated = kIntegrated;
    data_.loudnessEBU128.update(stats);
  }

  float integrated() const {
    MeasureEBU128::LoudnessStats stats{};
    data_.loudnessEBU128.read(stats);
    return stats.loudnessIntegrated;
  }

  static constexpr float kIntegrated = -12.5f;
  SpeakerMonitorData data_;
  RoomSetupRepository repo_;
  MixMonitorProcessor proc_;
};
}  // namespace

// Switching only the export profile keeps the running loudness measurement.
TEST_F(MixMonitorExportProfileTest, profile_change_keeps_stats) {
  seedStats();
  RoomSetup setup = repo_.get();
  ASSERT_EQ(setup.getExportProfile(), ExportProfile::kYouTube);
  setup.setExportProfile(ExportProfile::kAdvanced);
  repo_.update(setup);

  EXPECT_FALSE(data_.resetStats.load());
  EXPECT_FLOAT_EQ(integrated(), kIntegrated);
}

// Changing the speaker layout still resets the loudness measurement.
TEST_F(MixMonitorExportProfileTest, layout_change_resets_stats) {
  seedStats();
  RoomSetup setup = repo_.get();
  ASSERT_NE(setup.getSpeakerLayout(), speakerLayoutConfigurationOptions[2]);
  setup.setSpeakerLayout(speakerLayoutConfigurationOptions[2]);
  repo_.update(setup);

  EXPECT_TRUE(data_.resetStats.load());
  EXPECT_FLOAT_EQ(integrated(), 0.0f);
}
