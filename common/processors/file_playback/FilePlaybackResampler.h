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
#include <juce_audio_basics/juce_audio_basics.h>

#include "IAMFBufferedReader.h"

/**
 * @brief Resamples IAMF audio data from a source sample rate to a target sample
 * rate. Uses the buffered reader as an audio source that gets fed to the JUCE
 * resampler, which the processor pulls from in processBlock().
 *
 */
class FilePlaybackResampler {
 public:
  struct Context {
    double srcRate, dstRate;
    int numChannels;
  };

  FilePlaybackResampler() {}
  ~FilePlaybackResampler() {}

  void prepare(double sourceRate, double targetRate, int channels) {
    jassert(sourceRate > 0.0);
    jassert(targetRate > 0.0);
    jassert(channels > 0);
    if (sourceRate <= 0.0 || targetRate <= 0.0) {
      return;
    }

    numChannels_ = channels;
    dstRate_ = targetRate;
    resampleRatio_ = sourceRate / targetRate;

    // Configure the JUCE resampler with our adapter input
    if (!resampler_ || resamplerNumChannels_ != numChannels_) {
      resampler_ = std::make_unique<juce::ResamplingAudioSource>(
          &bbSource_, false, numChannels_);
      resamplerNumChannels_ = numChannels_;
    }
    resampler_->setResamplingRatio(resampleRatio_);
    // Prepare once with a reasonable block size; DO NOT re-prepare on every
    // block
    resampler_->prepareToPlay(4096, dstRate_);
    flush();
  }

  void flush() {
    if (resampler_) {
      resampler_->flushBuffers();
    }
  }

  int read(IamfBufferedReader& sourceFifo, juce::AudioBuffer<float>& destBuffer,
           int numSamplesToProduce) {
    jassert(destBuffer.getNumChannels() >= numChannels_);
    jassert(numSamplesToProduce > 0);

    // Ensure adapter points to the current FIFO
    bbSource_.setSourceBuffer(&sourceFifo);

    // Capture the cumulative input consumed before this block
    const size_t kInputBefore = bbSource_.getCumulativeInputRead();

    // Use destBuffer directly without resizing to avoid state disruption
    destBuffer.clear();
    juce::AudioSourceChannelInfo info(&destBuffer, 0, numSamplesToProduce);
    resampler_->getNextAudioBlock(info);

    // Calculate how many input samples were consumed for this specific block
    const size_t kInputAfter = bbSource_.getCumulativeInputRead();
    const size_t kInputForThisBlock = kInputAfter - kInputBefore;

    return static_cast<int>(kInputForThisBlock);
  }

 private:
  // Adapter that exposes BackgroundBuffer as an AudioSource for the resampler
  class IamfAudioSource : public juce::AudioSource {
   public:
    void setSourceBuffer(IamfBufferedReader* buf) { buffer_ = buf; }

    size_t getCumulativeInputRead() const { return cumulativeInputRead_; }

    void prepareToPlay(int, double) override {}
    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override {
      if (buffer_ == nullptr || info.buffer == nullptr ||
          info.numSamples <= 0) {
        if (info.buffer != nullptr)
          info.buffer->clear(info.startSample, info.numSamples);
        return;
      }

      // Read available samples into the provided buffer region
      const unsigned kToRead = info.numSamples;
      const size_t kRead =
          buffer_->readSamples(*info.buffer, info.startSample, kToRead);

      // Accumulate total input samples read
      cumulativeInputRead_ += kRead;

      // If fewer samples were available, let the resampler handle the underrun
      // rather than abruptly zeroing (this avoids clicks from truncation)
      if (kRead < kToRead) {
        // Only zero the unread portion if we got nothing at all
        if (kRead == 0) {
          const unsigned kRem = kToRead;
          info.buffer->clear(info.startSample, kRem);
        }
        // Otherwise let the resampler interpolate from partial data
      }
    }

   private:
    IamfBufferedReader* buffer_ = nullptr;
    size_t cumulativeInputRead_ = 0;
  };

  IamfAudioSource bbSource_;
  std::unique_ptr<juce::ResamplingAudioSource> resampler_;
  int resamplerNumChannels_ = 0;
  double resampleRatio_ = 1.0;
  double dstRate_ = 0.0;
  int numChannels_ = 0;
};
