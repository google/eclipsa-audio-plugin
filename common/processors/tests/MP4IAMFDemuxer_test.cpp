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
#include "data_structures/src/FileExport.h"
#include "processors/tests/FileOutputTestUtils.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

using Layout = Speakers::AudioElementSpeakerLayout;

class MP4IAMFDemuxerTest : public FileOutputTests {
 public:
  MP4IAMFDemuxerTest() : demuxer() { muxSources = genMuxSources(); }

 protected:
  bool validateMuxedFile(const juce::String& path) {
#ifdef ECLIPSA_FFMPEG_AVAILABLE
    return validateMuxFFmpeg(path);
#else
    return true;
#endif
  }

  std::vector<std::filesystem::path> genMuxSources() {
    std::vector<std::filesystem::path> sources;
    for (const auto& codec : kTestSourceVideoCodecs) {
      for (const auto& container : kTestSourceVideoContainers) {
        const std::filesystem::path kNewSource =
            kTestSourceVideo.string() + "_" + codec + container;
        // We only add the source if the file exists. Some combinations cannot
        // exist (e.g., av1 in .mov)
        if (std::filesystem::exists(kNewSource)) {
          sources.push_back(kNewSource);
        }
      }
    }
    return sources;
  }

  MP4IAMFDemuxer demuxer;
  std::vector<std::filesystem::path> muxSources;
};

TEST_F(MP4IAMFDemuxerTest, mux_demux_iamf_1ae_cb) {
  for (const auto& source : muxSources) {
    for (const auto layout : kAudioElementLayouts) {
      const juce::Uuid kAE = addAudioElement(layout);
      const juce::Uuid kMP = addMixPresentation();
      addAudioElementsToMix(kMP, {kAE});

      setTestExportOpts({.codec = AudioCodec::LPCM,
                         .exportVideo = true,
                         .videoSource = source.string()});

      ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
      ASSERT_FALSE(std::filesystem::exists(videoOutPath));

      bounceAudio(fio_proc, audioElementRepository, ex.getSampleRate());

      ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
      ASSERT_TRUE(std::filesystem::exists(videoOutPath));
      ASSERT_TRUE(demuxer.verifyIAMFIntegrity(
          videoOutPath.string(), iamfOutPath.string(), kSampleRate, 16));
      ASSERT_TRUE(validateMuxedFile(videoOutPath.string()))
          << "Muxing validation failed for muxing source: " << source.string()
          << ", layout: " << layout.toString();

      std::filesystem::remove(iamfOutPath);
      std::filesystem::remove(videoOutPath);
      audioElementRepository.clear();
      mixRepository.clear();
    }
  }
}

TEST_F(MP4IAMFDemuxerTest, mux_demux_iamf_1ae_2mp) {
  for (const auto& source : muxSources) {
    for (const auto layout : kAudioElementLayouts) {
      const juce::Uuid kAE = addAudioElement(layout);
      const juce::Uuid kMP = addMixPresentation();
      const juce::Uuid kMP2 = addMixPresentation("Second Mix");
      addAudioElementsToMix(kMP, {kAE});
      addAudioElementsToMix(kMP2, {kAE});

      setTestExportOpts({.codec = AudioCodec::LPCM,
                         .exportVideo = true,
                         .videoSource = source.string()});

      ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
      ASSERT_FALSE(std::filesystem::exists(videoOutPath));

      bounceAudio(fio_proc, audioElementRepository, ex.getSampleRate());

      ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
      ASSERT_TRUE(std::filesystem::exists(videoOutPath));
      ASSERT_TRUE(demuxer.verifyIAMFIntegrity(
          videoOutPath.string(), iamfOutPath.string(), kSampleRate, 16));
      ASSERT_TRUE(validateMuxedFile(videoOutPath.string()))
          << "Muxing validation failed for muxing source: " << source.string()
          << ", layout: " << layout.toString();
      std::filesystem::remove(iamfOutPath);
      std::filesystem::remove(videoOutPath);
      audioElementRepository.clear();
      mixRepository.clear();
    }
  }
}

TEST_F(MP4IAMFDemuxerTest, mux_demux_iamf_2ae_cb) {
  for (const auto& source : muxSources) {
    const juce::Uuid kAE1 = addAudioElement(Speakers::kStereo);
    const juce::Uuid kAE2 = addAudioElement(Speakers::kExpl9Point1Point6Side);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE1, kAE2});

    setTestExportOpts({.codec = AudioCodec::LPCM,
                       .profile = FileProfile::BASE_ENHANCED,
                       .exportVideo = true,
                       .videoSource = source.string()});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
    ASSERT_FALSE(std::filesystem::exists(videoOutPath));

    bounceAudio(fio_proc, audioElementRepository, ex.getSampleRate());

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    ASSERT_TRUE(std::filesystem::exists(videoOutPath));

    EXPECT_TRUE(demuxer.verifyIAMFIntegrity(
        videoOutPath.string(), iamfOutPath.string(), ex.getSampleRate(), 16,
        SOUND_SYSTEM_A, 0.01f));
    ASSERT_TRUE(validateMuxedFile(videoOutPath.string()))
        << "Muxing validation failed for muxing source: " << source.string()
        << ", layout: " << Speakers::kStereo.toString() << " + "
        << Speakers::kExpl9Point1Point6Side.toString();
    std::filesystem::remove(iamfOutPath);
    std::filesystem::remove(videoOutPath);
  }
}

TEST_F(MP4IAMFDemuxerTest, e2e_iamf_1ae_cb) {
  const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM, .exportVideo = true});

  bounceAudio(fio_proc, audioElementRepository, ex.getSampleRate());

  EXPECT_TRUE(demuxer.verifyIAMFIntegrity(
      videoOutPath.string(), iamfOutPath.string(), ex.getSampleRate(), 16,
      SOUND_SYSTEM_A, 0.01f));
}

TEST_F(MP4IAMFDemuxerTest, e2e_iamf_1ae_sb) {
  const juce::Uuid kAE = addAudioElement(Speakers::kHOA1);
  const juce::Uuid kMP = addMixPresentation();
  addAudioElementsToMix(kMP, {kAE});

  setTestExportOpts({.codec = AudioCodec::LPCM, .exportVideo = true});

  bounceAudio(fio_proc, audioElementRepository, ex.getSampleRate());

  EXPECT_TRUE(demuxer.verifyIAMFIntegrity(
      videoOutPath.string(), iamfOutPath.string(), ex.getSampleRate(), 16,
      SOUND_SYSTEM_A, 0.01f));
}

TEST_F(MP4IAMFDemuxerTest, e2e_iamf_2ae_cb) {
  for (const auto& source : muxSources) {
    const juce::Uuid kAE1 = addAudioElement(Speakers::kStereo);
    const juce::Uuid kAE2 = addAudioElement(Speakers::kExplLFE);
    const juce::Uuid kMP = addMixPresentation();

    addAudioElementsToMix(kMP, {kAE1, kAE2});

    setTestExportOpts({.codec = AudioCodec::LPCM,
                       .exportVideo = true,
                       .videoSource = source.string(),
                       .profile = FileProfile::BASE_ENHANCED});

    ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
    ASSERT_FALSE(std::filesystem::exists(videoOutPath));

    bounceAudio(fio_proc, audioElementRepository, ex.getSampleRate());

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    ASSERT_TRUE(std::filesystem::exists(videoOutPath));

    EXPECT_TRUE(demuxer.verifyIAMFIntegrity(
        videoOutPath.string(), iamfOutPath.string(), ex.getSampleRate(), 16,
        SOUND_SYSTEM_A, 0.01f));
    std::filesystem::remove(iamfOutPath);
    std::filesystem::remove(videoOutPath);
  }
}

TEST_F(MP4IAMFDemuxerTest, e2e_iamf_all_layouts) {
  for (const Layout layout : kAudioElementLayouts) {
    const juce::Uuid kAE = addAudioElement(layout);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});

    setTestExportOpts(
        {.codec = AudioCodec::LPCM,
         .profile = (layout == Speakers::kMono || layout == Speakers::kStereo ||
                     layout == Speakers::kBinaural)
                        ? FileProfile::SIMPLE
                        : FileProfile::BASE_ENHANCED,
         .exportVideo = true});

    bounceAudio(fio_proc, audioElementRepository, ex.getSampleRate());

    ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
    ASSERT_TRUE(std::filesystem::exists(videoOutPath));

    EXPECT_TRUE(demuxer.verifyIAMFIntegrity(
        videoOutPath.string(), iamfOutPath.string(), ex.getSampleRate(), 16,
        SOUND_SYSTEM_A, 0.01f))
        << "Integrity failed for layout: " << layout.toString();

    std::filesystem::remove(iamfOutPath);
    std::filesystem::remove(videoOutPath);

    audioElementRepository.clear();
    mixRepository.clear();
    mixPresentationLoudnessRepository.clear();
  }
}

TEST_F(MP4IAMFDemuxerTest, e2e_iamf_codecs) {
  for (const auto& source : muxSources) {
    const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});

    for (const AudioCodec codec : {
             AudioCodec::LPCM,
             AudioCodec::FLAC,
             AudioCodec::OPUS,
         }) {
      setTestExportOpts({.codec = codec,
                         .exportVideo = true,
                         .videoSource = source.string(),
                         .sampleRate = (int)48e3});

      ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
      ASSERT_FALSE(std::filesystem::exists(videoOutPath));

      bounceAudio(fio_proc, audioElementRepository, ex.getSampleRate());

      ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
      ASSERT_TRUE(std::filesystem::exists(videoOutPath));

      EXPECT_TRUE(demuxer.verifyIAMFIntegrity(
          videoOutPath.string(), iamfOutPath.string(), ex.getSampleRate(), 16,
          SOUND_SYSTEM_A, 0.01f));

      std::filesystem::remove(iamfOutPath);
      std::filesystem::remove(videoOutPath);
    }
  }
}

TEST_F(MP4IAMFDemuxerTest, e2e_iamf_bit_depths) {
  for (const auto& source : muxSources) {
    const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});

    setTestExportOpts({.codec = AudioCodec::LPCM,
                       .exportVideo = true,
                       .videoSource = source.string()});

    for (int bitDepth : {16, 24, 32}) {
      bounceAudio(fio_proc, audioElementRepository, ex.getSampleRate());

      ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
      ASSERT_TRUE(std::filesystem::exists(videoOutPath));

      EXPECT_TRUE(demuxer.verifyIAMFIntegrity(
          videoOutPath.string(), iamfOutPath.string(), ex.getSampleRate(),
          bitDepth, SOUND_SYSTEM_A, 0.01f));

      std::filesystem::remove(iamfOutPath);
      std::filesystem::remove(videoOutPath);
    }
  }
}

TEST_F(MP4IAMFDemuxerTest, e2e_iamf_sample_rates) {
  for (const auto& source : muxSources) {
    const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});
    for (int sr : {44100, 48000, 96000}) {
      setTestExportOpts({.codec = AudioCodec::LPCM,
                         .sampleRate = sr,
                         .videoSource = source.string(),
                         .exportVideo = true});

      ASSERT_FALSE(std::filesystem::exists(iamfOutPath));
      ASSERT_FALSE(std::filesystem::exists(videoOutPath));

      bounceAudio(fio_proc, audioElementRepository, sr);

      ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
      ASSERT_TRUE(std::filesystem::exists(videoOutPath));

      EXPECT_TRUE(demuxer.verifyIAMFIntegrity(videoOutPath.string(),
                                              iamfOutPath.string(), sr, 16,
                                              SOUND_SYSTEM_A, 0.01f));

      std::filesystem::remove(iamfOutPath);
      std::filesystem::remove(videoOutPath);
    }
  }
}