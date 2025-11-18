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

#include "IAMFDecoderSource.h"

#include "logger/logger.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

IAMFDecoderSource::IAMFDecoderSource(std::unique_ptr<IAMFFileReader> reader)
    : decoder_(std::move(reader)), isPlaying_(false) {
  streamData_ = decoder_->getStreamData();
}

void IAMFDecoderSource::play() {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  isPlaying_ = true;
}

void IAMFDecoderSource::pause() {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  isPlaying_ = false;
}

void IAMFDecoderSource::stop() {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  isPlaying_ = false;
  finished_ = false;
  if (buffer_) {
    buffer_->seek(0);
    sampleCount_ = frameCount_ = 0;
  }
}

bool IAMFDecoderSource::seek(size_t frameIndex) {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  if (frameIndex >= streamData_.numFrames) {
    return false;
  }
  sampleCount_ = frameIndex * streamData_.frameSize;
  frameCount_ = frameIndex;
  finished_ = false;
  buffer_->seek(frameIndex);
  return true;
}

void IAMFDecoderSource::prepareToPlay(int, double) {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  if (!buffer_) {
    buffer_ = std::make_unique<BackgroundBuffer>(kPadSecs_, *decoder_);
  }
}

void IAMFDecoderSource::releaseResources() {}

void IAMFDecoderSource::setLayout(
    const Speakers::AudioElementSpeakerLayout layout) {
  const juce::SpinLock::ScopedLockType lock(stateLock_);
  LOG_INFO(0, "IAMFDecoderSource: Changing layout to " +
                  layout.toString().toStdString());

  // Destroy the buffer so it can't access the decoder during reconfiguration
  // Reconfigure the decoder with new settings
  buffer_.reset();
  decoder_->resetLayout(layout);
  streamData_ = decoder_->getStreamData();

  // Reset counters
  sampleCount_ = 0;
  frameCount_ = 0;
  finished_ = false;

  // Prepare to play again
  LOG_INFO(0, "IAMFDecoderSource: Buffering audio with new layout");
  buffer_ = std::make_unique<BackgroundBuffer>(kPadSecs_, *decoder_);

  LOG_INFO(0, "IAMFDecoderSource: Layout change complete. New channel count: " +
                  std::to_string(streamData_.numChannels));
}

void IAMFDecoderSource::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& info) {
  // If we can't take the lock (UI doing heavy background work), provide silence
  if (!stateLock_.tryEnter()) {
    info.clearActiveBufferRegion();
    return;
  }

  if (!isPlaying_) {
    info.clearActiveBufferRegion();
    stateLock_.exit();
    return;
  }

  const size_t kNumRead =
      buffer_->readSamples(*info.buffer, info.startSample, info.numSamples);
  sampleCount_ += kNumRead;
  frameCount_ = sampleCount_ / streamData_.frameSize;

  if (kNumRead < static_cast<size_t>(info.numSamples)) {
    info.buffer->clear(info.startSample + kNumRead, info.numSamples - kNumRead);
  }

  if (!finished_ && kNumRead == 0 && isPlaying_) {
    finished_ = true;
    if (onFinished_) onFinished_();
  }

  stateLock_.exit();
}