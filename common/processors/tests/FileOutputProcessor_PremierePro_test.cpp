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

#include <gtest/gtest.h>

#include <filesystem>

#include "FileOutputTestFixture.h"
#include "juce_cryptography/juce_cryptography.h"
#include "processors/tests/FileOutputTestUtils.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

TEST_F(FileOutputTests, iamf_pp_lpc_1ae_1mp) {
  for (const Layout layout : kAudioElementLayouts) {
    const juce::Uuid kAE = addAudioElement(layout);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});

    setTestExportOpts({AudioCodec::LPCM});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bouncePremiereProAudio(fileExportRepository, audioElementRepository,
                           mixRepository, mixPresentationLoudnessRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_pp_lpc_1ae_1mp_expl) {
  for (const Layout layout : kAudioElementExpandedLayouts) {
    const juce::Uuid kAE = addAudioElement(layout);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});

    setTestExportOpts(
        {AudioCodec::LPCM, .profile = FileProfile::BASE_ENHANCED});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
    bouncePremiereProAudio(fileExportRepository, audioElementRepository,
                           mixRepository, mixPresentationLoudnessRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_pp_lpc_2ae_1mp) {
  const Layout kLayout1 = Speakers::kStereo;
  const Layout kLayout2 = Speakers::kHOA2;
  const juce::Uuid kAE1 = addAudioElement(kLayout1);
  const juce::Uuid kAE2 = addAudioElement(kLayout2);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE1, kAE2});

  setTestExportOpts({AudioCodec::LPCM});

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

  bouncePremiereProAudio(fileExportRepository, audioElementRepository,
                         mixRepository, mixPresentationLoudnessRepository);

  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  std::filesystem::remove(iamfOutPath);  // Rm for next iteration
}

TEST_F(FileOutputTests, iamf_pp_lpc_2ae_expl_1mp) {
  const Layout kLayout1 = Speakers::kStereo;
  const Layout kLayout2 = Speakers::kExplLFE;
  const juce::Uuid kAE1 = addAudioElement(kLayout1);
  const juce::Uuid kAE2 = addAudioElement(kLayout2);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE1, kAE2});

  setTestExportOpts({AudioCodec::LPCM, .profile = FileProfile::BASE_ENHANCED});

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

  bounceAudio(fio_proc, audioElementRepository);

  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  std::filesystem::remove(iamfOutPath);  // Rm for next iteration
}

TEST_F(FileOutputTests, pp_validate_file_checksum) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({AudioCodec::LPCM});

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

  bouncePremiereProAudio(fileExportRepository, audioElementRepository,
                         mixRepository, mixPresentationLoudnessRepository);

  // Verify the file exists and generate checksum
  juce::File iamfFile(iamfOutPath.string());
  ASSERT_TRUE(iamfFile.existsAsFile());

  std::unique_ptr<juce::FileInputStream> fileStream(
      iamfFile.createInputStream());
  juce::MemoryBlock fileData;
  fileData.setSize(iamfFile.getSize());
  fileStream->read(fileData.getData(), fileData.getSize());

  // Generate checksum for the exported file
  juce::SHA256 newChecksum(fileData.getData(), fileData.getSize());
  const juce::String kNewChecksumString = newChecksum.toHexString();

  // Select reference checksum file based on build type
  const char* kReferenceFile =
#ifdef NDEBUG
      "HashSourceFileRelease.iamf";
#else
      "HashSourceFileDebug.iamf";
#endif

  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path().parent_path() /
      "common/processors/tests/test_resources" / kReferenceFile;

  const juce::File kReferenceChecksumFile(kReferenceFilePath.string());
  ASSERT_TRUE(kReferenceChecksumFile.existsAsFile());

  juce::MemoryBlock referenceData;
  kReferenceChecksumFile.loadFileAsData(referenceData);

  const juce::SHA256 kReferenceChecksum(referenceData.getData(),
                                        referenceData.getSize());
  const juce::String kReferenceChecksumString =
      kReferenceChecksum.toHexString();

  // Compare the checksums
  EXPECT_EQ(kNewChecksumString, kReferenceChecksumString);

  std::filesystem::remove(iamfOutPath);
}