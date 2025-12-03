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

TEST_F(FileOutputTests, iamf_lpc_1ae_1mp) {
  for (const Layout layout : kAudioElementLayouts) {
    const juce::Uuid kAE = addAudioElement(layout);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});

    setTestExportOpts({.codec = AudioCodec::LPCM});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_lpc_1ae_1mp_expl) {
  for (const Layout layout : kAudioElementExpandedLayouts) {
    const juce::Uuid kAE = addAudioElement(layout);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});

    setTestExportOpts(
        {.codec = AudioCodec::LPCM, .profile = FileProfile::BASE_ENHANCED});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_lpc_2ae_1mp) {
  const Layout kLayout1 = Speakers::kStereo;
  const Layout kLayout2 = Speakers::kHOA2;
  const juce::Uuid kAE1 = addAudioElement(kLayout1);
  const juce::Uuid kAE2 = addAudioElement(kLayout2);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE1, kAE2});

  setTestExportOpts({.codec = AudioCodec::LPCM});

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

  bounceAudio(fio_proc, audioElementRepository);

  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  std::filesystem::remove(iamfOutPath);  // Rm for next iteration
}

TEST_F(FileOutputTests, iamf_lpc_2ae_expl_1mp) {
  const Layout kLayout1 = Speakers::kStereo;
  const Layout kLayout2 = Speakers::kExplLFE;
  const juce::Uuid kAE1 = addAudioElement(kLayout1);
  const juce::Uuid kAE2 = addAudioElement(kLayout2);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE1, kAE2});

  setTestExportOpts(
      {.codec = AudioCodec::LPCM, .profile = FileProfile::BASE_ENHANCED});

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

  bounceAudio(fio_proc, audioElementRepository);

  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  std::filesystem::remove(iamfOutPath);  // Rm for next iteration
}

TEST_F(FileOutputTests, iamf_lpc_1ae_2mp) {
  for (const Layout layout : kAudioElementLayouts) {
    const juce::Uuid kAE = addAudioElement(layout);
    const juce::Uuid kMP1 = addMixPresentation();
    const juce::Uuid kMP2 = addMixPresentation();
    addAudioElementsToMix(kMP1, {kAE});
    addAudioElementsToMix(kMP2, {kAE});

    setTestExportOpts({.codec = AudioCodec::LPCM});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_lpc_2ae_2mp) {
  for (const Layout layout : kAudioElementLayouts) {
    const juce::Uuid kAE1 = addAudioElement(layout);
    const juce::Uuid kAE2 = addAudioElement(Speakers::kStereo);
    const juce::Uuid kMP1 = addMixPresentation();
    const juce::Uuid kMP2 = addMixPresentation();
    addAudioElementsToMix(kMP1, {kAE1, kAE2});
    addAudioElementsToMix(kMP2, {kAE1, kAE2});

    setTestExportOpts(
        {.codec = AudioCodec::LPCM, .profile = FileProfile::BASE_ENHANCED});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_lpc_28ae_1mp) {
  std::vector<juce::Uuid> aeIds;
  for (int i = 0; i < 28; ++i) {
    aeIds.push_back(addAudioElement(Speakers::kMono));
  }
  const juce::Uuid kMP1 = addMixPresentation();
  addAudioElementsToMix(kMP1, aeIds);

  setTestExportOpts(
      {.codec = AudioCodec::LPCM, .profile = FileProfile::BASE_ENHANCED});

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

  bounceAudio(fio_proc, audioElementRepository);

  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  std::filesystem::remove(iamfOutPath);  // Rm for next iteration
}

TEST_F(FileOutputTests, iamf_multi_codec_multi_sr_1ae_1mp) {
  const juce::Uuid kAE = addAudioElement(Speakers::k7Point1Point4);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});
  for (const AudioCodec codec :
       {AudioCodec::LPCM, AudioCodec::FLAC, AudioCodec::OPUS}) {
    for (const int sampleRate : {16e3, 44.1e3, 48e3, 96e3}) {
      if (codec == AudioCodec::OPUS &&
          (sampleRate == 44.1e3 || sampleRate == 96e3)) {
        continue;  // Opus does not support 44.1kHz and 96kHz
      }
      setTestExportOpts({.codec = codec, .sampleRate = sampleRate});

      ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

      bounceAudio(fio_proc, audioElementRepository, sampleRate);

      ASSERT_TRUE(std::filesystem::exists(iamfOutPath))
          << sampleRate << ":" << static_cast<int>(codec);
      std::filesystem::remove(iamfOutPath);  // Rm for next iteration
    }
  }
}

TEST_F(FileOutputTests, iamf_flac_1ae_1mp) {
  for (const Layout layout : kAudioElementLayouts) {
    const juce::Uuid kAE = addAudioElement(layout);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});

    setTestExportOpts({.codec = AudioCodec::FLAC});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_opus_1ae_1mp) {
  for (const Layout layout : kAudioElementLayouts) {
    const juce::Uuid kAE = addAudioElement(layout);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});

    setTestExportOpts({.codec = AudioCodec::OPUS});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_flac_2ae_1mp) {
  for (const Layout layout : kAudioElementLayouts) {
    const juce::Uuid kAE1 = addAudioElement(layout);
    const juce::Uuid kAE2 = addAudioElement(Speakers::kStereo);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE1, kAE2});

    setTestExportOpts(
        {.codec = AudioCodec::FLAC, .profile = FileProfile::BASE_ENHANCED});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_opus_2ae_1mp) {
  for (const Layout layout : kAudioElementLayouts) {
    const juce::Uuid kAE1 = addAudioElement(layout);
    const juce::Uuid kAE2 = addAudioElement(Speakers::kStereo);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE1, kAE2});

    setTestExportOpts(
        {.codec = AudioCodec::OPUS, .profile = FileProfile::BASE_ENHANCED});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, mux_iamf_lpc_1ae_1mp) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM, .exportVideo = true});

  EXPECT_FALSE(std::filesystem::exists(iamfOutPath));
  EXPECT_FALSE(std::filesystem::exists(videoOutPath));

  bounceAudio(fio_proc, audioElementRepository);

  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_TRUE(std::filesystem::exists(videoOutPath));
}

TEST_F(FileOutputTests, mux_iamf_flac_2ae_1mp) {
  const juce::Uuid kAE1 = addAudioElement(Speakers::kStereo);
  const juce::Uuid kAE2 = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE1, kAE2});

  setTestExportOpts({.codec = AudioCodec::FLAC, .exportVideo = true});

  EXPECT_FALSE(std::filesystem::exists(iamfOutPath));
  EXPECT_FALSE(std::filesystem::exists(videoOutPath));

  bounceAudio(fio_proc, audioElementRepository);

  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_TRUE(std::filesystem::exists(videoOutPath));
}

TEST_F(FileOutputTests, mux_iamf_opus_2ae_2mp) {
  const juce::Uuid kAE1 = addAudioElement(Speakers::kStereo);
  const juce::Uuid kAE2 = addAudioElement(Speakers::kHOA3);
  const juce::Uuid kMP1 = addMixPresentation();
  const juce::Uuid kMP2 = addMixPresentation();
  addAudioElementsToMix(kMP1, {kAE1, kAE2});
  addAudioElementsToMix(kMP2, {kAE1, kAE2});

  setTestExportOpts({.codec = AudioCodec::OPUS, .exportVideo = true});

  EXPECT_FALSE(std::filesystem::exists(iamfOutPath));
  EXPECT_FALSE(std::filesystem::exists(videoOutPath));

  bounceAudio(fio_proc, audioElementRepository);

  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_TRUE(std::filesystem::exists(videoOutPath));
}

// Codec param tests. These tests focus on testing advanced codec specific file
// export configurations. As such, the configuration is kept local to the tests
// rather than being done through the generic `setTestExportOpts` function.
TEST_F(FileOutputTests, iamf_lpc_custom_param) {
  const juce::Uuid kAE = addAudioElement(Speakers::k5Point1Point4);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  auto config = fileExportRepository.get();
  config.setAudioCodec(AudioCodec::LPCM);
  for (int i = 16; i <= 32; i += 8) {
    config.setLPCMSampleSize(i);
    fileExportRepository.update(config);

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));

    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_opus_custom_param) {
  const juce::Uuid kAE = addAudioElement(Speakers::k5Point1Point4);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  auto config = fileExportRepository.get();
  config.setAudioCodec(AudioCodec::OPUS);
  for (int i = 6000; i < 256000; i += 1000) {
    config.setOpusTotalBitrate(i);
    fileExportRepository.update(config);

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, iamf_flac_custom_param) {
  const juce::Uuid kAE = addAudioElement(Speakers::k5Point1Point4);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  auto config = fileExportRepository.get();
  config.setAudioCodec(AudioCodec::FLAC);
  for (int i = 0; i < 16; ++i) {
    config.setFlacCompressionLevel(i);
    fileExportRepository.update(config);

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

    bounceAudio(fio_proc, audioElementRepository);

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    std::filesystem::remove(iamfOutPath);  // Rm for next iteration
  }
}

TEST_F(FileOutputTests, validate_file_checksum) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({AudioCodec::LPCM});

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));

  bounceAudio(fio_proc, audioElementRepository);

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

// Test with an invalid IAMF path
TEST_F(FileOutputTests, iamf_invalid_path) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  const std::filesystem::path kInvalidIamfPath = "/invalid_path/test.iamf";

  FileExport config = fileExportRepository.get();
  config.setExportFile(kInvalidIamfPath.string());
  fileExportRepository.update(config);

  EXPECT_FALSE(std::filesystem::exists(kInvalidIamfPath));

  bounceAudio(fio_proc, audioElementRepository);

  EXPECT_FALSE(std::filesystem::exists(kInvalidIamfPath));
}

// Test with an invalid video source path
TEST_F(FileOutputTests, mux_iamf_invalid_vin_path) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  const std::filesystem::path kInvalidVinPath = "/invalid_path/source.mp4";

  FileExport config = fileExportRepository.get();
  config.setVideoSource(kInvalidVinPath.string());
  config.setExportVideo(true);
  fileExportRepository.update(config);

  EXPECT_FALSE(std::filesystem::exists(kInvalidVinPath));
  EXPECT_FALSE(std::filesystem::exists(iamfOutPath));

  bounceAudio(fio_proc, audioElementRepository);

  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_FALSE(std::filesystem::exists(videoOutPath));
}

// Test with an invalid video output path
TEST_F(FileOutputTests, mux_iamf_invalid_vout_path) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  const std::filesystem::path kInvalidVoutPath = "/invalid_path/muxed.mp4";

  FileExport config = fileExportRepository.get();
  config.setVideoExportFolder(kInvalidVoutPath.string());
  config.setExportVideo(true);
  fileExportRepository.update(config);

  EXPECT_FALSE(std::filesystem::exists(iamfOutPath));
  EXPECT_FALSE(std::filesystem::exists(kInvalidVoutPath));

  bounceAudio(fio_proc, audioElementRepository);

  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_FALSE(std::filesystem::exists(videoOutPath));
}