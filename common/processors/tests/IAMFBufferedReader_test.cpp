#include "../file_playback/IAMFBufferedReader.h"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>

#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"
#include "processors/tests/FileOutputTestFixture.h"

class IAMFBufferedReaderTest : public FileOutputTests {
  void TearDown() override {
    decoder_.reset();
    if (std::filesystem::exists(kTestFilePath_)) {
      std::filesystem::remove(kTestFilePath_);
    }
  }

 protected:
  std::unique_ptr<IAMFFileReader> decoder_;
  std::filesystem::path kTestFilePath_;
};

class IAMFBufferedReaderStereoTest : public IAMFBufferedReaderTest {
 protected:
  void SetUp() override {
    kTestFilePath_ = std::filesystem::current_path() / "buffer_test.iamf";
    createIAMFFile30SecStereo(kTestFilePath_);
    std::atomic_bool cancel = false;
    decoder_ = IAMFFileReader::createIamfReader(kTestFilePath_);
    ASSERT_NE(decoder_, nullptr);
    decoder_->indexFile(cancel);
  }
};

class IAMFBufferedReader2AETest : public IAMFBufferedReaderTest {
 protected:
  void SetUp() override {
    kTestFilePath_ = std::filesystem::current_path() / "buffer_test.iamf";
    createIAMFFile2AE2MP(kTestFilePath_);
    std::atomic_bool cancel = false;
    decoder_ = IAMFFileReader::createIamfReader(kTestFilePath_);
    ASSERT_NE(decoder_, nullptr);
    decoder_->indexFile(cancel);
  }
};

const std::filesystem::path kReferenceFilePath =
    std::filesystem::current_path() /
    "../common/processors/tests/test_resources/" / "HashSourceFileDebug.iamf";

// 1. Test creating and filling the buffer.
TEST(IAMFBufferedReader, fill) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);
  IamfBufferedReader buffer(std::move(decoder), 1);

  buffer.waitUntilReady();

  EXPECT_TRUE(buffer.availableSamples() > 0);
}

// 2. Test filling the buffer then reading some samples.
TEST(IAMFBufferedReader, fill_read) {
  std::atomic_bool cancel = false;
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  decoder->indexFile(cancel);
  ASSERT_NE(decoder, nullptr);
  const IAMFFileReader::StreamData kSData = decoder->getStreamData();
  IamfBufferedReader buffer(std::move(decoder), 1);
  buffer.waitUntilReady();

  juce::AudioBuffer<float> out(kSData.numChannels, kSData.frameSize);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());

  juce::AudioBuffer<float> out2(kSData.numChannels, kSData.frameSize + 7);
  EXPECT_EQ(buffer.readSamples(out2, 0, out2.getNumSamples()),
            out2.getNumSamples());
}

// 3. Test filling the buffer, then seeking to a position ahead but in the
// buffer.
TEST(IAMFBufferedReader, fill_seek_ahead) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);
  std::atomic_bool cancel = false;
  decoder->indexFile(cancel);

  const IAMFFileReader::StreamData kSData = decoder->getStreamData();
  IamfBufferedReader buffer(std::move(decoder), 1);

  buffer.waitUntilReady();

  EXPECT_TRUE(buffer.availableSamples() > 0);

  juce::AudioBuffer<float> out(kSData.numChannels, kSData.frameSize);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());

  buffer.seek(3);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());
}

// 4. Test filling the buffer, then seeking to a position behind but in the
// buffer.
TEST(IAMFBufferedReader, fill_seek_behind) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);
  std::atomic_bool cancel = false;
  decoder->indexFile(cancel);

  const size_t kPadSamples = 100;
  const IAMFFileReader::StreamData kSData = decoder->getStreamData();
  IamfBufferedReader buffer(std::move(decoder), 1);

  buffer.waitUntilReady();

  EXPECT_TRUE(buffer.availableSamples() > 0);

  // Read through the padding. The underlying window should retain the padding
  // as it's the first time data is being read from the buffer.
  juce::AudioBuffer<float> out(kSData.numChannels, kPadSamples);
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
TEST(IAMFBufferedReader, fill_seek_ahead_ob) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);
  std::atomic_bool cancel = false;
  decoder->indexFile(cancel);

  const unsigned kPadSecs = 1;
  const size_t kPadSamples = decoder->getStreamData().sampleRate * kPadSecs;
  IamfBufferedReader buffer(std::move(decoder), kPadSecs);

  buffer.waitUntilReady();

  EXPECT_TRUE(buffer.availableSamples() > 0);

  // Attempt seeking to a position outside the amount of padding we have.
  buffer.seek(kPadSamples * 3);
}

// 6. Test filling the buffer, then seeking to a position behind outside the
// buffer.
TEST(IAMFBufferedReader, fill_seek_behind_ob) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);
  std::atomic_bool cancel = false;
  decoder->indexFile(cancel);

  const size_t kPadSamples = 100;
  IamfBufferedReader buffer(std::move(decoder), 1);

  buffer.waitUntilReady();

  EXPECT_TRUE(buffer.availableSamples() > 0);

  // Read through the padding. The underlying window should retain the padding
  // as it's the first time data is being read from the buffer. But we expect
  // the requested frame to not be in the buffer as we've read past it.
  juce::AudioBuffer<float> out(2, kPadSamples * 2);
  EXPECT_EQ(buffer.readSamples(out, 0, out.getNumSamples()),
            out.getNumSamples());

  // Attempt seeking to a position outside the amount of padding we have.
  buffer.seek(0);
}

// 7. Read through the entire IAMF file.
TEST(IAMFBufferedReader, whole_file) {
  auto decoder = IAMFFileReader::createIamfReader(kReferenceFilePath);
  ASSERT_NE(decoder, nullptr);
  std::atomic_bool cancel = false;
  decoder->indexFile(cancel);
  const IAMFFileReader::StreamData kSData = decoder->getStreamData();

  const unsigned kPadSecs = 3;
  const size_t kPadSamples = kSData.sampleRate * kPadSecs;
  IamfBufferedReader buffer(std::move(decoder), kPadSecs);

  const size_t kTotalSamples = kSData.numFrames * kSData.frameSize;
  const size_t kBufferSz = 1024;
  juce::AudioBuffer<float> out(kSData.numChannels, kBufferSz);

  size_t totalSamplesRead = 0;
  while (totalSamplesRead < kTotalSamples) {
    if (buffer.availableSamples()) {
      size_t samplesRead = buffer.readSamples(out, 0, kBufferSz);
      totalSamplesRead += samplesRead;
    } else {
      buffer.waitUntilReady();
    }
  }
  EXPECT_EQ(totalSamplesRead, kTotalSamples);
}

// 8. Using the output test fixture, write an IAMF file. Read the IAMF file back
// from the buffer and validate each sample is as expected.
TEST_F(IAMFBufferedReader2AETest, write_read_validate) {
  const unsigned kPadSecs = 1;
  const IAMFFileReader::StreamData kSData = decoder_->getStreamData();
  IamfBufferedReader buffer(std::move(decoder_), kPadSecs);

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
      buffer.waitUntilReady();
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
TEST_F(IAMFBufferedReader2AETest, vary_read_write) {
  const unsigned kPadSecs = 1;
  const IAMFFileReader::StreamData kSData = decoder_->getStreamData();
  IamfBufferedReader buffer(std::move(decoder_), kPadSecs);

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
      buffer.waitUntilReady();
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
TEST_F(IAMFBufferedReaderStereoTest, vary_read_write_long) {
  const unsigned kPadSecs = 1;
  const IAMFFileReader::StreamData kSData = decoder_->getStreamData();
  IamfBufferedReader buffer(std::move(decoder_), kPadSecs);

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
      buffer.waitUntilReady();
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
TEST_F(IAMFBufferedReaderStereoTest, vary_read_write_long_vary_pad) {
  for (const unsigned kPadSecs : {2, 4, 8, 16, 32, 64}) {
    std::cout << "Testing with pad seconds: " << kPadSecs << std::endl;

    // Recreate decoder for each iteration since we're moving it
    auto decoder = IAMFFileReader::createIamfReader(kTestFilePath_);
    ASSERT_NE(decoder, nullptr);
    const IAMFFileReader::StreamData kSData = decoder->getStreamData();

    IamfBufferedReader buffer(std::move(decoder), kPadSecs);
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
        buffer.waitUntilReady();
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
TEST_F(IAMFBufferedReaderStereoTest, seek_and_validate) {
  const unsigned kPadSecs = 1;
  const IAMFFileReader::StreamData kSData = decoder_->getStreamData();
  IamfBufferedReader buffer(std::move(decoder_), kPadSecs);

  buffer.waitUntilReady();

  ASSERT_TRUE(buffer.availableSamples() > 0);

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
  buffer.waitUntilReady();
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