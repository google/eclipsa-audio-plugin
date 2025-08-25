#include "../src/transport/IAMFDecoderSource.h"

#include <gtest/gtest.h>

#include "processors/tests/FileOutputTestFixture.h"

class IAMFDecoderSourceTest : public FileOutputTests {};

// Use a decoder audio source to entirely parse a file.
// Validate audio content is as expected.
TEST_F(IAMFDecoderSourceTest, decode_all_samples) {
  const std::filesystem::path kTestFilePath =
      std::filesystem::current_path() / "source_test.iamf";
  createIAMFFile2AE2MP(kTestFilePath);

  auto source = IAMFDecoderSource(kTestFilePath);

  // Configuration
  std::atomic_bool done = false;
  source.setOnFinishedCallback([&done]() { done = true; });
  source.prepareToPlay(-1, -1);
  const int kBufferSz = 67;
  juce::AudioBuffer<float> buffer(source.getStreamData().numChannels,
                                  kBufferSz);
  juce::AudioSourceChannelInfo info(buffer);

  // Calculate total samples in the file (2 seconds at 16kHz)
  const size_t kTotalSamples = 2 * source.getStreamData().sampleRate;

  // Wait for some samples to populate
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  size_t frameCount = 0;
  source.play();
  while (!done) {
    source.getNextAudioBlock(info);
    // Validate block content
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
      for (int i = 0; i < buffer.getNumSamples(); ++i) {
        const size_t globalSample = frameCount * kBufferSz + i;
        float actual = buffer.getSample(channel, i);

        if (globalSample < kTotalSamples) {
          // Within file bounds: validate against expected sine wave
          float expected = sampleSine(440.0f, globalSample,
                                      source.getStreamData().sampleRate);
          ASSERT_NEAR(actual, expected, 0.0001f)
              << "Mismatch at global sample " << globalSample << ", channel "
              << channel << ", sample " << i;
        } else {
          // Beyond file bounds: should be zero-padded
          ASSERT_NEAR(actual, 0.0f, 0.0001f)
              << "Expected zero-padding at global sample " << globalSample
              << ", channel " << channel << ", sample " << i;
        }
      }
    }
    ++frameCount;
  }
}