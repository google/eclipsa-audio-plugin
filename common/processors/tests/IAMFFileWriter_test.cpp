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

class IAMFFileWriterAccessible : public IAMFFileWriter {
 public:
  IAMFFileWriterAccessible(
      FileExportRepository& fileExportRepository,
      AudioElementRepository& audioElementRepository,
      MixPresentationRepository& mixPresentationRepository,
      MixPresentationLoudnessRepository& mixPresentationLoudnessRepository,
      int samplesPerFrame, int sampleRate)
      : IAMFFileWriter(fileExportRepository, audioElementRepository,
                       mixPresentationRepository,
                       mixPresentationLoudnessRepository, samplesPerFrame,
                       sampleRate) {}

  void fetchUserMetadata(iamf_tools_cli_proto::UserMetadata& outMetadata) {
    outMetadata.CopyFrom(*userMetadata_);
  }
};

class IAMFFileWriterTest : public FileOutputTests {
 public:
  bool validateProfileSelection(
      iamf_tools_cli_proto::ProfileVersion expectedProfile) {
    iamf_tools_cli_proto::UserMetadata iamfMD;

    // Configure the profile to be the highest
    FileExport fileExport = fileExportRepository.get();
    fileExport.setProfile(FileProfile::BASE_ENHANCED);
    fileExportRepository.update(fileExport);

    // Create the IAMF file
    IAMFFileWriterAccessible writer(
        fileExportRepository, audioElementRepository, mixRepository,
        mixPresentationLoudnessRepository, kSamplesPerFrame, kSampleRate);
    EXPECT_TRUE(writer.open(iamfOutPath));
    EXPECT_TRUE(writer.close());

    // Validate the profile written is simple
    // I'd prefer to check the file directly here, but for now there is no way
    // to do this with the decoder
    writer.fetchUserMetadata(iamfMD);
    EXPECT_TRUE(iamfMD.ia_sequence_header_metadata(0).primary_profile() ==
                expectedProfile);
    EXPECT_TRUE(iamfMD.ia_sequence_header_metadata(0).additional_profile() ==
                expectedProfile);
  }
};

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

TEST_F(IAMFFileWriterTest, validate_simple_profile_selection) {
  // Simple profile for single audio element
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});
  validateProfileSelection(iamf_tools_cli_proto::PROFILE_VERSION_SIMPLE);
}

TEST_F(IAMFFileWriterTest, validate_base_profile_selection) {
  // Base profile with 2 audio elements
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kAE2 = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE, kAE2});

  validateProfileSelection(iamf_tools_cli_proto::PROFILE_VERSION_BASE);
}

TEST_F(IAMFFileWriterTest, validate_expanded_profile_selection) {
  // Expanded profile required for 3 audio elements
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kAE2 = addAudioElement(Speakers::kStereo);
  const juce::Uuid kAE3 = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE, kAE2, kAE3});
  validateProfileSelection(iamf_tools_cli_proto::PROFILE_VERSION_BASE_ENHANCED);
}

TEST_F(IAMFFileWriterTest, validate_expanded_element_profile_selection) {
  // Exapnded profile required for expanded audio element type
  const juce::Uuid kAE = addAudioElement(Speakers::kExpl7Point1Point4Front);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});
  validateProfileSelection(iamf_tools_cli_proto::PROFILE_VERSION_BASE_ENHANCED);
}