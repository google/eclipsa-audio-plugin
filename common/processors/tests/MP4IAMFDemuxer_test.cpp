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

// Demuxer-specific tests using the shared fixture
class MP4IAMFDemuxerTest : public SharedTestFixture {};

// Test muxing with an IAMF file with a single channel-based audio element
TEST_F(MP4IAMFDemuxerTest, mux_iamf_1ae_cb) {
  setup_1ae_cb();

  // Configure video export settings
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

  // Validate the MP4 file was created
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_TRUE(std::filesystem::exists(videoOutPath));

  // Validate the muxed file integrity
  MP4IAMFDemuxer demuxer;
  EXPECT_TRUE(demuxer.verifyIAMFIntegrity(
      videoPathStr, iamfPathStr, kSampleRate,
      16,              // Bit depth
      SOUND_SYSTEM_A,  // Sound system (stereo=0)
      0.01f            // Tolerance (1%)
      ))
      << "IAMF integrity verification failed";

  // Clean up created files
  std::filesystem::remove(iamfOutPath);
  std::filesystem::remove(videoOutPath);
}

// Test muxing with an IAMF file with a single scene-based audio element
TEST_F(MP4IAMFDemuxerTest, mux_iamf_1ae_sb) {
  setup_1ae_sb();

  // Configure video export settings
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

  // Validate the MP4 file was created
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_TRUE(std::filesystem::exists(videoOutPath));

  // Validate the muxed file integrity
  MP4IAMFDemuxer demuxer;
  EXPECT_TRUE(demuxer.verifyIAMFIntegrity(
      videoPathStr, iamfPathStr, kSampleRate,
      16,              // Bit depth
      SOUND_SYSTEM_A,  // Sound system (stereo=0)
      0.01f            // Tolerance (1%)
      ))
      << "IAMF integrity verification failed";

  // Clean up created files
  std::filesystem::remove(iamfOutPath);
  std::filesystem::remove(videoOutPath);
}

// Test muxing with an IAMF file with 2 channel-based audio elements
TEST_F(MP4IAMFDemuxerTest, mux_iamf_2ae_cb) {
  setup_2ae_cb();

  // Configure video export settings
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

  // Validate the MP4 file was created
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_TRUE(std::filesystem::exists(videoOutPath));

  // Validate the muxed file integrity
  MP4IAMFDemuxer demuxer;
  EXPECT_TRUE(demuxer.verifyIAMFIntegrity(
      videoPathStr, iamfPathStr, kSampleRate,
      16,              // Bit depth
      SOUND_SYSTEM_A,  // Sound system (stereo=0)
      0.01f            // Tolerance (1%)
      ))
      << "IAMF integrity verification failed";

  // Clean up created files
  std::filesystem::remove(iamfOutPath);
  std::filesystem::remove(videoOutPath);
}

// Complete end-to-end test with a single channel-based audio element
TEST_F(MP4IAMFDemuxerTest, e2e_iamf_1ae_cb) {
  // Set up a single channel-based audio element
  setup_1ae_cb();  // Sets up stereo layout

  // Configure export settings
  ex = fileExportRepository.get();
  ex.setExportVideo(true);
  ex.setVideoSource(videoSourcePath.string());
  ex.setProfile(FileProfile::SIMPLE);
  fileExportRepository.update(ex);

  // Run end-to-end test
  EXPECT_TRUE(runEndToEndTest()) << "End-to-end test failed";
}

// Complete end-to-end test with a single scene-based audio element
TEST_F(MP4IAMFDemuxerTest, e2e_iamf_1ae_sb) {
  // Set up a single scene-based audio element
  setup_1ae_sb();

  // Configure export settings
  ex = fileExportRepository.get();
  ex.setExportVideo(true);
  ex.setVideoSource(videoSourcePath.string());

  ex.setProfile(FileProfile::SIMPLE);
  fileExportRepository.update(ex);

  // Run end-to-end test
  EXPECT_TRUE(runEndToEndTest()) << "End-to-end test failed";
}

// Complete end-to-end test with two channel-based audio elements
TEST_F(MP4IAMFDemuxerTest, e2e_iamf_2ae_cb) {
  // Set up two channel-based audio elements
  setup_2ae_cb();

  // Configure export settings
  ex = fileExportRepository.get();
  ex.setExportVideo(true);
  ex.setVideoSource(videoSourcePath.string());
  ex.setProfile(FileProfile::BASE_ENHANCED);
  fileExportRepository.update(ex);

  // Run end-to-end test
  EXPECT_TRUE(runEndToEndTest()) << "End-to-end test failed";
}

// Test all speaker layouts with end-to-end verification
TEST_F(MP4IAMFDemuxerTest, e2e_iamf_all_layouts) {
  // Check if video source exists, skip video tests if not
  if (!std::filesystem::exists(videoSourcePath)) {
    std::cerr << "Video source not found: " << videoSourcePath.string()
              << std::endl;
    GTEST_SKIP() << "Skipping test as video source file not found";
  }

  for (const Speakers::AudioElementSpeakerLayout aeLayout :
       kAudioElementLayouts) {
    // Create an AudioElement with the current layout
    audioElementRepository.clear();
    AudioElement ae(juce::Uuid(), "Audio Element", aeLayout.toString(),
                    aeLayout, 0);
    audioElementRepository.add(ae);

    // Add the audio element to the mix presentation
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

    // Configure export settings
    ex = fileExportRepository.get();
    ex.setExportVideo(true);
    ex.setVideoSource(videoSourcePath.string());
    ex.setProfile(aeLayout == Speakers::kMono ||
                          aeLayout == Speakers::kStereo ||
                          aeLayout == Speakers::kBinaural
                      ? FileProfile::SIMPLE
                      : FileProfile::BASE_ENHANCED);
    fileExportRepository.update(ex);

    // Generate and verify
    generateAndBounceAudio();

    EXPECT_TRUE(std::filesystem::exists(iamfOutPath))
        << "IAMF file wasn't created for layout: " << aeLayout.toString();
    EXPECT_TRUE(std::filesystem::exists(videoOutPath))
        << "MP4 file wasn't created for layout: " << aeLayout.toString();

    // Verify IAMF integrity
    MP4IAMFDemuxer demuxer;
    bool integrityResult =
        demuxer.verifyIAMFIntegrity(videoPathStr, iamfPathStr, kSampleRate,
                                    16,              // Bit depth
                                    SOUND_SYSTEM_A,  // Sound system (stereo=0)
                                    0.01f            // Tolerance (1%)
        );

    EXPECT_TRUE(integrityResult)
        << "IAMF integrity verification failed for layout: "
        << aeLayout.toString();

    // Clean up
    std::filesystem::remove(iamfOutPath);
    std::filesystem::remove(videoOutPath);
  }
}

// Test various encoding formats (LPCM, FLAC, OPUS)
TEST_F(MP4IAMFDemuxerTest, e2e_iamf_codecs) {
  // Set up a single channel-based audio element (stereo for simplicity)
  audioElementRepository.clear();
  AudioElement ae(juce::Uuid(), "Audio Element", "Stereo", Speakers::kStereo,
                  0);
  audioElementRepository.add(ae);

  // Add the audio element to the mix presentation
  mixRepository.clear();
  MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1,
                      LanguageData::MixLanguages::English, {});
  MixPresentationLoudness mixLoudness(mp1.getId());
  mp1.addAudioElement(ae.getId(), 0, ae.getName());
  mixRepository.add(mp1);
  mixPresentationLoudnessRepository.add(mixLoudness);

  // Base export settings
  ex = fileExportRepository.get();
  ex.setExportVideo(true);
  ex.setVideoSource(videoSourcePath.string());
  ex.setProfile(FileProfile::SIMPLE);

  // Test different codecs
  std::vector<AudioCodec> codecs = {AudioCodec::LPCM, AudioCodec::FLAC,
                                    AudioCodec::OPUS};
  std::vector<std::string> codecNames = {"LPCM", "FLAC", "OPUS"};

  for (size_t i = 0; i < codecs.size(); i++) {
    // Set codec
    ex.setAudioCodec(codecs[i]);
    fileExportRepository.update(ex);

    // Run the test
    generateAndBounceAudio();

    // Verify files were created
    EXPECT_TRUE(std::filesystem::exists(iamfOutPath))
        << "IAMF file wasn't created for codec: " << codecNames[i];
    EXPECT_TRUE(std::filesystem::exists(videoOutPath))
        << "MP4 file wasn't created for codec: " << codecNames[i];

    // Verify integrity
    MP4IAMFDemuxer demuxer;
    bool integrityResult =
        demuxer.verifyIAMFIntegrity(videoPathStr, iamfPathStr, kSampleRate,
                                    16,              // Bit depth
                                    SOUND_SYSTEM_A,  // Sound system (stereo=0)
                                    0.01f            // Tolerance (1%)
        );

    EXPECT_TRUE(integrityResult)
        << "IAMF integrity verification failed for codec: " << codecNames[i];

    // Clean up
    std::filesystem::remove(iamfOutPath);
    std::filesystem::remove(videoOutPath);
  }
}

// Test different bit depths
TEST_F(MP4IAMFDemuxerTest, e2e_iamf_bit_depths) {
  // Set up a simple stereo audio element
  audioElementRepository.clear();
  AudioElement ae(juce::Uuid(), "Audio Element", "Stereo", Speakers::kStereo,
                  0);
  audioElementRepository.add(ae);

  // Add the audio element to the mix presentation
  mixRepository.clear();
  MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1,
                      LanguageData::MixLanguages::English, {});
  MixPresentationLoudness mixLoudness(mp1.getId());
  mp1.addAudioElement(ae.getId(), 0, ae.getName());
  mixRepository.add(mp1);
  mixPresentationLoudnessRepository.add(mixLoudness);

  // Base export settings
  ex = fileExportRepository.get();
  ex.setExportVideo(true);
  ex.setVideoSource(videoSourcePath.string());
  ex.setProfile(FileProfile::SIMPLE);
  fileExportRepository.update(ex);

  // Test different bit depths (16, 24, 32)
  std::vector<int> bitDepths = {16, 24, 32};
  MP4IAMFDemuxer demuxer;

  for (int bitDepth : bitDepths) {
    // Generate files
    generateAndBounceAudio();

    // Verify integrity with specified bit depth
    bool integrityResult =
        demuxer.verifyIAMFIntegrity(videoPathStr, iamfPathStr, kSampleRate,
                                    bitDepth,        // Use specific bit depth
                                    SOUND_SYSTEM_A,  // Sound system (stereo=0)
                                    0.01f            // Tolerance (1%)
        );

    EXPECT_TRUE(integrityResult)
        << "IAMF integrity verification failed for bit depth: " << bitDepth;

    // Clean up
    std::filesystem::remove(iamfOutPath);
    std::filesystem::remove(videoOutPath);
  }
}

// Test different sample rates
TEST_F(MP4IAMFDemuxerTest, e2e_iamf_sample_rates) {
  // Set up a simple stereo audio element
  audioElementRepository.clear();
  AudioElement ae(juce::Uuid(), "Audio Element", "Stereo", Speakers::kStereo,
                  0);
  audioElementRepository.add(ae);

  // Add the audio element to the mix presentation
  mixRepository.clear();
  MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1,
                      LanguageData::MixLanguages::English, {});
  MixPresentationLoudness mixLoudness(mp1.getId());
  mp1.addAudioElement(ae.getId(), 0, ae.getName());
  mixRepository.add(mp1);
  mixPresentationLoudnessRepository.add(mixLoudness);

  // Test different sample rates
  std::vector<int> sampleRates = {44100, 48000, 96000};
  MP4IAMFDemuxer demuxer;

  for (int sampleRate : sampleRates) {
    // Configure export settings with specific sample rate
    ex = fileExportRepository.get();
    ex.setExportVideo(true);
    ex.setVideoSource(videoSourcePath.string());
    ex.setProfile(FileProfile::SIMPLE);
    ex.setSampleRate(sampleRate);
    fileExportRepository.update(ex);

    // Generate files
    generateAndBounceAudio();

    // Verify integrity with specified sample rate
    bool integrityResult =
        demuxer.verifyIAMFIntegrity(videoPathStr, iamfPathStr,
                                    sampleRate,      // Use specific sample rate
                                    16,              // Bit depth
                                    SOUND_SYSTEM_A,  // Sound system (stereo=0)
                                    0.01f            // Tolerance (1%)
        );

    EXPECT_TRUE(integrityResult)
        << "IAMF integrity verification failed for sample rate: " << sampleRate;

    // Clean up
    std::filesystem::remove(iamfOutPath);
    std::filesystem::remove(videoOutPath);
  }
}