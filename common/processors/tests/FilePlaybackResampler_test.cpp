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

#include "../file_playback/FilePlaybackResampler.h"

#include <gtest/gtest.h>

#include <memory>

#include "FileOutputTestFixture.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

#undef RDR_TO_FILE

// class FilePlaybackResamplerTest : public FileOutputTests {
//  protected:
//   FilePlaybackResamplerTest() {
//     // Create the file
//     const std::filesystem::path kReferenceFilePath =
//         std::filesystem::current_path() / "test_fpb_resampler.iamf";
//     createIAMFFile2AE2MP(kReferenceFilePath);

//     // Create the reader and background buffer
//     reader = IAMFFileReader::createIamfReader(
//         kReferenceFilePath.string(), IAMFFileReader::kDefaultReaderSettings,
//         abort_);
//     backgroundBuffer = std::make_unique<IamfBackgroundBuffer>(2, *reader);
//     while (!backgroundBuffer->isReady()) {
//       std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }

//     // Create the resampler
//     const double kSourceRate = reader->getStreamData().sampleRate;
//   }

//   std::unique_ptr<IAMFFileReader> reader;
//   std::unique_ptr<IamfBackgroundBuffer> backgroundBuffer;
//   FilePlaybackResampler resampler;
//   std::atomic_bool abort_{false};
// };

// Feed an IAMF file with a tone to the BackgroundBuffer then test the
// resampling functionality of the ResamplingReader class.
#ifdef RDR_TO_FILE
TEST_F(FilePlaybackResamplerTest, vary_rates) {
  const auto kSourceRate = reader->getStreamData().sampleRate;
  for (const double kTargetRate : {16e3, 44.1e3, 48e3, 96e3}) {
    backgroundBuffer->seek(0);

    resampler.prepare(kSourceRate, kTargetRate,
                      static_cast<int>(reader->getStreamData().numChannels));

    // Create output WAV file for this target rate
    const std::filesystem::path outputPath =
        std::filesystem::current_path() /
        ("resampled_" + std::to_string(static_cast<int>(kTargetRate)) + ".wav");
    WavFileWriter wavWriter(
        outputPath, static_cast<int>(reader->getStreamData().numChannels),
        kTargetRate);
    ASSERT_TRUE(wavWriter.isOpen()) << "Failed to open WAV file for writing";

    // Read and write samples through the resampler
    const int kNumSamplesToRead = 512;
    const int kTotalSamples = static_cast<int>(kTargetRate * 2);  // 2 seconds
    int samplesWritten = 0;
    juce::AudioBuffer<float> destBuffer(
        static_cast<int>(reader->getStreamData().numChannels),
        kNumSamplesToRead);

    while (samplesWritten < kTotalSamples) {
      const int samplesToRead =
          std::min(kNumSamplesToRead, kTotalSamples - samplesWritten);
      resampler.read(*backgroundBuffer, destBuffer, samplesToRead);
      ASSERT_TRUE(wavWriter.write(destBuffer, samplesToRead))
          << "Failed to write samples to WAV file";
      samplesWritten += samplesToRead;
    }
  }
}
#endif
