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
  static constexpr int kSampleRate = 16e3;
  static constexpr int kSamplesPerFrame = 128;

  // Create an IAMF file at a specified path with basic content
  bool createBasicIAMFFile(const std::filesystem::path& path) {
    const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});

    setTestExportOpts({.codec = AudioCodec::LPCM, .sampleRate = kSampleRate});
    auto FileExport = fileExportRepository.get();
    FileExport.setExportFile(path.string());
    fileExportRepository.update(FileExport);

    bounceAudio(fio_proc, audioElementRepository);
    return true;
  }

  void createIAMFFile2AE2MP(const std::filesystem::path& path) {
    const Layout kLayout1 = Speakers::kStereo;
    const Layout kLayout2 = Speakers::k5Point1;
    const juce::Uuid kAE1 = addAudioElement(kLayout1, "440Hz Sine");
    const juce::Uuid kAE2 =
        addAudioElement(kLayout2, "660Hz Sine", kLayout1.getNumChannels());
    const juce::Uuid kMP1 = addMixPresentation("Mix 440Hz");
    const juce::Uuid kMP2 = addMixPresentation("Mix 660Hz");
    addAudioElementsToMix(kMP1, {kAE1});
    addAudioElementsToMix(kMP2, {kAE2});

    setTestExportOpts({.codec = AudioCodec::LPCM,
                       .profile = FileProfile::BASE_ENHANCED,
                       .sampleRate = kSampleRate});

    auto fileExport = fileExportRepository.get();
    fileExport.setExportFile(path.string());
    fileExportRepository.update(fileExport);

    // Prepare processor for rendering
    fio_proc.prepareToPlay(kSampleRate, kSamplesPerFrame);
    fio_proc.setNonRealtime(true);

    // Configure for 2 seconds of audio
    const int totalFrames = 2 * kSampleRate;
    const int numBlocks = totalFrames / kSamplesPerFrame;

    // Create combined buffer for both audio elements
    const int totalChannels =
        kLayout1.getNumChannels() + kLayout2.getNumChannels();
    juce::AudioBuffer<float> combinedBuffer(totalChannels, kSamplesPerFrame);
    juce::MidiBuffer midiBuffer;

    // Process audio in blocks
    for (int block = 0; block < numBlocks; ++block) {
      // Clear the combined buffer
      combinedBuffer.clear();

      // Generate 440Hz sine wave for first element
      juce::AudioBuffer<float> sineWave440(1, kSamplesPerFrame);
      float* channelData1 = sineWave440.getWritePointer(0);
      for (int i = 0; i < kSamplesPerFrame; ++i) {
        channelData1[i] =
            sampleSine(440.0f, block * kSamplesPerFrame + i, kSampleRate);
      }
      for (int channel = 0; channel < kLayout1.getNumChannels(); ++channel) {
        combinedBuffer.copyFrom(channel, 0, sineWave440, 0, 0,
                                kSamplesPerFrame);
      }

      // Generate 660Hz sine wave for second element
      juce::AudioBuffer<float> sineWave660(1, kSamplesPerFrame);
      float* channelData2 = sineWave660.getWritePointer(0);
      for (int i = 0; i < kSamplesPerFrame; ++i) {
        channelData2[i] =
            sampleSine(660.0f, block * kSamplesPerFrame + i, kSampleRate);
      }
      for (int channel = 0; channel < kLayout2.getNumChannels(); ++channel) {
        combinedBuffer.copyFrom(kLayout1.getNumChannels() + channel, 0,
                                sineWave660, 0, 0, kSamplesPerFrame);
      }

      // Process the combined buffer
      fio_proc.processBlock(combinedBuffer, midiBuffer);
    }

    // Clean up
    fio_proc.setNonRealtime(false);
  }

  void createIAMFFile30SecStereo(const std::filesystem::path& path) {
    const Layout kLayout = Speakers::kStereo;
    const juce::Uuid kAE = addAudioElement(kLayout, "Stereo Sine");
    const juce::Uuid kMP = addMixPresentation("Stereo Mix");
    addAudioElementsToMix(kMP, {kAE});

    setTestExportOpts({.codec = AudioCodec::LPCM, .sampleRate = kSampleRate});

    auto fileExport = fileExportRepository.get();
    fileExport.setExportFile(path.string());
    fileExportRepository.update(fileExport);

    // Prepare processor for rendering
    fio_proc.prepareToPlay(kSampleRate, kSamplesPerFrame);
    fio_proc.setNonRealtime(true);

    // Configure for 30 seconds of audio
    const int totalFrames = 30 * kSampleRate;
    const int numBlocks = totalFrames / kSamplesPerFrame;

    // Create buffer for stereo audio
    const int totalChannels = kLayout.getNumChannels();
    juce::AudioBuffer<float> audioBuffer(totalChannels, kSamplesPerFrame);
    juce::MidiBuffer midiBuffer;

    // Process audio in blocks
    for (int block = 0; block < numBlocks; ++block) {
      // Clear the buffer
      audioBuffer.clear();

      // Generate 440Hz sine wave
      juce::AudioBuffer<float> sineWave(1, kSamplesPerFrame);
      float* channelData = sineWave.getWritePointer(0);
      for (int i = 0; i < kSamplesPerFrame; ++i) {
        channelData[i] =
            sampleSine(440.0f, block * kSamplesPerFrame + i, kSampleRate);
      }

      // Copy to all channels
      for (int channel = 0; channel < totalChannels; ++channel) {
        audioBuffer.copyFrom(channel, 0, sineWave, 0, 0, kSamplesPerFrame);
      }

      // Process the buffer
      fio_proc.processBlock(audioBuffer, midiBuffer);
    }

    // Clean up
    fio_proc.setNonRealtime(false);
  }

 protected:
  FileOutputTests()
      : ex(fileExportRepository.get()),
        fio_proc(fileExportRepository, audioElementRepository, mixRepository,
                 mixPresentationLoudnessRepository) {
    // Configure basic audio export data
    ex.setExportAudio(true);
    ex.setAudioFileFormat(AudioFileFormat::IAMF);
    ex.setSampleRate(kSampleRate);
    ex.setVideoSource((std::filesystem::current_path()
                           .parent_path()
                           .append("common")
                           .append("processors")
                           .append("tests")
                           .append("test_resources")
                           .append("SilentSampleVideo_h264.mp4"))
                          .string());
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
    int sampleRate = kSampleRate;
    bool exportVideo = false;
    std::string videoSource = (std::filesystem::current_path()
                                   .parent_path()
                                   .append("common")
                                   .append("processors")
                                   .append("tests")
                                   .append("test_resources")
                                   .append("SilentSampleVideo_h264.mp4"))
                                  .string();
  };

  void setTestExportOpts(const ExportTestOpts opts) {
    const FileProfile resolvedProfile =
        opts.profile.has_value() ? *opts.profile
                                 : profileFromAEs(audioElementRepository);
    ex.setProfile(resolvedProfile);
    ex.setAudioCodec(opts.codec);
    ex.setSampleRate(opts.sampleRate);
    ex.setExportVideo(opts.exportVideo);
    ex.setVideoSource(opts.videoSource);
    fileExportRepository.update(ex);
  }

  // Constants
  const std::vector<Speakers::AudioElementSpeakerLayout> kAudioElementLayouts =
      {
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

  const std::filesystem::path kTestSourceVideo =
      std::filesystem::current_path().parent_path() /
      "common/processors/tests/test_resources/SilentSampleVideo";

  const std::vector<std::string> kTestSourceVideoCodecs = {"h264", "h265",
                                                           "av1"};

  const std::vector<std::string> kTestSourceVideoContainers = {".mp4", ".mov"};

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
