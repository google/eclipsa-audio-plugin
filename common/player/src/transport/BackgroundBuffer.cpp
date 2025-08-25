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

#include "BackgroundBuffer.h"

#include <cstddef>

#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

BackgroundBuffer::BackgroundBuffer(const unsigned paddingSeconds,
                                   IAMFFileReader& decoder)
    : decoder_(decoder), stop_(false), eof_(false) {
  const auto kStreamData = decoder_.getStreamData();
  padSamples_ = std::min((size_t)paddingSeconds * kStreamData.sampleRate,
                         kStreamData.numFrames * kStreamData.frameSize);
  absSamplePos_ = 0;
  decoder_.seekFrame(0);
  pbuffer_ =
      std::make_unique<PbRingBuffer>(kStreamData.numChannels, padSamples_);
  decodeThread_ = std::thread(&BackgroundBuffer::decodeTask, this);
  notifyTask();
}

BackgroundBuffer::~BackgroundBuffer() {
  stop_ = true;
  cv_.notify_all();
  if (decodeThread_.joinable()) {
    decodeThread_.join();
  }
}

bool BackgroundBuffer::isReady() {
  const juce::SpinLock::ScopedLockType lock(bufferLock_);
  return pbuffer_->availReadSamples() >= padSamples_;
}

size_t BackgroundBuffer::availableSamples() const {
  const juce::SpinLock::ScopedLockType lock(bufferLock_);
  return pbuffer_->availReadSamples();
}

size_t BackgroundBuffer::readSamples(juce::AudioBuffer<float>& out,
                                     const unsigned startSample,
                                     const unsigned numSamples) {
  size_t kSamplesRead;
  {
    const juce::SpinLock::ScopedLockType lock(bufferLock_);
    if (pbuffer_->availReadSamples() == 0 && eof_) {
      out.clear(startSample, numSamples);
      return 0;
    }
    kSamplesRead = pbuffer_->readSamples(startSample, numSamples, out);
  }

  // Zero-pad if necessary. Ideally we never hit this.
  if (kSamplesRead < numSamples) {
    notifyTask();
    out.clear(startSample + kSamplesRead, numSamples - kSamplesRead);
  }

  absSamplePos_ += kSamplesRead;
  return kSamplesRead;
}

void BackgroundBuffer::seek(const size_t newFrameIdx) {
  bool posInBuff;
  const size_t kNewAbsSamplePos =
      newFrameIdx * decoder_.getStreamData().frameSize;
  {
    const juce::SpinLock::ScopedLockType lock(bufferLock_);
    size_t diff;
    if (kNewAbsSamplePos > absSamplePos_) {
      diff = kNewAbsSamplePos - absSamplePos_;
      posInBuff = pbuffer_->seek(diff, true);
    } else {
      diff = absSamplePos_ - kNewAbsSamplePos;
      posInBuff = pbuffer_->seek(diff, false);
    }
    // If frame was in buffer great - decoder can continue as normal.
    // If the frame was not in the buffer, decoder needs to seek to that pos.
    if (!posInBuff) {
      decoder_.seekFrame(newFrameIdx);
    }
  }
  absSamplePos_ = kNewAbsSamplePos;
  eof_ = false;
  notifyTask();
}

void BackgroundBuffer::notifyTask() { cv_.notify_all(); }

void BackgroundBuffer::decodeTask() {
  const IAMFFileReader::StreamData kStreamData = decoder_.getStreamData();
  juce::AudioBuffer<float> tempBuffer(kStreamData.numChannels,
                                      kStreamData.frameSize);
  while (!stop_) {
    std::unique_lock<std::mutex> lock(cvm_);
    cv_.wait_for(lock, std::chrono::milliseconds(5),
                 [this] { return stop_ || !eof_; });
    if (stop_) return;
    if (eof_) continue;
    // On startup, this thread will hold the lock and write until the buffer is
    // full.
    const juce::SpinLock::ScopedLockType bl(bufferLock_);
    while (pbuffer_->availWriteSamples() >= kStreamData.frameSize) {
      const size_t kSamplesDecoded = decoder_.readFrame(tempBuffer);
      if (kSamplesDecoded == 0) {
        eof_ = true;
        break;
      }
      pbuffer_->writeSamples(kSamplesDecoded, tempBuffer);
    }
  }
}