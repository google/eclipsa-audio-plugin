#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

#include "BackgroundBuffer.h"

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
    if (resampler_) resampler_->flushBuffers();
  }

  int read(BackgroundBuffer& sourceFifo, juce::AudioBuffer<float>& destBuffer,
           int numSamplesToProduce) {
    jassert(destBuffer.getNumChannels() >= numChannels_);
    jassert(numSamplesToProduce > 0);

    // Ensure adapter points to the current FIFO
    bbSource_.setBackgroundBuffer(&sourceFifo);

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
  class BackgroundBufferAudioSource : public juce::AudioSource {
   public:
    void setBackgroundBuffer(BackgroundBuffer* buf) { buffer_ = buf; }

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
      const unsigned toRead = static_cast<unsigned>(info.numSamples);
      const size_t samplesRead = buffer_->readSamples(
          *info.buffer, static_cast<unsigned>(info.startSample), toRead);

      // Accumulate total input samples read
      cumulativeInputRead_ += samplesRead;

      // If fewer samples were available, let the resampler handle the underrun
      // rather than abruptly zeroing (this avoids clicks from truncation)
      if (samplesRead < toRead) {
        // Only zero the unread portion if we got nothing at all
        if (samplesRead == 0) {
          const int rem = static_cast<int>(toRead);
          info.buffer->clear(info.startSample, rem);
        }
        // Otherwise let the resampler interpolate from partial data
      }
    }

   private:
    BackgroundBuffer* buffer_ = nullptr;
    size_t cumulativeInputRead_ = 0;
  };

  BackgroundBufferAudioSource bbSource_;
  std::unique_ptr<juce::ResamplingAudioSource> resampler_;
  int resamplerNumChannels_ = 0;
  double resampleRatio_ = 1.0;
  double dstRate_ = 0.0;
  int numChannels_ = 0;
};
