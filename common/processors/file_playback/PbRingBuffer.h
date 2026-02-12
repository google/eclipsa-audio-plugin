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

class PbRingBuffer {
 public:
  using Buffer = juce::AudioBuffer<float>;

  PbRingBuffer(int numChannels, size_t padSamples = 1024)
      : kPad_(padSamples),
        kCapacity_(3 * kPad_),
        buffer_(numChannels, (int)kCapacity_),
        head_(0),
        tail_(0) {}

  size_t availReadSamples() const { return distance(head_, tail_); }

  size_t availWriteSamples() const {
    return kCapacity_ - distance(head_, tail_) - 1;
  }

  bool writeSamples(size_t numSamples, const Buffer& in) {
    if (numSamples > availWriteSamples()) return false;

    const int numCh = std::min(buffer_.getNumChannels(), in.getNumChannels());

    size_t firstChunk = std::min(numSamples, kCapacity_ - tail_);
    size_t secondChunk = numSamples - firstChunk;

    for (int ch = 0; ch < numCh; ++ch) {
      buffer_.copyFrom(ch, (int)tail_, in, ch, 0, (int)firstChunk);
      if (secondChunk > 0)
        buffer_.copyFrom(ch, 0, in, ch, (int)firstChunk, (int)secondChunk);
    }

    tail_ = (tail_ + numSamples) % kCapacity_;

    jassert(availReadSamples() + availWriteSamples() == kCapacity_ - 1);
    return true;
  }

  size_t readSamples(size_t startSample, size_t numSamples, Buffer& out) {
    const size_t toRead = std::min(numSamples, availReadSamples());
    const int numCh = std::min(buffer_.getNumChannels(), out.getNumChannels());

    size_t firstChunk = std::min(toRead, kCapacity_ - head_);
    size_t secondChunk = toRead - firstChunk;

    for (int ch = 0; ch < numCh; ++ch) {
      out.copyFrom(ch, (int)startSample, buffer_, ch, (int)head_,
                   (int)firstChunk);
      if (secondChunk > 0)
        out.copyFrom(ch, (int)(startSample + firstChunk), buffer_, ch, 0,
                     (int)secondChunk);
    }

    head_ = (head_ + toRead) % kCapacity_;

    jassert(availReadSamples() + availWriteSamples() == kCapacity_ - 1);
    return toRead;
  }

  [[maybe_unused]] bool seek(size_t numSamples, bool forwards) {
    size_t availRead = availReadSamples();
    if (forwards && numSamples <= availRead) {
      head_ = (head_ + numSamples) % kCapacity_;
      return true;
    } else if (!forwards && numSamples <= (kCapacity_ - availRead)) {
      head_ = (head_ + kCapacity_ - numSamples) % kCapacity_;
      return true;
    } else {
      buffer_.clear();
      head_ = tail_ = 0;
      return false;
    }
  }

 private:
  size_t distance(size_t head, size_t tail) const {
    return (tail + kCapacity_ - head) % kCapacity_;
  }

  const size_t kPad_, kCapacity_;
  Buffer buffer_;
  size_t head_, tail_;
};
