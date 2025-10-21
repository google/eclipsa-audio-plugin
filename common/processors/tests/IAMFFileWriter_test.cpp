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

#include "../file_output/iamf_export_utils/IAMFFileWriter.h"

#include <gtest/gtest.h>

#include <filesystem>

#include "FileOutputTestFixture.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class IAMFFileWriterTest : public FileOutputTests {};

// Open and close the writer
TEST_F(IAMFFileWriterTest, open_close) {
  iamfOutPath = std::filesystem::current_path() / "writer_test.iamf";
  IAMFFileWriter writer(fileExportRepository, audioElementRepository,
                        mixRepository, mixPresentationLoudnessRepository,
                        kSamplesPerFrame, kSampleRate);
  EXPECT_TRUE(writer.open(iamfOutPath));
  EXPECT_TRUE(writer.close());
}

// Write a simple IAMF file. Decode the file to WAV and compare with source
// audio
TEST_F(IAMFFileWriterTest, write_iamf) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  IAMFFileWriter writer(fileExportRepository, audioElementRepository,
                        mixRepository, mixPresentationLoudnessRepository,
                        kSamplesPerFrame, kSampleRate);
  EXPECT_TRUE(writer.open(iamfOutPath));

  // Generate and write 375 frames of sine tone audio
  juce::AudioBuffer<float> buffer(2, kSamplesPerFrame);
  for (int frame = 0; frame < 375; ++frame) {
    for (int channel = 0; channel < 2; ++channel) {
      for (int sample = 0; sample < kSamplesPerFrame; ++sample) {
        const double kTime = (frame * kSamplesPerFrame + sample) /
                             static_cast<double>(kSampleRate);
        buffer.setSample(channel, sample,
                         0.2 * std::sin(2.0 * M_PI * 660.0 * kTime));
      }
    }

    // Write the frame
    EXPECT_TRUE(writer.writeFrame(buffer));
  }

  EXPECT_TRUE(writer.close());
}