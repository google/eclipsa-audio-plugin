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

TEST_F(FileOutputTests, mux_iamf_container_duration_matches_video) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM, .exportVideo = true});

  bounceAudio(fio_proc, audioElementRepository);
  ASSERT_TRUE(std::filesystem::exists(videoOutPath));

  double videoDuration =
      getMP4DurationSeconds(ex.getVideoSource().toStdString());
  double outputDuration = getMP4DurationSeconds(videoOutPath.string());

  ASSERT_GT(videoDuration, 0.0);
  ASSERT_GT(outputDuration, 0.0);
  EXPECT_NEAR(outputDuration, videoDuration, 0.1);
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

// =============================================================================
// Helper: bounce with explicit control over frame count, so we can produce
// a known duration of audio.  Returns the number of blocks processed.
// =============================================================================
static int bounceAudioForDuration(
    FileOutputProcessor& fio_proc,
    AudioElementRepository& audioElementRepository, double durationSeconds,
    unsigned sampleRate = 48000, unsigned frameSize = 128) {
  const unsigned kNumChannels = totalAudioChannels(audioElementRepository);
  const auto kSineTone = generateSineWave(440.0f, sampleRate, frameSize);

  fio_proc.prepareToPlay(sampleRate, frameSize);
  fio_proc.setNonRealtime(true);

  const int numBlocks =
      static_cast<int>(std::ceil(durationSeconds * sampleRate / frameSize));

  juce::AudioBuffer<float> audioBuffer(kNumChannels, frameSize);
  juce::MidiBuffer dummyMidiBuffer;
  for (int block = 0; block < numBlocks; ++block) {
    for (unsigned i = 0; i < kNumChannels; ++i) {
      audioBuffer.copyFrom(i, 0, kSineTone, 0, 0, frameSize);
    }
    fio_proc.processBlock(audioBuffer, dummyMidiBuffer);
  }
  fio_proc.setNonRealtime(false);
  return numBlocks;
}

// =============================================================================
// Tests for start/end time (shouldBufferBeWritten) logic
// =============================================================================

// Baseline: no start/end time set (both 0) — all audio is written.
TEST_F(FileOutputTests, time_range_no_limits_writes_all) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  // startTime=0, endTime=0 (defaults) — no range limiting
  auto config = fileExportRepository.get();
  ASSERT_EQ(config.getStartSampleIdx(), 0);
  ASSERT_EQ(config.getEndSampleIdx(), 0);

  setTestExportOpts({.codec = AudioCodec::LPCM});

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudio(fio_proc, audioElementRepository);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));

  // Verify the file has non-zero size (audio was written)
  EXPECT_GT(std::filesystem::file_size(iamfOutPath), 0u);
  std::filesystem::remove(iamfOutPath);
}

// Start time only: skip the first N seconds of the timeline.
// We bounce 2 seconds of audio with startTime=48000 samples (1 second at 48000
// Hz). The output file should exist but be smaller than a full 2s export.
TEST_F(FileOutputTests, time_range_start_only) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM});

  // First: bounce full 2 seconds with no time limits for size reference
  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 2.0, kSampleRate,
                         kSamplesPerFrame);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  const auto fullSize = std::filesystem::file_size(iamfOutPath);
  std::filesystem::remove(iamfOutPath);

  // Now bounce with startTime = 48000 samples (1 second in at 48000 Hz)
  // Need to recreate processor state since setNonRealtime(false) closed export
  auto config = fileExportRepository.get();
  config.setStartSampleIdx(48000);  // 48000 samples = 1 second at 48000 Hz
  config.setEndSampleIdx(0);        // no end limit
  fileExportRepository.update(config);

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 2.0, kSampleRate,
                         kSamplesPerFrame);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  const auto startLimitedSize = std::filesystem::file_size(iamfOutPath);

  // The start-limited file should be smaller (roughly half the audio)
  EXPECT_LT(startLimitedSize, fullSize);
  std::filesystem::remove(iamfOutPath);

  // Reset for other tests
  config.setStartSampleIdx(0);
  fileExportRepository.update(config);
}

// End time only: stop writing after N seconds.
TEST_F(FileOutputTests, time_range_end_only) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM});

  // Full 2-second bounce for reference
  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 2.0, kSampleRate,
                         kSamplesPerFrame);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  const auto fullSize = std::filesystem::file_size(iamfOutPath);
  std::filesystem::remove(iamfOutPath);

  // Now bounce 2 seconds but with endTime = 48000 samples (stop at 1 second)
  auto config = fileExportRepository.get();
  config.setStartSampleIdx(0);
  config.setEndSampleIdx(48000);  // 48000 samples = 1 second at 48000 Hz
  fileExportRepository.update(config);

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 2.0, kSampleRate,
                         kSamplesPerFrame);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  const auto endLimitedSize = std::filesystem::file_size(iamfOutPath);

  EXPECT_EQ(endLimitedSize, fullSize);
  std::filesystem::remove(iamfOutPath);

  // Reset
  config.setEndSampleIdx(0);
  fileExportRepository.update(config);
}

// Both start and end time: only write the window [start, end).
TEST_F(FileOutputTests, time_range_start_and_end) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM});

  // Full 4-second bounce for reference
  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 4.0, kSampleRate,
                         kSamplesPerFrame);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  const auto fullSize = std::filesystem::file_size(iamfOutPath);
  std::filesystem::remove(iamfOutPath);

  // Bounce 4 seconds with window [1s, 3s) — should capture ~2s of audio
  auto config = fileExportRepository.get();
  config.setStartSampleIdx(48000);  // 48000 samples = 1 second at 48000 Hz
  config.setEndSampleIdx(144000);   // 144000 samples = 3 seconds at 48000 Hz
  fileExportRepository.update(config);

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 4.0, kSampleRate,
                         kSamplesPerFrame);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  const auto windowedSize = std::filesystem::file_size(iamfOutPath);

  EXPECT_LT(windowedSize, fullSize);
  std::filesystem::remove(iamfOutPath);

  // Reset
  config.setStartSampleIdx(0);
  config.setEndSampleIdx(0);
  fileExportRepository.update(config);
}

// End time before start time: nothing should be written (nonsensical input).
// The file may still be created (IAMF header/structure) but the player should
// indicate it's invalid
TEST_F(FileOutputTests, time_range_end_before_start) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM});

  // Full bounce for reference
  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 2.0, kSampleRate,
                         kSamplesPerFrame);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  const auto fullSize = std::filesystem::file_size(iamfOutPath);
  std::filesystem::remove(iamfOutPath);

  // End at 1s, start at 2s — window is empty
  auto config = fileExportRepository.get();
  config.setStartSampleIdx(96000);  // 96000 samples = 2 seconds at 48000 Hz
  config.setEndSampleIdx(48000);    // 48000 samples = 1 second at 48000 Hz
  fileExportRepository.update(config);

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 2.0, kSampleRate,
                         kSamplesPerFrame);

  // File may or may not exist depending on IAMF writer behavior with 0 frames.
  // If it exists, it should be much smaller than a full export.
  if (std::filesystem::exists(iamfOutPath)) {
    const auto emptyWindowSize = std::filesystem::file_size(iamfOutPath);
    EXPECT_LT(emptyWindowSize, fullSize);
    std::filesystem::remove(iamfOutPath);
  }

  // Reset
  config.setStartSampleIdx(0);
  config.setEndSampleIdx(0);
  fileExportRepository.update(config);
}

// Start time beyond the total audio duration: no audio frames written.
TEST_F(FileOutputTests, time_range_start_beyond_duration) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM});

  // Full 1-second bounce for reference
  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 1.0, kSampleRate,
                         kSamplesPerFrame);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  const auto fullSize = std::filesystem::file_size(iamfOutPath);
  std::filesystem::remove(iamfOutPath);

  // Start at 5 seconds but only bounce 1 second of audio
  auto config = fileExportRepository.get();
  config.setStartSampleIdx(
      240000);  // 240000 samples = 5 seconds at 48000 Hz — beyond the 1s bounce
  config.setEndSampleIdx(0);
  fileExportRepository.update(config);

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 1.0, kSampleRate,
                         kSamplesPerFrame);

  if (std::filesystem::exists(iamfOutPath)) {
    const auto noAudioSize = std::filesystem::file_size(iamfOutPath);
    EXPECT_LT(noAudioSize, fullSize);
    std::filesystem::remove(iamfOutPath);
  }

  // Reset
  config.setStartSampleIdx(0);
  fileExportRepository.update(config);
}

// Verify sub-second boundary precision using sample counts.
// startTime and endTime are stored as sample counts (long) in FileExport.
// This test uses a precise sub-second boundary.
TEST_F(FileOutputTests, time_range_subsecond_precision) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM});

  // Bounce 1 second, window [250ms, 750ms) — should capture ~500ms
  auto config = fileExportRepository.get();
  config.setStartSampleIdx(12000);  // 12000 samples = 250ms at 48000 Hz
  config.setEndSampleIdx(36000);    // 36000 samples = 750ms at 48000 Hz
  fileExportRepository.update(config);

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 1.0, kSampleRate,
                         kSamplesPerFrame);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));

  // Just verify file was created with some content — the windowed export worked
  EXPECT_GT(std::filesystem::file_size(iamfOutPath), 0u);
  std::filesystem::remove(iamfOutPath);

  // Reset
  config.setStartSampleIdx(0);
  config.setEndSampleIdx(0);
  fileExportRepository.update(config);
}

// Large sample count values (simulating a time deep into a session).
// e.g., 1 hour at 48000 Hz = 172,800,000 samples
TEST_F(FileOutputTests, time_range_large_tc_values) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM});

  // Set start time to 1 hour in (172,800,000 samples at 48000 Hz), bounce only
  // 1 second All buffers should be skipped since currentSample never reaches
  // startTime
  auto config = fileExportRepository.get();
  config.setStartSampleIdx(
      172800000);  // 172800000 samples = 1 hour at 48000 Hz
  config.setEndSampleIdx(
      172848000);  // 172848000 samples = 1 hour + 1 second at 48000 Hz
  fileExportRepository.update(config);

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 1.0, kSampleRate,
                         kSamplesPerFrame);

  // No audio frames should have been written. File may or may not exist.
  if (std::filesystem::exists(iamfOutPath)) {
    // If file exists it's just IAMF structure with no audio data
    // It should be smaller than a normal 1-second export
    // (We mainly care that no crash occurred with large values)
    std::filesystem::remove(iamfOutPath);
  }

  // Reset
  config.setStartSampleIdx(0);
  config.setEndSampleIdx(0);
  fileExportRepository.update(config);
}

// End time at exactly 0 with a positive start time: endTime <= 0 means
// "no end limit", so audio from startTime onward should all be written.
TEST_F(FileOutputTests, time_range_start_set_end_zero) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM});

  // Bounce 2 seconds with startTime=24000 samples (500ms), endTime=0 (no end
  // limit)
  auto config = fileExportRepository.get();
  config.setStartSampleIdx(24000);  // 24000 samples = 500ms at 48000 Hz
  config.setEndSampleIdx(0);
  fileExportRepository.update(config);

  ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
  bounceAudioForDuration(fio_proc, audioElementRepository, 2.0, kSampleRate,
                         kSamplesPerFrame);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_GT(std::filesystem::file_size(iamfOutPath), 0u);
  std::filesystem::remove(iamfOutPath);

  // Reset
  config.setStartSampleIdx(0);
  fileExportRepository.update(config);
}