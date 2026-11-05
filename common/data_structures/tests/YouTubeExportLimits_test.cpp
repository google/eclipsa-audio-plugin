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

#include "../src/YouTubeExportLimits.h"

#include <gtest/gtest.h>

#include <cmath>

using YouTubeExportLimits::check;
using YouTubeExportLimits::kMaxBytes;
using YouTubeExportLimits::kMaxDurationSec;

// Exactly 12 hours and exactly 256,000,000,000 bytes are allowed.
TEST(YouTubeExportLimits, limits_are_inclusive) {
  EXPECT_EQ(kMaxDurationSec, 43200.0);
  EXPECT_EQ(kMaxBytes, 256'000'000'000);
  EXPECT_EQ(check(43200.0, 256'000'000'000), kNoError);
}

// Just over 12 hours is a duration error, even when the size is also over.
TEST(YouTubeExportLimits, duration_over_limit) {
  const double kJustOver = std::nextafter(kMaxDurationSec, 1e9);
  EXPECT_EQ(check(kJustOver, 0), kYouTubeDurationExceeded);
  EXPECT_EQ(check(kJustOver, kMaxBytes + 1), kYouTubeDurationExceeded);
}

// One byte over the limit is a size error.
TEST(YouTubeExportLimits, size_over_limit) {
  EXPECT_EQ(check(kMaxDurationSec, kMaxBytes + 1), kYouTubeSizeExceeded);
}

// Unknown (negative) inputs pass.
TEST(YouTubeExportLimits, unknown_inputs_pass) {
  EXPECT_EQ(check(-1.0, -1), kNoError);
}

// Injected limits replace the defaults.
TEST(YouTubeExportLimits, injected_limits) {
  const YouTubeExportLimits::Limits kLimits{.maxDurationSec = 1.0,
                                            .maxBytes = 10};
  EXPECT_EQ(check(1.5, 0, kLimits), kYouTubeDurationExceeded);
  EXPECT_EQ(check(1.0, 11, kLimits), kYouTubeSizeExceeded);
  EXPECT_EQ(check(1.0, 10, kLimits), kNoError);
}
