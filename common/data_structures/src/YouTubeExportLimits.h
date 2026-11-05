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

#include <cstdint>

#include "FileExport.h"

// Upload limits YouTube enforces
// (https://support.google.com/youtube/answer/71673).
namespace YouTubeExportLimits {
inline constexpr double kMaxDurationSec = 43200.0;  // 12 hours.
// 256 GB read as decimal gigabytes, the smaller of the two readings.
inline constexpr std::int64_t kMaxBytes = 256'000'000'000;

struct Limits {
  double maxDurationSec = kMaxDurationSec;
  std::int64_t maxBytes = kMaxBytes;
};

// Duration is checked before size. A negative input is unknown and passes.
inline ExportError check(const double durationSec,
                         const std::int64_t estimatedBytes,
                         const Limits& limits = {}) {
  if (durationSec > limits.maxDurationSec) {
    return kYouTubeDurationExceeded;
  }
  if (estimatedBytes > limits.maxBytes) {
    return kYouTubeSizeExceeded;
  }
  return kNoError;
}
}  // namespace YouTubeExportLimits
