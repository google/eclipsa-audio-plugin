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

#include <chrono>
#include <iostream>
#include <memory>

#include "IAMFBufferedReader.h"
#include "data_structures/src/FilePlayback.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

namespace PlaybackTasks {

enum ResultType {
  kPreempted,
  kLoadFinished,
  kLoadFailed,
  kLayoutFinished,
  kSeekPlayingFinished,
  kSeekPausedFinished,
};

struct Result {
  ResultType status;
  std::unique_ptr<IamfBufferedReader> buffer;
  size_t seekedFrameIdx;
  IAMFFileReader::StreamData metadata;
};

static bool isIamfFile(const std::filesystem::path& file) {
  if (file.empty() || file.extension() != ".iamf" ||
      !std::filesystem::exists(file)) {
    return false;
  }
  return true;
}

static Result loadTask(const std::filesystem::path file,
                       const Speakers::AudioElementSpeakerLayout layout,
                       std::atomic_bool& stop) {
  if (!isIamfFile(file)) {
    return Result{kLoadFailed};
  }

  const auto t0 = std::chrono::steady_clock::now();

  auto reader = IAMFFileReader::createIamfReader(
      file, IAMFFileReader::kDefaultReaderSettings, stop);
  if (!reader) {
    return stop ? Result{kPreempted} : Result{kLoadFailed};
  }
  const auto t1 = std::chrono::steady_clock::now();
  std::cout << "[PlaybackTasks::loadTask] createIamfReader: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
            << "ms\n";

  reader->indexFile(stop);
  const auto t2 = std::chrono::steady_clock::now();
  std::cout << "[PlaybackTasks::loadTask] indexFile: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
            << "ms\n";

  reader->resetLayout(layout);
  const auto t3 = std::chrono::steady_clock::now();
  std::cout << "[PlaybackTasks::loadTask] resetLayout: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count()
            << "ms\n";

  const IAMFFileReader::StreamData kData = reader->getStreamData();

  auto buffer = std::make_unique<IamfBufferedReader>(std::move(reader), 2);
  buffer->waitUntilReady();
  const auto t4 = std::chrono::steady_clock::now();
  std::cout << "[PlaybackTasks::loadTask] IamfBufferedReader ready: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count()
            << "ms\n";

  std::cout << "[PlaybackTasks::loadTask] total: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t0).count()
            << "ms\n";

  const ResultType kRt = stop ? kPreempted : kLoadFinished;
  return Result{kRt, std::move(buffer), 0, kData};
}

static Result layoutTask(const std::filesystem::path file,
                         const Speakers::AudioElementSpeakerLayout layout,
                         size_t numFrames, std::atomic_bool& stop) {
  if (!isIamfFile(file)) {
    return Result{kLoadFailed};
  }

  const auto t0 = std::chrono::steady_clock::now();

  auto reader = IAMFFileReader::createIamfReader(
      file, IAMFFileReader::kDefaultReaderSettings, stop);
  if (!reader) {
    return stop ? Result{kPreempted} : Result{kLoadFailed};
  }
  const auto t1 = std::chrono::steady_clock::now();
  std::cout << "[PlaybackTasks::layoutTask] createIamfReader: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
            << "ms\n";

  if (numFrames == 0) {
    numFrames = reader->indexFile(stop);
  } else {
    reader->setNumFrames(numFrames);
  }
  if (stop) {
    return Result{kPreempted};
  }
  const auto t2 = std::chrono::steady_clock::now();
  std::cout << "[PlaybackTasks::layoutTask] indexFile/setNumFrames: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
            << "ms\n";

  reader->resetLayout(layout);
  const auto t3 = std::chrono::steady_clock::now();
  std::cout << "[PlaybackTasks::layoutTask] resetLayout: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2).count()
            << "ms\n";

  const IAMFFileReader::StreamData kData = reader->getStreamData();

  auto buffer = std::make_unique<IamfBufferedReader>(std::move(reader), 2);
  buffer->waitUntilReady();
  const auto t4 = std::chrono::steady_clock::now();
  std::cout << "[PlaybackTasks::layoutTask] IamfBufferedReader ready: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count()
            << "ms\n";

  std::cout << "[PlaybackTasks::layoutTask] total: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t0).count()
            << "ms\n";

  return Result{kLayoutFinished, std::move(buffer), 0, kData};
}

static Result seekTask(const std::filesystem::path file,
                       const Speakers::AudioElementSpeakerLayout layout,
                       const float pos,
                       const FilePlayback::ProcessorState prevState,
                       std::atomic_bool& stop) {
  Result res = loadTask(file, layout, stop);
  if (res.status != kLoadFinished) {
    return res;
  }

  const size_t kSeekedIdx = pos * res.metadata.numFrames;
  res.buffer->seek(kSeekedIdx, stop);
  if (stop) {
    return Result{kPreempted};
  }

  res.buffer->waitUntilReady();

  const ResultType kRt = prevState == FilePlayback::ProcessorState::kPlaying
                             ? kSeekPlayingFinished
                             : kSeekPausedFinished;
  return Result{kRt, std::move(res.buffer), kSeekedIdx, res.metadata};
}

static Result seekTask(const float pos,
                       const FilePlayback::ProcessorState prevState,
                       const size_t numFrames, IamfBufferedReader* buffer,
                       std::atomic_bool& stop) {
  if (!buffer) {
    return Result{kLoadFailed};
  }

  const auto t0 = std::chrono::steady_clock::now();

  const size_t kSeekedIdx = pos * numFrames;
  buffer->seek(kSeekedIdx, stop);
  if (stop) {
    return Result{kPreempted};
  }
  const auto t1 = std::chrono::steady_clock::now();
  std::cout << "[PlaybackTasks::seekTask] seek: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()
            << "ms\n";

  buffer->waitUntilReady();
  const auto t2 = std::chrono::steady_clock::now();
  std::cout << "[PlaybackTasks::seekTask] waitUntilReady: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count()
            << "ms\n";

  std::cout << "[PlaybackTasks::seekTask] total: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t0).count()
            << "ms\n";

  const ResultType kRt = prevState == FilePlayback::ProcessorState::kPlaying
                             ? kSeekPlayingFinished
                             : kSeekPausedFinished;
  return Result{kRt,
                std::unique_ptr<IamfBufferedReader>(buffer),
                kSeekedIdx,
                {.valid = false}};
}
}  // namespace PlaybackTasks
