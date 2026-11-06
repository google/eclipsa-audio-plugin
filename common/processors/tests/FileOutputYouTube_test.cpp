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
#include "data_structures/src/YouTubeExportLimits.h"
#include "processors/file_output/iamf_export_utils/IAMFExportUtil.h"
#include "processors/tests/FileOutputTestUtils.h"

namespace {
// Exposes the render state closeFileExport resets.
class TestableFileOutputProcessor : public FileOutputProcessor {
 public:
  using FileOutputProcessor::FileOutputProcessor;
  bool isRendering() const { return performingRender_; }
  bool hasSecurityScopedAccess() const {
    return securityScopedHandle_ != nullptr;
  }
};

// Counts every change to the stored export error.
class ExportErrorChangeCounter : public juce::ValueTree::Listener {
 public:
  explicit ExportErrorChangeCounter(FileExportRepository& repo) : repo_(repo) {
    repo_.registerListener(this);
  }
  ~ExportErrorChangeCounter() override { repo_.deregisterListener(this); }
  void valueTreePropertyChanged(juce::ValueTree&,
                                const juce::Identifier& property) override {
    if (property == FileExport::kExportError) ++count;
  }
  int count = 0;

 private:
  FileExportRepository& repo_;
};
}  // namespace

class FileOutputYouTube : public FileOutputTests {
 protected:
  FileOutputYouTube() {
    const juce::Uuid kAE = addAudioElement(Speakers::kStereo);
    const juce::Uuid kMP = addMixPresentation();
    addAudioElementsToMix(kMP, {kAE});
    setTestExportOpts({.codec = AudioCodec::LPCM});
    setExportProfile(ExportProfile::kYouTube);
  }

  std::unique_ptr<TestableFileOutputProcessor> makeProcessor(
      const YouTubeExportLimits::Limits& limits = {}) {
    return std::make_unique<TestableFileOutputProcessor>(
        fileExportRepository, fpbr, audioElementRepository, mixRepository,
        mixPresentationLoudnessRepository, roomSetupRepository, limits);
  }

  void bounce(FileOutputProcessor& proc, unsigned numBlocks = 8) {
    bounceAudio(proc, audioElementRepository, kSampleRate, kSamplesPerFrame,
                numBlocks);
  }

  void bouncePremierePro(const YouTubeExportLimits::Limits& limits = {}) {
    bouncePremiereProAudio(fileExportRepository, fpbr, audioElementRepository,
                           mixRepository, mixPresentationLoudnessRepository,
                           roomSetupRepository, kSampleRate, kSamplesPerFrame,
                           8, limits);
  }

  void setRange(juce::int64 startSample, juce::int64 endSample) {
    FileExport config = fileExportRepository.get();
    config.setStartSampleIdx(startSample);
    config.setEndSampleIdx(endSample);
    fileExportRepository.update(config);
  }

  ExportError exportError() {
    return fileExportRepository.get().getExportError();
  }

  std::filesystem::path elementWavPath() const {
    return iamfOutPath.string() + "_Audio Element.wav";
  }

  // A cap that falls between the second and third block.
  static constexpr double kCapSec = 2.5 * kSamplesPerFrame / kSampleRate;

  // One second past YouTube's 12-hour limit.
  static constexpr juce::int64 kOverCapEnd = static_cast<juce::int64>(
      (YouTubeExportLimits::kMaxDurationSec + 1.0) * kSampleRate);
};

// A set range over 12 hours stops the export before any writer is created and
// still closes it out.
TEST_F(FileOutputYouTube, range_over_cap_stops_before_render) {
  auto proc = makeProcessor();
  setRange(0, kOverCapEnd);
  bounce(*proc);

  EXPECT_EQ(exportError(), kYouTubeDurationExceeded);
  EXPECT_FALSE(std::filesystem::exists(iamfOutPath));
  EXPECT_FALSE(std::filesystem::exists(elementWavPath()));
  EXPECT_FALSE(proc->isRendering());
  EXPECT_FALSE(proc->hasSecurityScopedAccess());
}

// An estimated output over the size limit stops the export the same way.
TEST_F(FileOutputYouTube, size_estimate_over_limit_stops_before_render) {
  // One second of 24-bit stereo LPCM at 16 kHz is 96,000 bytes.
  auto proc = makeProcessor({.maxBytes = 50'000});
  setRange(0, kSampleRate);
  bounce(*proc);

  EXPECT_EQ(exportError(), kYouTubeSizeExceeded);
  EXPECT_FALSE(std::filesystem::exists(iamfOutPath));
  EXPECT_FALSE(std::filesystem::exists(elementWavPath()));
  EXPECT_FALSE(proc->isRendering());
  EXPECT_FALSE(proc->hasSecurityScopedAccess());
}

// The size estimate passes when it is within the limit.
TEST_F(FileOutputYouTube, size_estimate_within_limit_exports) {
  auto proc = makeProcessor({.maxBytes = 150'000});
  setRange(0, kSampleRate);
  bounce(*proc);

  EXPECT_EQ(exportError(), kNoError);
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
}

// With no range and an unreadable video, the pre-render check is skipped and
// the export reaches the in-render cap, which stops it.
TEST_F(FileOutputYouTube, unknown_inputs_proceed_to_later_checks) {
  setTestExportOpts({.codec = AudioCodec::LPCM,
                     .exportVideo = true,
                     .videoSource = "/nonexistent/source.mp4"});
  auto proc = makeProcessor({.maxDurationSec = kCapSec});
  bounce(*proc);

  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_EQ(exportError(), kYouTubeDurationExceeded);
  EXPECT_FALSE(std::filesystem::exists(videoOutPath));
}

// With no range and no video, a short export under YouTube succeeds.
TEST_F(FileOutputYouTube, no_range_short_export_succeeds) {
  auto proc = makeProcessor();
  bounce(*proc);

  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_EQ(exportError(), kNoError);
}

// With no end set, writing stops before the block that would cross the cap,
// and the error is recorded once.
TEST_F(FileOutputYouTube, cap_stops_writing_and_records_once) {
  {
    ExportErrorChangeCounter counter(fileExportRepository);
    auto proc = makeProcessor({.maxDurationSec = kCapSec});
    bounce(*proc, 8);
    EXPECT_EQ(exportError(), kYouTubeDurationExceeded);
    EXPECT_EQ(counter.count, 1);
  }
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  const auto kCappedSize = std::filesystem::file_size(iamfOutPath);
  std::filesystem::remove(iamfOutPath);

  // Reference: exactly two blocks, the most that fit under the cap.
  setExportProfile(ExportProfile::kAdvanced);
  bounce(fio_proc, 2);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_EQ(kCappedSize, std::filesystem::file_size(iamfOutPath));
}

// A finished file over the size limit is kept but not muxed.
TEST_F(FileOutputYouTube, close_size_over_limit_skips_mux) {
  setTestExportOpts({.codec = AudioCodec::LPCM, .exportVideo = true});
  const juce::String kVideo = fileExportRepository.get().getVideoSource();
  const double kVideoSec = IAMFExportHelper::getMediaDurationSeconds(kVideo);
  ASSERT_GT(kVideoSec, 0.0);
  const std::int64_t kVideoBytes = juce::File(kVideo).getSize();
  // 24-bit stereo LPCM.
  const std::int64_t kAudioBytesForVideo =
      static_cast<std::int64_t>(2 * kSampleRate * kVideoSec * 3);

  // The pre-render estimate (video + audio for the video's length) fits;
  // rendering three times the video's length does not.
  auto proc =
      makeProcessor({.maxBytes = kVideoBytes + 2 * kAudioBytesForVideo});
  const unsigned kBlocks =
      static_cast<unsigned>(3 * kVideoSec * kSampleRate / kSamplesPerFrame);
  bounce(*proc, kBlocks);

  EXPECT_EQ(exportError(), kYouTubeSizeExceeded);
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
  EXPECT_FALSE(std::filesystem::exists(videoOutPath));
}

// A session that needs Base Enhanced still exports under YouTube.
TEST_F(FileOutputYouTube, base_enhanced_session_still_exports) {
  addAudioElement(Speakers::kExpl7Point1Point4Front, "Expanded",
                  Speakers::kStereo.getNumChannels());
  setTestExportOpts(
      {.codec = AudioCodec::LPCM, .profile = FileProfile::BASE_ENHANCED});
  auto proc = makeProcessor();
  bounce(*proc);

  EXPECT_EQ(exportError(), kNoError);
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
}

// Under Advanced, the inputs that trip each YouTube limit export as today.
TEST_F(FileOutputYouTube, advanced_ignores_limits) {
  setExportProfile(ExportProfile::kAdvanced);
  const YouTubeExportLimits::Limits kTiny{.maxDurationSec = kCapSec,
                                          .maxBytes = 1};

  // Range and size over the limits.
  setRange(0, kSampleRate);
  {
    auto proc = makeProcessor(kTiny);
    bounce(*proc);
  }
  EXPECT_EQ(exportError(), kNoError);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  std::filesystem::remove(iamfOutPath);

  // No range, past the duration cap.
  setRange(0, 0);
  std::uintmax_t cappedInputSize = 0;
  {
    auto proc = makeProcessor(kTiny);
    bounce(*proc, 8);
  }
  EXPECT_EQ(exportError(), kNoError);
  ASSERT_TRUE(std::filesystem::exists(iamfOutPath));
  cappedInputSize = std::filesystem::file_size(iamfOutPath);
  std::filesystem::remove(iamfOutPath);

  bounce(fio_proc, 8);
  EXPECT_EQ(cappedInputSize, std::filesystem::file_size(iamfOutPath));
}

// The Premiere Pro processor inherits the YouTube limits.
TEST_F(FileOutputYouTube, premiere_pro_range_over_cap_stops_before_render) {
  setRange(0, kOverCapEnd);
  bouncePremierePro();

  EXPECT_EQ(exportError(), kYouTubeDurationExceeded);
  EXPECT_FALSE(std::filesystem::exists(iamfOutPath));
}

// The Premiere Pro processor stops at the in-render cap.
TEST_F(FileOutputYouTube, premiere_pro_cap_stops_writing) {
  bouncePremierePro({.maxDurationSec = kCapSec});

  EXPECT_EQ(exportError(), kYouTubeDurationExceeded);
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
}

// Under Advanced, the Premiere Pro processor exports the same inputs as today.
TEST_F(FileOutputYouTube, premiere_pro_advanced_ignores_limits) {
  setExportProfile(ExportProfile::kAdvanced);
  setRange(0, kOverCapEnd);
  bouncePremierePro({.maxDurationSec = kCapSec, .maxBytes = 1});

  EXPECT_EQ(exportError(), kNoError);
  EXPECT_TRUE(std::filesystem::exists(iamfOutPath));
}
