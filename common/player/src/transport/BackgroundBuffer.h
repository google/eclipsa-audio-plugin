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

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdlib>
#include <memory>

#include "PbRingBuffer.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class BackgroundBuffer {
 public:
  BackgroundBuffer(const unsigned paddingSeconds, IAMFFileReader& decoder);
  ~BackgroundBuffer();

  bool isReady();
  size_t availableSamples() const;
  size_t readSamples(juce::AudioBuffer<float>& out, const unsigned startSample,
                     const unsigned numSamples);
  void seek(const size_t newFrameIdx);

 private:
  void notifyTask();
  void decodeTask();

  IAMFFileReader& decoder_;
  size_t padSamples_, absSamplePos_;
  juce::SpinLock bufferLock_;
  std::unique_ptr<PbRingBuffer> pbuffer_;
  // Decoding thread and control
  std::atomic_bool stop_, eof_;
  std::condition_variable cv_;
  std::mutex cvm_;
  std::thread decodeThread_;
};