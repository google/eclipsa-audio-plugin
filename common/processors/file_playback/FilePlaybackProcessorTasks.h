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

#pragma once
#include <cstdint>
#include <string>

#include "data_structures/src/FilePlayback.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

namespace FilePlaybackProcessorEvents {
enum TaskType {
  kNone,
  kLoad,
  kLayout,
  kSeek,
  kPlay,
  kPause,
  kStop,
};

enum TaskResult {
  kLoadFinished,
  kLoadFailed,
  kLayoutPlayingFinished,
  kLayoutPausedFinished,
  kSeekPlayingFinished,
  kSeekPausedFinished,
};

constexpr int getEventPriority(const TaskType event) {
  switch (event) {
    case TaskType::kLoad:
      return 3;
    case TaskType::kLayout:
      return 2;
    case TaskType::kSeek:
      return 1;
      default:
      break;
  }
  return 0;
}

inline const char* taskTypeToString(const TaskType type) {
  switch (type) {
    case TaskType::kNone:
      return "kNone";
    case TaskType::kLoad:
      return "kLoad";
    case TaskType::kLayout:
      return "kLayout";
    case TaskType::kSeek:
      return "kSeek";
    case TaskType::kPlay:
      return "kPlay";
    case TaskType::kPause:
      return "kPause";
    case TaskType::kStop:
      return "kStop";
    default:
      return "Unknown";
  }
}

inline const char* taskResultToString(const TaskResult result) {
  switch (result) {
    case TaskResult::kLoadFinished:
      return "kLoadFinished";
    case TaskResult::kLoadFailed:
      return "kLoadFailed";
    case TaskResult::kLayoutPlayingFinished:
      return "kLayoutPlayingFinished";
    case TaskResult::kLayoutPausedFinished:
      return "kLayoutPausedFinished";
    case TaskResult::kSeekPlayingFinished:
      return "kSeekPlayingFinished";
    case TaskResult::kSeekPausedFinished:
      return "kSeekPausedFinished";
    default:
      return "Unknown";
  }
}

// Task data structure for worker operations
struct TaskData {
  FilePlayback::ProcessorState prevState;
  Speakers::AudioElementSpeakerLayout layout;
  float val = 0.0f;
  uint64_t id = 0;
  std::string filePath;
};

}  // namespace FilePlaybackProcessorEvents
