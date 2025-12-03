#include "../src/transport/BackgroundBuffer.h"

#include <gtest/gtest.h>

#include <filesystem>

#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"
#include "processors/tests/FileOutputTestFixture.h"

class BackgroundBufferTest : public FileOutputTests {
  void TearDown() override {
    decoder_.reset();  // Releaes the file before deleting
    if (std::filesystem::exists(kTestFilePath_)) {
      std::filesystem::remove(kTestFilePath_);
    }
  }

 protected:
  std::unique_ptr<IAMFFileReader> decoder_;
  std::filesystem::path kTestFilePath_;
};

class BackgroundBufferStereoTest : public BackgroundBufferTest {
 protected:
  void SetUp() override {
    kTestFilePath_ = std::filesystem::current_path() / "buffer_test.iamf";
    createIAMFFile30SecStereo(kTestFilePath_);
    decoder_ = IAMFFileReader::createIamfReader(kTestFilePath_);
    ASSERT_NE(decoder_, nullptr);
  }
};

class BackgroundBuffer2AETest : public BackgroundBufferTest {
 protected:
  void SetUp() override {
    kTestFilePath_ = std::filesystem::current_path() / "buffer_test.iamf";
    createIAMFFile2AE2MP(kTestFilePath_);
    decoder_ = IAMFFileReader::createIamfReader(kTestFilePath_);
    ASSERT_NE(decoder_, nullptr);
  }
};

static void waitForData() {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

static void waitForReady(BackgroundBuffer& buffer) {
  const unsigned kMaxWaitMs = 5000;
  unsigned waitedMs = 0;
  while (!buffer.isReady() && waitedMs < kMaxWaitMs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    waitedMs += 100;
  }
  EXPECT_TRUE(buffer.isReady());
}

const std::filesystem::path kReferenceFilePath =
    std::filesystem::current_path() / "../common/player/test/test_resources" /
    "test.iamf";

// 1. Test creating and filling the buffer.
TEST(BackgroundBuffer, fill) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);
  BackgroundBuffer buffer(1, *decoder);

  waitForData();
  EXPECT_TRUE(buffer.availableSamples() > 0);
}

// 2. Test filling the buffer then reading some samples.
TEST(BackgroundBuffer, fill_read) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);
  BackgroundBuffer buffer(1, *decoder);

  waitForReady(buffer);

  juce::AudioBuffer<float> out(decoder->getStreamData().numChannels,
                               decoder->getStreamData().frameSize);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());

  juce::AudioBuffer<float> out2(decoder->getStreamData().numChannels,
                                decoder->getStreamData().frameSize + 7);
  EXPECT_EQ(buffer.readSamples(out2, 0, out2.getNumSamples()),
            out2.getNumSamples());
}

// 3. Test filling the buffer, then seeking to a position ahead but in the
// buffer.
TEST(BackgroundBuffer, fill_seek_ahead) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);
  BackgroundBuffer buffer(1, *decoder);

  waitForReady(buffer);

  EXPECT_TRUE(buffer.availableSamples() > 0);

  juce::AudioBuffer<float> out(decoder->getStreamData().numChannels,
                               decoder->getStreamData().frameSize);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());

  buffer.seek(20);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());
}

// 4. Test filling the buffer, then seeking to a position behind but in the
// buffer.
TEST(BackgroundBuffer, fill_seek_behind) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);

  const unsigned kPadSecs = 1;
  const size_t kPadSamples = decoder->getStreamData().sampleRate * kPadSecs;
  BackgroundBuffer buffer(kPadSecs, *decoder);

  waitForReady(buffer);

  EXPECT_TRUE(buffer.availableSamples() > 0);

  // Read through the padding. The underlying window should retain the padding
  // as it's the first time data is being read from the buffer.
  juce::AudioBuffer<float> out(decoder->getStreamData().numChannels,
                               kPadSamples);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());

  // We expect that if we seek to somewhere within that initial padding, the
  // data will be within our buffer.
  buffer.seek(0);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());
}

// 5. Test filling the buffer, then seeking to a position ahead outside the
// buffer.
TEST(BackgroundBuffer, fill_seek_ahead_ob) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);

  const unsigned kPadSecs = 1;
  const size_t kPadSamples = decoder->getStreamData().sampleRate * kPadSecs;
  BackgroundBuffer buffer(kPadSecs, *decoder);

  waitForReady(buffer);

  EXPECT_TRUE(buffer.availableSamples() > 0);

  // Attempt seeking to a position outside the amount of padding we have.
  buffer.seek(kPadSamples * 3);
}

// 6. Test filling the buffer, then seeking to a position behind outside the
// buffer.
TEST(BackgroundBuffer, fill_seek_behind_ob) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);

  const unsigned kPadSecs = 1;
  const size_t kPadSamples = decoder->getStreamData().sampleRate * kPadSecs;
  BackgroundBuffer buffer(kPadSecs, *decoder);

  waitForReady(buffer);

  EXPECT_TRUE(buffer.availableSamples() > 0);

  // Read through the padding. The underlying window should retain the padding
  // as it's the first time data is being read from the buffer. But we expect
  // the requested frame to not be in the buffer as we've read past it.
  juce::AudioBuffer<float> out(decoder->getStreamData().numChannels,
                               kPadSamples * 2);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());

  // Attempt seeking to a position outside the amount of padding we have.
  buffer.seek(0);
}

// 7. Read through the entire IAMF file.
TEST(BackgroundBuffer, whole_file) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);

  const unsigned kPadSecs = 3;
  const size_t kPadSamples = decoder->getStreamData().sampleRate * kPadSecs;
  BackgroundBuffer buffer(kPadSecs, *decoder);

  const IAMFFileReader::StreamData kSData = decoder->getStreamData();
  const size_t kTotalSamples = kSData.numFrames * kSData.frameSize;
  const size_t kBufferSz = 1024;
  juce::AudioBuffer<float> out(kSData.numChannels, kBufferSz);

  size_t totalSamplesRead = 0;
  while (totalSamplesRead < kTotalSamples) {
    if (buffer.availableSamples()) {
      size_t samplesRead = buffer.readSamples(out, 0, kBufferSz);
      totalSamplesRead += samplesRead;
    } else {
      waitForData();
    }
  }
  EXPECT_EQ(totalSamplesRead, kTotalSamples);
}

// 8. Using the output test fixture, write an IAMF file. Read the IAMF file back
// from the buffer and validate each sample is as expected.
TEST_F(BackgroundBuffer2AETest, write_read_validate) {
  const unsigned kPadSecs = 1;
  BackgroundBuffer buffer(kPadSecs, *decoder_);

  const IAMFFileReader::StreamData kSData = decoder_->getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.sampleRate, kSampleRate);
  EXPECT_EQ(kSData.numChannels, Speakers::kStereo.getNumChannels());

  // Read and validate samples
  juce::AudioBuffer<float> readBuffer(kSData.numChannels, kSData.frameSize);
  size_t totalFramesRead = 0;

  // Read through the entire file
  while (totalFramesRead < kSData.numFrames) {
    // Wait if not enough samples available
    if (buffer.availableSamples() < kSData.frameSize) {
      waitForData();
      continue;
    }

    size_t samplesRead = buffer.readSamples(readBuffer, 0, kSData.frameSize);
    ASSERT_EQ(samplesRead, kSData.frameSize);

    // Validate each sample matches the expected 440Hz sine wave
    for (int channel = 0; channel < kSData.numChannels; ++channel) {
      for (int i = 0; i < samplesRead; ++i) {
        const float expected = sampleSine(
            440.0f, totalFramesRead * kSData.frameSize + i, kSData.sampleRate);
        const float actual = readBuffer.getSample(channel, i);

        // Use a tolerance to account for codec artifacts
        ASSERT_NEAR(actual, expected, 0.0001f)
            << "Mismatch at frame " << totalFramesRead << ", channel "
            << channel << ", sample " << i;
      }
    }

    ++totalFramesRead;
  }

  // Verify we read the expected number of frames
  EXPECT_GT(totalFramesRead, 0);
}

// 9. Try various reads and writes
TEST_F(BackgroundBuffer2AETest, vary_read_write) {
  const unsigned kPadSecs = 1;
  BackgroundBuffer buffer(kPadSecs, *decoder_);

  const IAMFFileReader::StreamData kSData = decoder_->getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.sampleRate, kSampleRate);
  EXPECT_EQ(kSData.numChannels, Speakers::kStereo.getNumChannels());

  // Read and validate samples
  const size_t kBufferSz = 1024;
  juce::AudioBuffer<float> readBuffer(kSData.numChannels, kBufferSz);

  // Read through the entire file, varying the read sizes
  const size_t kTotalSamps = kSData.numFrames * kSData.frameSize;
  size_t totalSamplesRead = 0, samplesToRead = kBufferSz;
  while (totalSamplesRead < kTotalSamps) {
    size_t startSample = kBufferSz - samplesToRead;
    size_t samplesRead =
        buffer.readSamples(readBuffer, startSample, samplesToRead);
    if (samplesRead < samplesToRead) {
      waitForData();
    }

    // Validate each sample matches the expected 440Hz sine wave
    for (int channel = 0; channel < kSData.numChannels; ++channel) {
      for (int i = 0; i < samplesRead; ++i) {
        const float expected =
            sampleSine(440.0f, totalSamplesRead + i, kSData.sampleRate);
        const float actual = readBuffer.getSample(channel, startSample + i);

        // Use a tolerance to account for codec artifacts
        ASSERT_NEAR(actual, expected, 0.0001f)
            << "Mismatch at sample " << totalSamplesRead << ", channel "
            << channel << ", sample " << i;
      }
    }
    totalSamplesRead += samplesRead;
    samplesToRead = (samplesToRead + 1) % kBufferSz;
  }

  // Verify we read the expected number of frames
  EXPECT_EQ(totalSamplesRead, kTotalSamps);
}

// 10. Try various reads and writes on a longer file
TEST_F(BackgroundBufferStereoTest, vary_read_write_long) {
  const unsigned kPadSecs = 1;
  BackgroundBuffer buffer(kPadSecs, *decoder_);

  const IAMFFileReader::StreamData kSData = decoder_->getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.sampleRate, kSampleRate);
  EXPECT_EQ(kSData.numChannels, Speakers::kStereo.getNumChannels());

  // Read and validate samples
  const size_t kBufferSz = 1024;
  juce::AudioBuffer<float> readBuffer(kSData.numChannels, kBufferSz);

  // Read through the entire file, varying the read sizes
  const size_t kTotalSamps = kSData.numFrames * kSData.frameSize;
  size_t totalSamplesRead = 0, samplesToRead = kBufferSz;
  while (totalSamplesRead < kTotalSamps) {
    size_t startSample = kBufferSz - samplesToRead;
    size_t samplesRead =
        buffer.readSamples(readBuffer, startSample, samplesToRead);
    if (samplesRead < samplesToRead) {
      std::cout << "Waiting for data at totalSamplesRead: " << totalSamplesRead
                << std::endl;
      waitForData();
    }

    // Validate each sample matches the expected 440Hz sine wave
    for (int channel = 0; channel < kSData.numChannels; ++channel) {
      for (int i = 0; i < samplesRead; ++i) {
        const float expected =
            sampleSine(440.0f, totalSamplesRead + i, kSData.sampleRate);
        const float actual = readBuffer.getSample(channel, startSample + i);

        // Use a tolerance to account for codec artifacts
        ASSERT_NEAR(actual, expected, 0.0001f)
            << "Mismatch at sample " << totalSamplesRead << ", channel "
            << channel << ", sample " << i;
      }
    }
    totalSamplesRead += samplesRead;
    samplesToRead = (samplesToRead + 1) % kBufferSz;
  }

  // Verify we read the expected number of frames
  EXPECT_EQ(totalSamplesRead, kTotalSamps);
}

// 11. Try various reads and writes on a longer file, vary buffer padding size
TEST_F(BackgroundBufferStereoTest, vary_read_write_long_vary_pad) {
  for (const unsigned kPadSecs : {2, 4, 8, 16, 32, 64}) {
    std::cout << "Testing with pad seconds: " << kPadSecs << std::endl;
    BackgroundBuffer buffer(kPadSecs, *decoder_);

    const IAMFFileReader::StreamData kSData = decoder_->getStreamData();
    EXPECT_TRUE(kSData.valid);
    EXPECT_EQ(kSData.sampleRate, kSampleRate);
    EXPECT_EQ(kSData.numChannels, Speakers::kStereo.getNumChannels());

    // Read and validate samples
    const size_t kBufferSz = 1024;
    juce::AudioBuffer<float> readBuffer(kSData.numChannels, kBufferSz);

    // Read through the entire file, varying the read sizes
    const size_t kTotalSamps = kSData.numFrames * kSData.frameSize;
    size_t totalSamplesRead = 0, samplesToRead = kBufferSz;
    while (totalSamplesRead < kTotalSamps) {
      size_t startSample = kBufferSz - samplesToRead;
      size_t samplesRead =
          buffer.readSamples(readBuffer, startSample, samplesToRead);
      if (samplesRead < samplesToRead) {
        std::cout << "Waiting for data at totalSamplesRead: "
                  << totalSamplesRead << " with pad seconds: " << kPadSecs
                  << std::endl;
        waitForData();
      }

      // Validate each sample matches the expected 440Hz sine wave
      for (int channel = 0; channel < kSData.numChannels; ++channel) {
        for (int i = 0; i < samplesRead; ++i) {
          const float expected =
              sampleSine(440.0f, totalSamplesRead + i, kSData.sampleRate);
          const float actual = readBuffer.getSample(channel, startSample + i);

          // Use a tolerance to account for codec artifacts
          ASSERT_NEAR(actual, expected, 0.0001f)
              << "Mismatch at sample " << totalSamplesRead << ", channel "
              << channel << ", sample " << i;
        }
      }
      totalSamplesRead += samplesRead;
      samplesToRead = (samplesToRead + 1) % kBufferSz;
    }

    // Verify we read the expected number of frames
    EXPECT_EQ(totalSamplesRead, kTotalSamps);
  }
}

// 12. Using the output test fixture, write an IAMF file. Read the IAMF file
// back from the buffer and validate each sample is as expected.
TEST_F(BackgroundBufferStereoTest, seek_and_validate) {
  const unsigned kPadSecs = 1;
  BackgroundBuffer buffer(kPadSecs, *decoder_);

  waitForReady(buffer);
  ASSERT_TRUE(buffer.availableSamples() > 0);

  const IAMFFileReader::StreamData kSData = decoder_->getStreamData();
  EXPECT_TRUE(kSData.valid);
  EXPECT_EQ(kSData.sampleRate, kSampleRate);
  EXPECT_EQ(kSData.numChannels, Speakers::kStereo.getNumChannels());

  // Read and validate samples
  juce::AudioBuffer<float> readBuffer(kSData.numChannels, kSData.frameSize);
  size_t totalFramesRead = 0;

  // Seek to a position within the padding and validate samples
  size_t seekFrameIdx = 10;
  buffer.seek(seekFrameIdx);
  ASSERT_EQ(buffer.readSamples(readBuffer, 0, kSData.frameSize),
            kSData.frameSize);
  for (int channel = 0; channel < kSData.numChannels; ++channel) {
    for (int i = 0; i < kSData.frameSize; ++i) {
      const float expected = sampleSine(
          440.0f, seekFrameIdx * kSData.frameSize + i, kSData.sampleRate);
      const float actual = readBuffer.getSample(channel, i);

      // Use a tolerance to account for codec artifacts
      ASSERT_NEAR(actual, expected, 0.0001f)
          << "Mismatch at frame " << totalFramesRead << ", channel " << channel
          << ", sample " << i;
    }
  }

  // Seek to a position outside the padding and validate samples
  seekFrameIdx = 1000;
  buffer.seek(seekFrameIdx);
  waitForData();
  ASSERT_EQ(buffer.readSamples(readBuffer, 0, kSData.frameSize),
            kSData.frameSize);
  for (int channel = 0; channel < kSData.numChannels; ++channel) {
    for (int i = 0; i < kSData.frameSize; ++i) {
      const float expected = sampleSine(
          440.0f, seekFrameIdx * kSData.frameSize + i, kSData.sampleRate);
      const float actual = readBuffer.getSample(channel, i);

      // Use a tolerance to account for codec artifacts
      ASSERT_NEAR(actual, expected, 0.0001f)
          << "Mismatch at frame " << totalFramesRead << ", channel " << channel
          << ", sample " << i;
    }
  }
}