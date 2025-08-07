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

#include "FileOutputFixture.h"

// Validate IAMF data has been initialized correctly with construction of
// FileOutputProcessor
TEST(test_fio_proc, init_iamf_metadata) {
  TestFileExportRepository fileExportRepository;
  TestAudioElementRepository audioElementRepository;
  TestMixPresentationRepository mixPresentationRepository;
  TestMixPresentationLoudnessRepository mixPresentationLoudnessRepository;
  iamf_tools_cli_proto::UserMetadata iamfMD;

  // Create instance of FIO processor
  FileOutputProcessor fio_proc(fileExportRepository, audioElementRepository,
                               mixPresentationRepository,
                               mixPresentationLoudnessRepository);

  // Expect all MD to initially be cleared.
  fio_proc.initIamfMetadata(iamfMD, "test.wav");
  EXPECT_EQ(iamfMD.codec_config_metadata_size(), 0);
  EXPECT_EQ(iamfMD.audio_element_metadata_size(), 0);
  EXPECT_EQ(iamfMD.mix_presentation_metadata_size(), 0);
  EXPECT_EQ(iamfMD.audio_frame_metadata_size(), 0);
  EXPECT_EQ(iamfMD.ia_sequence_header_metadata_size(), 0);

  EXPECT_TRUE(
      iamfMD.test_vector_metadata().partition_mix_gain_parameter_blocks() ==
      false);
  EXPECT_TRUE(iamfMD.test_vector_metadata().file_name_prefix() == "test.wav");
}

// Validate IAMF metadata is updated correctly from the FileExportRepository.
TEST(test_fio_proc, md_from_fexport_repo) {
  TestFileExportRepository fileExportRepository;
  TestAudioElementRepository audioElementRepository;
  TestMixPresentationRepository mixPresentationRepository;
  TestMixPresentationLoudnessRepository mixPresentationLoudnessRepository;
  iamf_tools_cli_proto::UserMetadata iamfMD;

  // Select the FLAC audio codec for file export.
  FileExport ex = fileExportRepository.get();
  ex.setAudioCodec(AudioCodec::FLAC);
  fileExportRepository.update(ex);

  // Create instance of FIO processor
  FileOutputProcessor fio_proc(fileExportRepository, audioElementRepository,
                               mixPresentationRepository,
                               mixPresentationLoudnessRepository);

  // Update the IAMF metadata from the repository.
  fio_proc.updateIamfMDFromRepository(fileExportRepository, iamfMD);

  EXPECT_EQ(iamfMD.codec_config_metadata(0).codec_config().codec_id(),
            iamf_tools_cli_proto::CodecId::CODEC_ID_FLAC);
}

// Validate IAMF metadata is updated correctly from the
// AudioElementRepository
TEST(test_fio_proc, md_from_ae_repo) {
  TestFileExportRepository fileExportRepository;
  TestAudioElementRepository audioElementRepository;
  TestMixPresentationRepository mixPresentationRepository;
  TestMixPresentationLoudnessRepository mixPresentationLoudnessRepository;
  iamf_tools_cli_proto::UserMetadata iamfMD;
  std::unordered_map<juce::Uuid, int> audioElementIDMap;

  // Create some AudioElements to fill the repository with.
  AudioElement ae1(juce::Uuid(), "Audio Element 1", "Description 1",
                   Speakers::k5Point1, 0);
  AudioElement ae2(juce::Uuid(), "Audio Element 2", "Description 2",
                   Speakers::kStereo, 0);
  audioElementRepository.add(ae1);
  audioElementRepository.add(ae2);
  EXPECT_TRUE(audioElementRepository.getItemCount() == 2);

  // Set the file export repository to export an IAMF file
  FileExport ex = fileExportRepository.get();
  ex.setExportAudio(true);
  ex.setAudioFileFormat(AudioFileFormat::IAMF);
  ex.setExportFile(juce::File::getCurrentWorkingDirectory()
                       .getChildFile("test.iamf")
                       .getFullPathName());
  ex.setExportFolder(
      juce::File::getCurrentWorkingDirectory().getFullPathName());
  fileExportRepository.update(ex);

  // Create instance of FIO processor
  FileOutputProcessor fio_proc(fileExportRepository, audioElementRepository,
                               mixPresentationRepository,
                               mixPresentationLoudnessRepository);

  // Update the IAMF metadata from the repository.
  fio_proc.setNonRealtime(true);
  fio_proc.updateIamfMDFromRepository(audioElementRepository, iamfMD,
                                      audioElementIDMap);

  juce::Logger::outputDebugString(juce::String(iamfMD.DebugString()));

  // Validate the related IAMF metadata fields updated.
  auto ae1MD = iamfMD.audio_element_metadata(0);
  auto ae2MD = iamfMD.audio_element_metadata(1);
  EXPECT_TRUE(ae1MD.has_scalable_channel_layout_config());
  EXPECT_EQ(ae1MD.num_substreams(), 4);
  EXPECT_TRUE(ae2MD.has_scalable_channel_layout_config());
  EXPECT_EQ(ae2MD.num_substreams(), 1);
}

TEST(test_channel_based, output_iamf_file) {
  juce::ValueTree testState("test_state");

  FileExportRepository fileExportRepository(
      testState.getOrCreateChildWithName("file", nullptr));
  AudioElementRepository audioElementRepository(
      testState.getOrCreateChildWithName("element", nullptr));
  MixPresentationRepository mixRepository(
      testState.getOrCreateChildWithName("mix", nullptr));
  MixPresentationLoudnessRepository mixPresentationLoudnessRepository(
      testState.getOrCreateChildWithName("mixLoudness", nullptr));

  iamf_tools_cli_proto::UserMetadata iamfMD;

  // Set up an output filepath
  juce::String iamfPathStr(juce::File::getCurrentWorkingDirectory()
                               .getChildFile("test.iamf")
                               .getFullPathName());
  std::filesystem::path iamfPath(iamfPathStr.toStdString());
  std::filesystem::remove(iamfPath);
  FileExport ex = fileExportRepository.get();
  ex.setExportFolder(
      juce::File::getCurrentWorkingDirectory().getFullPathName());
  ex.setExportFile(juce::File::getCurrentWorkingDirectory()
                       .getChildFile("test")
                       .getFullPathName());
  ex.setExportAudio(true);
  ex.setAudioFileFormat(AudioFileFormat::IAMF);
  fileExportRepository.update(ex);

  // Create some AudioElements to fill the repository with.
  AudioElement ae1(juce::Uuid(), "Audio Element 1", "Description 1",
                   Speakers::kStereo, 0);
  AudioElement ae2(juce::Uuid(), "Audio Element 2", "Description 2",
                   Speakers::k5Point1, 2);
  audioElementRepository.add(ae2);
  audioElementRepository.add(ae1);

  // Create some MixPresentations to fill the repository with.
  const juce::Uuid mixId = juce::Uuid();
  MixPresentation mp1(mixId, "Mix Presentation 1", 1,
                      LanguageData::MixLanguages::English, {});
  MixPresentationLoudness mixLoudness = MixPresentationLoudness(mixId);
  mp1.addAudioElement(ae2.getId(), 0, ae2.getName());
  mp1.addAudioElement(ae1.getId(), 0, ae1.getName());

  mixLoudness.replaceLargestLayout(Speakers::k5Point1);

  mp1.addTagPair("artist", "Rockstars");
  mp1.addTagPair("album", "Eclipsa");
  mixRepository.add(mp1);
  mixPresentationLoudnessRepository.add(mixLoudness);

  // Create instance of FIO processor
  FileOutputProcessor fio_proc(fileExportRepository, audioElementRepository,
                               mixRepository,
                               mixPresentationLoudnessRepository);

  // Start a bounce
  fio_proc.prepareToPlay(16000, 128);
  fio_proc.setNonRealtime(true);

  // Pass 8 channels worth of data for the two audio elements
  juce::AudioBuffer<float> buffer(10, 10);
  juce::MidiBuffer midiBuffer;
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 10; ++j) {
      buffer.setSample(j, i, 0.5f);
    }
  }

  for (int i = 0; i < 10; ++i) {
    fio_proc.processBlock(buffer, midiBuffer);
  }

  // Complete the bounce
  fio_proc.setNonRealtime(false);

  // Validate the IAMF file was created.
  EXPECT_TRUE(std::filesystem::exists(iamfPath));

  // Clean up the IAMF file.
  std::filesystem::remove(iamfPath);
}

TEST(test_ambisonics, output_iamf_file) {
  juce::ValueTree testState("test_state");

  FileExportRepository fileExportRepository(
      testState.getOrCreateChildWithName("file", nullptr));
  AudioElementRepository audioElementRepository(
      testState.getOrCreateChildWithName("element", nullptr));
  MixPresentationRepository mixRepository(
      testState.getOrCreateChildWithName("mix", nullptr));
  MixPresentationLoudnessRepository mixPresentationLoudnessRepository(
      testState.getOrCreateChildWithName("mixLoudness", nullptr));

  iamf_tools_cli_proto::UserMetadata iamfMD;

  // Set up an output filepath
  juce::String iamfPathStr(juce::File::getCurrentWorkingDirectory()
                               .getChildFile("test.iamf")
                               .getFullPathName());
  std::filesystem::path iamfPath(iamfPathStr.toStdString());
  std::filesystem::remove(iamfPath);
  FileExport ex = fileExportRepository.get();
  ex.setExportFolder(
      juce::File::getCurrentWorkingDirectory().getFullPathName());
  ex.setExportFile(juce::File::getCurrentWorkingDirectory()
                       .getChildFile("test")
                       .getFullPathName());
  ex.setExportAudio(true);
  ex.setAudioFileFormat(AudioFileFormat::IAMF);
  fileExportRepository.update(ex);

  // Create some AudioElements to fill the repository with.
  AudioElement ae1(juce::Uuid(), "Audio Element 1", "Description 1",
                   Speakers::kHOA2, 0);
  AudioElement ae2(juce::Uuid(), "Audio Element 2", "Description 2",
                   Speakers::k5Point1, 2);
  audioElementRepository.add(ae2);
  audioElementRepository.add(ae1);

  // Create some MixPresentations to fill the repository with.
  MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1,
                      LanguageData::MixLanguages::English, {});
  MixPresentationLoudness mixLoudness(mp1.getId());
  mp1.addAudioElement(ae2.getId(), 0, ae2.getName());
  mp1.addAudioElement(ae1.getId(), 0, ae1.getName());
  // use Speakers::k5Point1 as the largest layout
  mixLoudness.replaceLargestLayout(Speakers::k5Point1);
  mixRepository.add(mp1);
  mixPresentationLoudnessRepository.add(mixLoudness);

  // Create instance of FIO processor
  FileOutputProcessor fio_proc(fileExportRepository, audioElementRepository,
                               mixRepository,
                               mixPresentationLoudnessRepository);

  // Start a bounce
  fio_proc.prepareToPlay(16000, 128);
  fio_proc.setNonRealtime(true);

  // Pass 8 channels worth of data for the two audio elements
  juce::AudioBuffer<float> buffer(10, 10);
  juce::MidiBuffer midiBuffer;
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 10; ++j) {
      buffer.setSample(j, i, 0.5f);
    }
  }

  for (int i = 0; i < 10; ++i) {
    fio_proc.processBlock(buffer, midiBuffer);
  }

  // Complete the bounce
  fio_proc.setNonRealtime(false);

  // Validate the IAMF file was created.
  EXPECT_TRUE(std::filesystem::exists(iamfPath));

  // Clean up the IAMF file.
  std::filesystem::remove(iamfPath);
}

class FileOutputProcessorTest : public SharedTestFixture {};

TEST_F(FileOutputProcessorTest, iamf_lpc_1ae_extl_1mp) {
  // Iterate over all currently supported Audio Element types, and export an
  // IAMF file.
  for (const Speakers::AudioElementSpeakerLayout aeLayout :
       kAudioElementExpandedLayouts) {
    // Create an AudioElement with the current layout.
    audioElementRepository.clear();
    AudioElement ae(juce::Uuid(), "Audio Element", aeLayout.toString(),
                    aeLayout, 0);
    audioElementRepository.add(ae);

    // Add the audio element to the mix presentation.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1.f,
                        LanguageData::MixLanguages::English, {});
    // not updating the largest layout for explanded layouts
    MixPresentationLoudness mixLoudness(mp1.getId());
    mp1.addAudioElement(ae.getId(), 0, ae.getName());
    mixRepository.add(mp1);
    mixPresentationLoudnessRepository.add(mixLoudness);

    // NOTE: This data is currently set by the UI. Setting here for testing.
    // Update the IAProfile header based on the number of audio
    // elements and channels.
    ex.setProfile(FileProfile::BASE_ENHANCED);
    generateAndBounceAudio();
    bounceExportConfig(ex, "IAMF file failed to be created for layout: " +
                               aeLayout.toString() + " with LPCM encoding.");
  }
}

// Test exporting an IAMF file with an Audio Element of one of the expanded
// loudspeaker layouts as well as an element with a standard layout.
TEST_F(FileOutputProcessorTest, iamf_lpc_2ae_extl_1mp) {
  // Iterate over all currently supported Audio Element types, and export an
  // IAMF file.
  for (const Speakers::AudioElementSpeakerLayout aeLayout :
       kAudioElementExpandedLayouts) {
    // Create an AudioElement with the current layout and one with a stereo
    // layout.
    audioElementRepository.clear();
    AudioElement ae(juce::Uuid(), "Audio Element", aeLayout.toString(),
                    aeLayout, 0);
    AudioElement ae1(juce::Uuid(), "Audio Element", aeLayout.toString(),
                     Speakers::kStereo, 0);
    audioElementRepository.add(ae);
    audioElementRepository.add(ae1);

    // Add the audio element to the mix presentation.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1.f,
                        LanguageData::MixLanguages::English, {});
    MixPresentationLoudness mixLoudness(mp1.getId());
    mp1.addAudioElement(ae.getId(), 0, ae.getName());
    if (aeLayout != Speakers::kBinaural && !aeLayout.isAmbisonics() &&
        aeLayout.getExplBaseLayout().getNumChannels() > 2) {
      mixLoudness.replaceLargestLayout(aeLayout);
    }
    mixRepository.add(mp1);
    mixPresentationLoudnessRepository.add(mixLoudness);

    // NOTE: This data is currently set by the UI. Setting here for testing.
    // Update the IAProfile header based on the number of audio
    // elements and channels.
    ex.setProfile(FileProfile::BASE_ENHANCED);
    bounceExportConfig(
        ex, "IAMF file failed to be created for Mix Presentation with AEs: " +
                ae.getDescription() + " + " + ae1.getDescription() +
                " with LPCM encoding.");
  }
}

TEST_F(FileOutputProcessorTest, iamf_flac_1ae_1mp) {
  ex.setAudioCodec(AudioCodec::FLAC);
  fileExportRepository.update(ex);

  // Iterate over all currently supported Audio Element types, and export an
  // IAMF file.
  for (const Speakers::AudioElementSpeakerLayout aeLayout :
       kAudioElementLayouts) {
    // Create an AudioElement with the current layout.
    audioElementRepository.clear();
    AudioElement ae(juce::Uuid(), "Audio Element", aeLayout.toString(),
                    aeLayout, 0);
    audioElementRepository.add(ae);

    // Add the audio element to the mix presentation.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1,
                        LanguageData::MixLanguages::English, {});
    MixPresentationLoudness mixLoudness(mp1.getId());
    mp1.addAudioElement(ae.getId(), 0, ae.getName());
    if (aeLayout != Speakers::kBinaural && !aeLayout.isAmbisonics()) {
      mixLoudness.replaceLargestLayout(aeLayout);
    }
    mixRepository.add(mp1);
    mixPresentationLoudnessRepository.add(mixLoudness);

    generateAndBounceAudio();

    EXPECT_NE(getLoggedExportStatus().find(
                  "IAMF export attempt completed with status: OK"),
              std::string::npos)
        << getLoggedExportStatus();

    EXPECT_TRUE(std::filesystem::exists(iamfOutPath))
        << "IAMF file failed to be created for layout: " << aeLayout.toString()
        << " with LPCM encoding.";

    std::filesystem::remove(iamfOutPath);
  }
}

TEST_F(FileOutputProcessorTest, iamf_lpc_1ae_2mp) {
  // Iterate over all currently supported Audio Element types, and export an
  // IAMF file.
  for (const Speakers::AudioElementSpeakerLayout aeLayout :
       kAudioElementLayouts) {
    // Create an AudioElement with the current layout.
    audioElementRepository.clear();
    AudioElement ae(juce::Uuid(), "Audio Element", aeLayout.toString(),
                    aeLayout, 0);
    audioElementRepository.add(ae);

    // Add the audio element to the mix presentation.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1.f,
                        LanguageData::MixLanguages::English, {});
    MixPresentation mp2(juce::Uuid(), "Mix Presentation 2", .5f,
                        LanguageData::MixLanguages::English, {});

    MixPresentationLoudness mixLoudness(mp1.getId());
    MixPresentationLoudness mixLoudness2(mp2.getId());

    mp1.addAudioElement(ae.getId(), 0, ae.getName());
    mp2.addAudioElement(ae.getId(), 0, ae.getName());
    if (aeLayout != Speakers::kBinaural && !aeLayout.isAmbisonics()) {
      mixLoudness.replaceLargestLayout(aeLayout);
      mixLoudness2.replaceLargestLayout(aeLayout);
    }
    mixRepository.add(mp1);
    mixRepository.add(mp2);
    mixPresentationLoudnessRepository.add(mixLoudness);
    mixPresentationLoudnessRepository.add(mixLoudness2);
    generateAndBounceAudio();

    EXPECT_NE(getLoggedExportStatus().find(
                  "IAMF export attempt completed with status: OK"),
              std::string::npos)
        << getLoggedExportStatus();

    EXPECT_TRUE(std::filesystem::exists(iamfOutPath))
        << "IAMF file failed to be created for layout: " << aeLayout.toString()
        << " with LPCM encoding.";

    std::filesystem::remove(iamfOutPath);
  }
}

TEST_F(FileOutputProcessorTest, iamf_opus_1ae_1mp) {
  ex.setAudioCodec(AudioCodec::OPUS);
  fileExportRepository.update(ex);

  // Iterate over all currently supported Audio Element types, and export an
  // IAMF file.
  for (const Speakers::AudioElementSpeakerLayout aeLayout :
       kAudioElementLayouts) {
    // Create an AudioElement with the current layout.
    audioElementRepository.clear();
    AudioElement ae(juce::Uuid(), "Audio Element", aeLayout.toString(),
                    aeLayout, 0);
    audioElementRepository.add(ae);

    // Add the audio element to the mix presentation.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1,
                        LanguageData::MixLanguages::English, {});
    MixPresentationLoudness mixLoudness(mp1.getId());
    mp1.addAudioElement(ae.getId(), 0, ae.getName());
    if (aeLayout != Speakers::kBinaural && !aeLayout.isAmbisonics()) {
      mixLoudness.replaceLargestLayout(aeLayout);
    }
    mixRepository.add(mp1);
    mixPresentationLoudnessRepository.add(mixLoudness);
    generateAndBounceAudio();

    EXPECT_TRUE(getLoggedExportStatus().find(
                    "IAMF export attempt completed with status: OK") !=
                std::string::npos)
        << getLoggedExportStatus();

    EXPECT_TRUE(std::filesystem::exists(iamfOutPath))
        << "IAMF file failed to be created for layout: " << aeLayout.toString()
        << " with LPCM encoding.";

    std::filesystem::remove(iamfOutPath);
  }
}

TEST_F(FileOutputProcessorTest, iamf_lpc_2ae_1mp) {
  // Iterate over all currently supported Audio Element types, and export an
  // IAMF file.
  for (const Speakers::AudioElementSpeakerLayout aeLayout :
       kAudioElementLayouts) {
    // Create an AudioElement with the current layout and one with a stereo
    // layout.
    audioElementRepository.clear();
    AudioElement ae(juce::Uuid(), "Audio Element", aeLayout.toString(),
                    aeLayout, 0);
    AudioElement ae1(juce::Uuid(), "Audio Element", aeLayout.toString(),
                     Speakers::kStereo, 0);
    audioElementRepository.add(ae);
    audioElementRepository.add(ae1);

    // Add the audio element to the mix presentation.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1.f,
                        LanguageData::MixLanguages::English, {});
    MixPresentationLoudness mixLoudness(mp1.getId());
    mp1.addAudioElement(ae.getId(), 0, ae.getName());
    if (aeLayout != Speakers::kBinaural && !aeLayout.isAmbisonics() &&
        aeLayout.getNumChannels() > 2) {
      mixLoudness.replaceLargestLayout(aeLayout);
    }
    mixRepository.add(mp1);
    mixPresentationLoudnessRepository.add(mixLoudness);
    // NOTE: This data is currently set by the UI. Setting here for testing.
    // Update the IAProfile header based on the number of audio
    // elements and channels.
    ex.setProfile(FileProfile::BASE_ENHANCED);
    fileExportRepository.update(ex);
    bounceExportConfig(
        ex, "IAMF file failed to be created for Mix Presentation with AEs: " +
                ae.getDescription() + " + " + ae1.getDescription() +
                " with LPCM encoding.");
  }
}

TEST_F(FileOutputProcessorTest, iamf_lpc_2ae_2mp) {
  // Iterate over all currently supported Audio Element types, and export an
  // IAMF file.
  for (const Speakers::AudioElementSpeakerLayout aeLayout :
       kAudioElementLayouts) {
    // Create an AudioElement with the current layout and one with a stereo
    // layout.
    audioElementRepository.clear();
    AudioElement ae(juce::Uuid(), "Audio Element", aeLayout.toString(),
                    aeLayout, 0);
    AudioElement ae1(juce::Uuid(), "Audio Element", aeLayout.toString(),
                     Speakers::kStereo, 0);
    audioElementRepository.add(ae);
    audioElementRepository.add(ae1);

    // Add each audio element to both mix presentations.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1.f,
                        LanguageData::MixLanguages::English, {});
    MixPresentation mp2(juce::Uuid(), "Mix Presentation 2", .5f,
                        LanguageData::MixLanguages::English, {});
    MixPresentationLoudness mixLoudness(mp1.getId());
    MixPresentationLoudness mixLoudness2(mp2.getId());
    mp1.addAudioElement(ae.getId(), 0, ae.getName());
    mp1.addAudioElement(ae1.getId(), 0, ae1.getName());

    mp2.addAudioElement(ae.getId(), 0, ae.getName());
    mp2.addAudioElement(ae1.getId(), 0, ae1.getName());
    if (aeLayout != Speakers::kBinaural && !aeLayout.isAmbisonics() &&
        aeLayout.getNumChannels() > 2) {
      mixLoudness.replaceLargestLayout(aeLayout);
      mixLoudness2.replaceLargestLayout(aeLayout);
    }
    mixRepository.add(mp1);
    mixRepository.add(mp2);
    mixPresentationLoudnessRepository.add(mixLoudness);
    mixPresentationLoudnessRepository.add(mixLoudness2);

    // NOTE: This data is currently set by the UI. Setting here for testing.
    // Update the IAProfile header based on the number of audio
    // elements and channels.
    ex.setProfile(FileProfile::BASE_ENHANCED);
    bounceExportConfig(ex, "LPCM with AEs: " + ae.getDescription() + " + " +
                               ae1.getDescription());
  }
}

TEST_F(FileOutputProcessorTest, iamf_lpc_28ae_1mp) {
  const Speakers::AudioElementSpeakerLayout kAudioElementLayout =
      Speakers::kMono;

  // Create the max number of elements supported by the Base-Enhanced IAProfile
  // (28).
  MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1.f,
                      LanguageData::MixLanguages::English, {});
  for (int i = 0; i < 28; ++i) {
    AudioElement ae(juce::Uuid(), "Audio Element " + juce::String(i),
                    kAudioElementLayout.toString(), kAudioElementLayout, i);
    audioElementRepository.add(ae);
    mp1.addAudioElement(ae.getId(), 1.f, ae.getName());
  }
  MixPresentationLoudness mixLoudness(mp1.getId());  // 28 mono elements
  mixRepository.add(mp1);
  mixPresentationLoudnessRepository.add(mixLoudness);
  // NOTE: This data is currently set by the UI. Setting here for testing.
  // Update the IAProfile header based on the number of audio elements and
  // channels.
  ex.setProfile(FileProfile::BASE_ENHANCED);
  fileExportRepository.update(ex);

  generateAndBounceAudio();

  EXPECT_TRUE(getLoggedExportStatus().find(
                  "IAMF export attempt completed with status: OK") !=
              std::string::npos)
      << getLoggedExportStatus();

  // Unless the IAProfile is invalid (possible given element combinations), we
  // expect an output file.
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));

  std::filesystem::remove(iamfOutPath);
}

// Test muxing with an IAMF file with a single channel-based audio element.
TEST_F(FileOutputProcessorTest, mux_iamf_1ae_cb) {
  setup_1ae_cb();

  // Configure video export settings.
  ex = fileExportRepository.get();
  ex.setExportVideo(true);
  ex.setVideoSource(videoSourcePath.string());
  ex.setProfile(FileProfile::SIMPLE);
  fileExportRepository.update(ex);

  generateAndBounceAudio();

  EXPECT_TRUE(getLoggedExportStatus().find(
                  "IAMF export attempt completed with status: OK") !=
              std::string::npos)
      << getLoggedExportStatus();

  // Validate the MP4 file was created.
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_TRUE(std::filesystem::exists(videoOutPath));

  // Clean up created files.
  std::filesystem::remove(iamfOutPath);
  std::filesystem::remove(videoOutPath);
}

// Test muxing with an IAMF file with a single scene-based audio element.
TEST_F(FileOutputProcessorTest, mux_iamf_1ae_sb) {
  setup_1ae_sb();

  // Configure video export settings.
  ex = fileExportRepository.get();
  ex.setExportVideo(true);
  ex.setVideoSource(videoSourcePath.string());
  ex.setProfile(FileProfile::SIMPLE);
  fileExportRepository.update(ex);

  generateAndBounceAudio();

  EXPECT_TRUE(getLoggedExportStatus().find(
                  "IAMF export attempt completed with status: OK") !=
              std::string::npos)
      << getLoggedExportStatus();

  // Validate the MP4 file was created.
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_TRUE(std::filesystem::exists(videoOutPath));

  // Clean up created files.
  std::filesystem::remove(iamfOutPath);
  std::filesystem::remove(videoOutPath);
}

// Test muxing with an IAMF file with a 2 channel-based audio elements.
TEST_F(FileOutputProcessorTest, mux_iamf_2ae_cb) {
  setup_2ae_cb();

  // Configure video export settings.
  ex = fileExportRepository.get();
  ex.setExportVideo(true);
  ex.setVideoSource(videoSourcePath.string());
  ex.setProfile(FileProfile::BASE_ENHANCED);
  fileExportRepository.update(ex);

  generateAndBounceAudio();

  EXPECT_TRUE(getLoggedExportStatus().find(
                  "IAMF export attempt completed with status: OK") !=
              std::string::npos)
      << getLoggedExportStatus();

  // Validate the MP4 file was created.
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_TRUE(std::filesystem::exists(videoOutPath));

  // Clean up created files.
  std::filesystem::remove(iamfOutPath);
  std::filesystem::remove(videoOutPath);
}

// Test custom LPC settings
TEST_F(FileOutputProcessorTest, iamf_lpc_custom_param) {
  setup_1ae_51();
  auto config = fileExportRepository.get();
  config.setAudioCodec(AudioCodec::LPCM);
  for (int i = 16; i <= 32; i += 8) {
    config.setLPCMSampleSize(i);
    bounceExportConfig(config, "Custom LPCM sample size: " + juce::String(i));
  }
}

// Test custom LPC settings
TEST_F(FileOutputProcessorTest, iamf_opus_custom_param) {
  setup_1ae_51();

  auto config = fileExportRepository.get();
  config.setAudioCodec(AudioCodec::OPUS);
  for (int i = 6000; i < 256000; i = i + 1000) {
    config.setOpusTotalBitrate(i);
    bounceExportConfig(config, "Custom OPUC bitrate: " + juce::String(i));
  }
}

TEST_F(FileOutputProcessorTest, iamf_flac_custom_param) {
  setup_1ae_51();

  auto config = fileExportRepository.get();
  config.setAudioCodec(AudioCodec::FLAC);
  for (int i = 0; i < 16; ++i) {
    config.setFlacCompressionLevel(i);
    fileExportRepository.update(config);
    bounceExportConfig(config, "Custom OPUC bitrate: " + juce::String(i));
  }
}