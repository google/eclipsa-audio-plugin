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

#pragma once

#include <gtest/gtest.h>
#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>

#include "../common/data_repository/implementation/AudioElementRepository.h"
#include "../common/data_repository/implementation/FileExportRepository.h"
#include "../common/data_repository/implementation/MixPresentationLoudnessRepository.h"
#include "../common/data_repository/implementation/MixPresentationRepository.h"
#include "../common/data_structures/src/LanguageCodeMetaData.h"
#include "../common/data_structures/src/MixPresentationLoudness.h"
#include "../common/substream_rdr/substream_rdr_utils/Speakers.h"
#include "../file_output/FileOutputProcessor.h"
#include "data_structures/src/FileExport.h"
#include "processors/tests/FileOutputTestUtils.h"

// Test repositories
class TestFileExportRepository : public FileExportRepository {
 public:
  TestFileExportRepository() : FileExportRepository(juce::ValueTree{"test"}) {}
};

class TestAudioElementRepository : public AudioElementRepository {
 public:
  TestAudioElementRepository()
      : AudioElementRepository(juce::ValueTree{"test"}) {}
};

class TestMixPresentationRepository : public MixPresentationRepository {
 public:
  TestMixPresentationRepository()
      : MixPresentationRepository(juce::ValueTree{"test"}) {}
};

class TestMixPresentationLoudnessRepository
    : public MixPresentationLoudnessRepository {
 public:
  TestMixPresentationLoudnessRepository()
      : MixPresentationLoudnessRepository(juce::ValueTree{"test"}) {}
};

class FileOutputTests : public ::testing::Test {
 public:
  using Layout = Speakers::AudioElementSpeakerLayout;

  // Constants
  static const int kSampleRate = 16e3;
  static const int kSamplesPerFrame = 128;

 protected:
  FileOutputTests()
      : ex(fileExportRepository.get()),
        fio_proc(fileExportRepository, audioElementRepository, mixRepository,
                 mixPresentationLoudnessRepository) {
    // Configure basic audio export data
    ex.setExportAudio(true);
    ex.setAudioFileFormat(AudioFileFormat::IAMF);
    ex.setSampleRate(kSampleRate);
    ex.setVideoSource({std::filesystem::current_path().parent_path() /
                       "common/processors/tests/test_resources" /
                       "SilentSampleVideo.mp4"});
    iamfOutPath = std::filesystem::current_path() / "test.iamf";
    videoOutPath = std::filesystem::current_path() / "test.mp4";
    ex.setVideoExportFolder(videoOutPath.string());
    ex.setExportFolder(std::filesystem::current_path().string());
    ex.setExportFile(iamfOutPath.string());
    fileExportRepository.update(ex);
  }

  ~FileOutputTests() override {
    // Clean up produced .wav, .iamf, .mp4, and log files
    for (const auto& entry :
         std::filesystem::directory_iterator(std::filesystem::current_path())) {
      if (entry.path().extension() == ".wav" ||
          entry.path().extension() == ".mp4") {
        std::filesystem::remove(entry.path());
      }
    }
    std::filesystem::remove(iamfOutPath);
  }

  /**
   * @brief Creates and adds an audio element with the given layout
   *
   * @param layout The layout of the audio element
   * @param name The name of the audio element
   * @param position The position of the audio element
   * @return juce::Uuid The ID of the created audio element
   */
  juce::Uuid addAudioElement(const Speakers::AudioElementSpeakerLayout& layout,
                             const juce::String& name = "", int position = 0) {
    AudioElement ae(juce::Uuid(), name.isEmpty() ? "Audio Element" : name,
                    layout.toString(), layout, position);
    audioElementRepository.add(ae);
    return ae.getId();
  }

  /**
   * @brief Creates and adds a mix presentation with given parameters
   *
   * @param name The name of the mix presentation
   * @param gain The gain of the mix presentation
   * @param lang The language of the mix presentation
   * @return juce::Uuid The ID of the created mix presentation
   */
  juce::Uuid addMixPresentation(
      const juce::String& name = "", float gain = 1.0f,
      LanguageData::MixLanguages lang = LanguageData::MixLanguages::English) {
    const juce::Uuid id = juce::Uuid();
    MixPresentation mp(id, name.isEmpty() ? "Mix Presentation" : name, gain,
                       lang, {});
    MixPresentationLoudness mixLoudness(id);
    mixRepository.add(mp);
    mixPresentationLoudnessRepository.add(mixLoudness);
    return id;
  }

  // Adds audio elements to an existing mix presentation
  // Updates the largest layout if needed based on the audio elements
  void addAudioElementsToMix(const juce::Uuid& mixId,
                             const std::vector<juce::Uuid>& elementIds,
                             float gain = 1.0f) {
    MixPresentation mp = mixRepository.get(mixId).value();
    MixPresentationLoudness mixLoudness =
        mixPresentationLoudnessRepository.get(mixId).value();

    Speakers::AudioElementSpeakerLayout largestLayout = Speakers::kStereo;

    for (const auto& elementId : elementIds) {
      const auto& ae = audioElementRepository.get(elementId).value();
      mp.addAudioElement(elementId, gain, ae.getName());

      // Get the layout from the audio element
      const auto& aeLayout = ae.getChannelConfig();
      if (!aeLayout.isAmbisonics() && aeLayout != Speakers::kBinaural &&
          aeLayout.getNumChannels() > largestLayout.getNumChannels()) {
        largestLayout = aeLayout;
      }
    }

    // Only update if we found a larger layout
    if (largestLayout.getNumChannels() > 2) {
      mixLoudness.replaceLargestLayout(largestLayout);
    }

    mixRepository.update(mp);
    mixPresentationLoudnessRepository.update(mixLoudness);
  }

  struct ExportTestOpts {
    AudioCodec codec = AudioCodec::LPCM;
    std::optional<FileProfile> profile = std::nullopt;
    int sampleRate = 48000;
    bool exportVideo = false;
  };

  void setTestExportOpts(const ExportTestOpts opts) {
    const FileProfile resolvedProfile =
        opts.profile.has_value() ? *opts.profile
                                 : profileFromAEs(audioElementRepository);
    ex.setProfile(resolvedProfile);
    ex.setAudioCodec(opts.codec);
    ex.setSampleRate(opts.sampleRate);
    ex.setExportVideo(opts.exportVideo);
    fileExportRepository.update(ex);
  }

  // Constants
  std::vector<Speakers::AudioElementSpeakerLayout> kAudioElementLayouts = {
      Speakers::kMono,          Speakers::kStereo,
      Speakers::k5Point1,       Speakers::k5Point1Point2,
      Speakers::k5Point1Point4, Speakers::k7Point1,
      Speakers::k7Point1Point2, Speakers::k7Point1Point4,
      Speakers::k3Point1Point2, Speakers::kBinaural,
      Speakers::kHOA1,          Speakers::kHOA2,
      Speakers::kHOA3,
  };
  std::vector<Speakers::AudioElementSpeakerLayout>
      kAudioElementExpandedLayouts = {
          Speakers::kExplLFE,
          Speakers::kExpl5Point1Point4Surround,
          Speakers::kExpl7Point1Point4SideSurround,
          Speakers::kExpl7Point1Point4RearSurround,
          Speakers::kExpl7Point1Point4TopFront,
          Speakers::kExpl7Point1Point4TopBack,
          Speakers::kExpl7Point1Point4Top,
          Speakers::kExpl7Point1Point4Front,
          Speakers::kExpl9Point1Point6,
          Speakers::kExpl9Point1Point6Front,
          Speakers::kExpl9Point1Point6Side,
          Speakers::kExpl9Point1Point6TopSide,
          Speakers::kExpl9Point1Point6Top,
      };

  // Repositories
  juce::ValueTree testState{"test_state"};
  FileExportRepository fileExportRepository{
      testState.getOrCreateChildWithName("file", nullptr)};
  AudioElementRepository audioElementRepository{
      testState.getOrCreateChildWithName("element", nullptr)};
  MixPresentationRepository mixRepository{
      testState.getOrCreateChildWithName("mix", nullptr)};
  MixPresentationLoudnessRepository mixPresentationLoudnessRepository{
      testState.getOrCreateChildWithName("mixLoud", nullptr)};

  std::filesystem::path iamfOutPath;
  std::filesystem::path videoOutPath;
  FileExport ex;
  FileOutputProcessor fio_proc;
};
