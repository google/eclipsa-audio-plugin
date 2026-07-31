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

#include "../src/FileExport.h"

#include <gtest/gtest.h>
#include <juce_data_structures/juce_data_structures.h>

TEST(test_file_export, default_state) {
  const FileExport fileExport;

  // Per-element WAV export is opt-in: a fresh export screen (default
  // FileExport state) must not have it enabled.
  EXPECT_FALSE(fileExport.getExportAudioElements());
}

TEST(test_file_export, from_tree_missing_export_audio_elements_defaults_false) {
  // A ValueTree that never had kExportAudioElements set (e.g. an older
  // saved session) must not implicitly re-enable per-element WAV export.
  const juce::ValueTree tree{FileExport::kTreeType, {}};

  const FileExport fileExport = FileExport::fromTree(tree);

  EXPECT_FALSE(fileExport.getExportAudioElements());
}

// Regression test: startSampleIdx_/endSampleIdx_ were widened from `long`
// (32-bit on Windows LLP64) to juce::int64 so a sample index deep into a
// long session (e.g. past ~12.4 hours at 48kHz) doesn't silently
// truncate/wrap. Confirm a value past INT32_MAX survives a
// toValueTree()/fromTree() round-trip -- the same path FileExportRepository
// uses internally -- fully intact.
TEST(test_file_export, round_trip_preserves_sample_indices_past_int32_max) {
  constexpr juce::int64 kBeyondInt32Max = 3'000'000'000LL;  // > 2^31 - 1

  FileExport config;
  config.setStartSampleIdx(kBeyondInt32Max);
  config.setEndSampleIdx(kBeyondInt32Max + 1000);

  const juce::ValueTree tree = config.toValueTree();
  const FileExport roundTripped = FileExport::fromTree(tree);

  EXPECT_EQ(roundTripped.getStartSampleIdx(), kBeyondInt32Max);
  EXPECT_EQ(roundTripped.getEndSampleIdx(), kBeyondInt32Max + 1000);
}
