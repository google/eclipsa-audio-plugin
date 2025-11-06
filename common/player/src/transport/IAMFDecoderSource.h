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

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_core/juce_core.h>

#include <functional>
#include <memory>

#include "player/src/transport/BackgroundBuffer.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

/**
 * @brief Audio source wrapper for the IAMF decoder. Uses an auto-filling
 * buffer to maintain x-second padding. Thread-safe
 * for concurrent access from UI and audio threads.
 */
class IAMFDecoderSource : public juce::AudioSource {
 public:
  explicit IAMFDecoderSource(std::unique_ptr<IAMFFileReader> reader);

  void play();
  void pause();
  void stop();
  bool seek(size_t frameIndex);

  // Changes the decode layout by recreating the decoder with new
  // settings. This will stop playback, release resources, recreate the decoder,
  // and prepare it again.
  void setLayout(const Speakers::AudioElementSpeakerLayout layout);

  void setOnFinishedCallback(std::function<void()> callback) {
    onFinished_ = callback;
  }

  void prepareToPlay(int, double) override;

  void releaseResources() override;

  void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override;

  bool isReady() const {
    const juce::SpinLock::ScopedLockType lock(stateLock_);
    return buffer_->isReady();
  }

  bool isPlaying() const {
    const juce::SpinLock::ScopedLockType lock(stateLock_);
    return isPlaying_;
  }

  IAMFFileReader::StreamData getStreamData() const {
    IAMFFileReader::StreamData data = streamData_;
    data.currentFrameIdx = frameCount_;
    return data;
  }

 private:
  void recreateDecoder();

  static constexpr unsigned kPadSecs_ = 5;
  std::unique_ptr<IAMFFileReader> decoder_;
  IAMFFileReader::StreamData streamData_;
  std::unique_ptr<BackgroundBuffer> buffer_;
  size_t sampleCount_ = 0, frameCount_ = 0;
  bool isPlaying_ = false;
  bool finished_ = false;
  std::function<void()> onFinished_;
  mutable juce::SpinLock stateLock_;
};