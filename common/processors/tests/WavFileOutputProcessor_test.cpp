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

#include "processors/file_output/WavFileOutputProcessor.h"

#include <gtest/gtest.h>
#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <functional>
#include <vector>

#include "data_repository/implementation/ActiveMixPresentationRepository.h"
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_repository/implementation/RoomSetupRepository.h"
#include "data_structures/src/ActiveMixPresentation.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/FileExport.h"
#include "data_structures/src/MixPresentation.h"
#include "processors/render/RenderProcessor.h"

namespace {
class TestFileExportRepository : public FileExportRepository {
 public:
  TestFileExportRepository() : FileExportRepository(juce::ValueTree{"test"}) {}
};

class TestRoomSetupRepository : public RoomSetupRepository {
 public:
  TestRoomSetupRepository() : RoomSetupRepository(juce::ValueTree{"test"}) {}
};

class SilentBinauralMix : public BinauralMixSource {
 public:
  SilentBinauralMix() { mix_.clear(); }
  const juce::AudioBuffer<float>& getBinauralMix() const override {
    return mix_;
  }

 private:
  juce::AudioBuffer<float> mix_{2, 512};
};

void setRoomLayout(RoomSetupRepository& repository,
                   Speakers::AudioElementSpeakerLayout layout) {
  RoomSetup roomSetup = repository.get();
  for (const RoomLayout& kOption : speakerLayoutConfigurationOptions) {
    if (kOption.getRoomSpeakerLayout() == layout) {
      roomSetup.setSpeakerLayout(kOption);
    }
  }
  repository.update(roomSetup);
}

// Wraps a real FileWriter (so construction still performs a real open
// against a real, writable path) but lets a test force write()/close() to
// report failure afterward -- the only way to exercise the
// write/close-after-successful-open branches in WavFileOutputProcessor,
// since production only reaches them via disk-full/WAV-4GB-cap conditions.
class FakeFileWriter : public FileWriter {
 public:
  using FileWriter::FileWriter;

  bool write(juce::AudioBuffer<float>& buffer) override {
    ++writeCallCount;
    if (forceWriteFailure) return false;
    return FileWriter::write(buffer);
  }

  bool close() override {
    if (forceCloseFailure) return false;
    return FileWriter::close();
  }

  bool forceWriteFailure = false;
  bool forceCloseFailure = false;
  int writeCallCount = 0;
};

// Queues WavFileOutputProcessor's deferred repository updates (see
// deferRepositoryUpdate()/postToMessageThread()) instead of posting them to
// juce::MessageManager::callAsync -- there is no running JUCE message loop
// in this test binary to pump (runDispatchLoopUntil isn't even compiled in;
// JUCE_MODAL_LOOPS_PERMITTED is 0 for this plugin build). Running the tasks
// immediately/synchronously here would defeat the very thing being tested:
// both the deadlock and the clobbering hazard deferRepositoryUpdate() exists
// to avoid only manifest when a write happens nested inside the triggering
// update() call's own call stack. Queuing and draining explicitly via
// runPendingTasks() -- called only after that outer call has fully
// returned -- reproduces the real timing guarantee callAsync provides,
// without needing a real message loop. Production code always goes through
// the real base-class implementation.
class TestableWavFileOutputProcessor : public WavFileOutputProcessor {
 public:
  using WavFileOutputProcessor::WavFileOutputProcessor;

  void runPendingTasks() {
    std::vector<std::function<void()>> tasks;
    tasks.swap(pendingTasks_);
    for (auto& task : tasks) task();
  }

  size_t pendingTaskCount() const { return pendingTasks_.size(); }

  // Set before triggering an export start so the FileWriter it opens is a
  // FakeFileWriter; the returned pointer is only valid for the lifetime of
  // that export (WavFileOutputProcessor deletes it on close).
  bool forceWriteFailure = false;
  bool forceCloseFailure = false;
  FakeFileWriter* lastFakeWriter = nullptr;
  int writersCreated = 0;

 protected:
  void postToMessageThread(std::function<void()> task) override {
    pendingTasks_.push_back(std::move(task));
  }

  FileWriter* createFileWriter(const juce::String& filename, double sampleRate,
                               const juce::AudioChannelSet& channelSet,
                               int firstChannel, int bitDepth,
                               AudioCodec codec) override {
    auto* fake = new FakeFileWriter(filename, sampleRate, channelSet,
                                    firstChannel, bitDepth, codec);
    fake->forceWriteFailure = forceWriteFailure;
    fake->forceCloseFailure = forceCloseFailure;
    lastFakeWriter = fake;
    ++writersCreated;
    return fake;
  }

 private:
  std::vector<std::function<void()>> pendingTasks_;
};

class WavFileOutputTests : public ::testing::Test {
 protected:
  TestFileExportRepository fileExportRepository;
  TestRoomSetupRepository roomSetupRepository;
  SilentBinauralMix binauralMix;
  TestableWavFileOutputProcessor processor{fileExportRepository,
                                           roomSetupRepository, binauralMix};
};
}  // namespace

// WavFileOutputProcessor registers itself as a listener on
// fileExportRepository (see its constructor); FileExportRepository::update()
// fires that listener synchronously on the calling thread. Toggling
// ManualExport through the repository -- exactly how production code (the
// export button in FileExportScreen.cpp) drives this processor -- exercises
// that same re-entrant path. Before the deferRepositoryUpdate() fix, an open
// failure here recorded the ExportError via a repository update issued
// synchronously from inside that listener callback, which re-entered
// setNonRealtime()'s non-reentrant lock_ and deadlocked. This test hanging
// (rather than completing) is the regression signal for that bug.
TEST_F(WavFileOutputTests,
       open_failure_via_manual_export_toggle_does_not_deadlock) {
  FileExport config = fileExportRepository.get();
  config.setAudioFileFormat(AudioFileFormat::WAV);
  config.setExportAudio(true);
  config.setSampleRate(48000);
  config.setBitDepth(16);
  config.setAudioCodec(AudioCodec::LPCM);
  config.setExportFile("/invalid_path/test.wav");
  config.setManualExport(false);
  fileExportRepository.update(config);

  // Flipping ManualExport to true synchronously triggers this processor's
  // own valueTreePropertyChanged -> checkManualExportStartStop ->
  // setNonRealtime path.
  config.setManualExport(true);
  fileExportRepository.update(config);
  processor.runPendingTasks();

  EXPECT_NE(fileExportRepository.get().getExportError(), ExportError::kNoError);

  // Stopping the (failed) render must also complete without hanging, and
  // must not crash on the null/never-opened writer.
  config = fileExportRepository.get();
  config.setManualExport(false);
  fileExportRepository.update(config);
  processor.runPendingTasks();
}

// Happy-path regression guard mirroring FileOutputTests' equivalent: a
// successful open/close on a valid path must leave ExportError at kNoError.
TEST_F(WavFileOutputTests, successful_export_leaves_no_error) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_output_test.wav";
  std::filesystem::remove(kOutPath);

  FileExport config = fileExportRepository.get();
  config.setAudioFileFormat(AudioFileFormat::WAV);
  config.setExportAudio(true);
  config.setSampleRate(48000);
  config.setBitDepth(16);
  config.setAudioCodec(AudioCodec::LPCM);
  config.setExportFile(kOutPath.string());
  config.setManualExport(false);
  fileExportRepository.update(config);

  config.setManualExport(true);
  fileExportRepository.update(config);
  processor.runPendingTasks();

  EXPECT_EQ(fileExportRepository.get().getExportError(), ExportError::kNoError);

  config = fileExportRepository.get();
  config.setManualExport(false);
  fileExportRepository.update(config);
  processor.runPendingTasks();

  EXPECT_TRUE(std::filesystem::exists(kOutPath));
  std::filesystem::remove(kOutPath);
}

// Regression test for a subtler hazard than the deadlock above: the real
// export button handler (rendererplugin/src/screens/FileExportScreen.cpp)
// does `config = repo.get(); config.setManualExport(!...); repo.update(config)`
// -- a read-modify-write of a FULL FileExport snapshot. Before
// deferRepositoryUpdate(), a synchronous classification write issued from
// inside the listener callback this triggers got silently overwritten
// moments later, when that same caller's copyPropertiesFrom loop went on to
// reapply its own stale (pre-attempt) exportError property (FileExport.h
// declares manualExport before exportError, so the loop processes
// exportError second, in every call). This test reproduces that exact
// caller pattern across a failed attempt followed by a successful retry,
// and asserts the retry's success is neither clobbered by, nor leaks, the
// prior failure.
TEST_F(WavFileOutputTests, retry_after_failure_does_not_clobber_or_leak_error) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_retry_test.wav";
  std::filesystem::remove(kOutPath);

  // First attempt: invalid path, fails.
  FileExport config = fileExportRepository.get();
  config.setAudioFileFormat(AudioFileFormat::WAV);
  config.setExportAudio(true);
  config.setSampleRate(48000);
  config.setBitDepth(16);
  config.setAudioCodec(AudioCodec::LPCM);
  config.setExportFile("/invalid_path/test.wav");
  config.setManualExport(false);
  fileExportRepository.update(config);

  config.setManualExport(true);
  fileExportRepository.update(config);
  processor.runPendingTasks();
  EXPECT_NE(fileExportRepository.get().getExportError(), ExportError::kNoError);

  config = fileExportRepository.get();
  config.setManualExport(false);
  fileExportRepository.update(config);
  processor.runPendingTasks();

  // Second attempt, mirroring FileExportScreen.cpp's export button handler
  // exactly: the snapshot captured here still holds the failed ExportError
  // from the first attempt, before flipping ManualExport and writing it
  // back with everything else it doesn't intend to change.
  config = fileExportRepository.get();
  config.setExportFile(kOutPath.string());
  config.setManualExport(!config.getManualExport());
  fileExportRepository.update(config);
  processor.runPendingTasks();

  EXPECT_EQ(fileExportRepository.get().getExportError(), ExportError::kNoError);

  config = fileExportRepository.get();
  config.setManualExport(false);
  fileExportRepository.update(config);
  processor.runPendingTasks();

  EXPECT_TRUE(std::filesystem::exists(kOutPath));
  std::filesystem::remove(kOutPath);
}

// Regression for recordWriteFailureIfAny() used to call deferRepositoryUpdate()
// unconditionally on every failing processBlock, ensuring
// persistent failures don't queue more than once. This reproduces the
// open-succeeds-then-every-write-fails shape via FakeFileWriter and asserts
// exactly one closure is queued no matter how many blocks fail.
TEST_F(WavFileOutputTests,
       many_failed_writes_after_successful_open_queue_a_single_update) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_write_failure_test.wav";
  std::filesystem::remove(kOutPath);

  FileExport config = fileExportRepository.get();
  config.setAudioFileFormat(AudioFileFormat::WAV);
  config.setExportAudio(true);
  config.setSampleRate(48000);
  config.setBitDepth(16);
  config.setAudioCodec(AudioCodec::LPCM);
  config.setExportFile(kOutPath.string());
  config.setManualExport(false);
  fileExportRepository.update(config);

  processor.forceWriteFailure = true;
  config.setManualExport(true);
  fileExportRepository.update(config);
  processor.runPendingTasks();  // Drain the export-start clean-slate reset.
  EXPECT_EQ(fileExportRepository.get().getExportError(), ExportError::kNoError);
  ASSERT_NE(processor.lastFakeWriter, nullptr);
  EXPECT_TRUE(processor.lastFakeWriter->isOpen());

  juce::AudioBuffer<float> buffer(2, 64);
  buffer.clear();
  juce::MidiBuffer midi;
  for (int i = 0; i < 5; ++i) {
    processor.processBlock(buffer, midi);
  }

  EXPECT_EQ(processor.lastFakeWriter->writeCallCount, 5);
  // The fix under test: 5 failing blocks must queue exactly one
  // deferRepositoryUpdate() closure, not one per block.
  EXPECT_EQ(processor.pendingTaskCount(), 1u);

  processor.runPendingTasks();
  EXPECT_EQ(fileExportRepository.get().getExportError(),
            ExportError::kFileWriteFailed);

  config = fileExportRepository.get();
  config.setManualExport(false);
  fileExportRepository.update(config);
  processor.runPendingTasks();
  std::filesystem::remove(kOutPath);
}

// Companion regression test covering the sibling untested branch flagged in
// the same review: a close() failure after a successful open and successful
// writes must still be recorded as kFileWriteFailed.
TEST_F(WavFileOutputTests, close_failure_after_successful_open_is_recorded) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_close_failure_test.wav";
  std::filesystem::remove(kOutPath);

  FileExport config = fileExportRepository.get();
  config.setAudioFileFormat(AudioFileFormat::WAV);
  config.setExportAudio(true);
  config.setSampleRate(48000);
  config.setBitDepth(16);
  config.setAudioCodec(AudioCodec::LPCM);
  config.setExportFile(kOutPath.string());
  config.setManualExport(false);
  fileExportRepository.update(config);

  processor.forceCloseFailure = true;
  config.setManualExport(true);
  fileExportRepository.update(config);
  processor.runPendingTasks();
  EXPECT_EQ(fileExportRepository.get().getExportError(), ExportError::kNoError);

  juce::AudioBuffer<float> buffer(2, 64);
  buffer.clear();
  juce::MidiBuffer midi;
  processor.processBlock(buffer, midi);  // A successful write.

  config = fileExportRepository.get();
  config.setManualExport(false);
  fileExportRepository.update(config);
  processor.runPendingTasks();

  EXPECT_EQ(fileExportRepository.get().getExportError(),
            ExportError::kFileWriteFailed);
  std::filesystem::remove(kOutPath);
}

namespace {
using ChannelType = juce::AudioChannelSet::ChannelType;

// Fills rendered channel i with this value, so each written channel shows
// which rendered channel it came from.
float ituMarker(int ituIndex) { return (ituIndex + 1) / 32.f; }

std::unique_ptr<juce::AudioFormatReader> readWav(
    const std::filesystem::path& path) {
  return std::unique_ptr<juce::AudioFormatReader>(
      juce::WavAudioFormat().createReaderFor(
          new juce::FileInputStream(juce::File(path.string())), true));
}

struct RoomWavCase {
  Speakers::AudioElementSpeakerLayout layout;
  std::vector<ChannelType> expectedSet;
  // Rendered (ITU order) channel expected in each written channel.
  std::vector<int> expectedSources;
};

// Keeps the test names ctest registers free of the parameter's raw bytes.
void PrintTo(const RoomWavCase& roomWavCase, std::ostream* os) {
  *os << roomWavCase.layout.toString();
}

const ChannelType L = ChannelType::left, R = ChannelType::right,
                  C = ChannelType::centre, LFE = ChannelType::LFE,
                  Ls = ChannelType::leftSurround,
                  Rs = ChannelType::rightSurround,
                  Lss = ChannelType::leftSurroundSide,
                  Rss = ChannelType::rightSurroundSide,
                  Ltf = ChannelType::topFrontLeft,
                  Rtf = ChannelType::topFrontRight,
                  Ltr = ChannelType::topRearLeft,
                  Rtr = ChannelType::topRearRight;

juce::AudioChannelSet makeSet(const std::vector<ChannelType>& types) {
  juce::AudioChannelSet set;
  for (const ChannelType kType : types) set.addChannel(kType);
  return set;
}

class WavFileOutputRoomOrder
    : public WavFileOutputTests,
      public ::testing::WithParamInterface<RoomWavCase> {};
}  // namespace

// Room WAV writes each room layout's rendered ITU channels in the export set's
// getChannelTypes() order, with that set as the file's channel mask.
TEST_P(WavFileOutputRoomOrder, writes_channels_in_export_set_order) {
  const RoomWavCase& kCase = GetParam();
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() /
      ("wav_room_order_" + kCase.layout.toString().toStdString() + ".wav");
  std::filesystem::remove(kOutPath);

  setRoomLayout(roomSetupRepository, kCase.layout);
  ASSERT_EQ(roomSetupRepository.get().getSpeakerLayout().getRoomSpeakerLayout(),
            kCase.layout);

  FileExport config = fileExportRepository.get();
  config.setAudioFileFormat(AudioFileFormat::WAV);
  config.setExportAudio(true);
  config.setSampleRate(48000);
  config.setBitDepth(24);
  config.setAudioCodec(AudioCodec::LPCM);
  config.setExportFile(kOutPath.string());
  config.setManualExport(false);
  fileExportRepository.update(config);
  processor.prepareToPlay(48000, 64);

  config.setManualExport(true);
  fileExportRepository.update(config);
  processor.runPendingTasks();

  const int kNumChannels = static_cast<int>(kCase.expectedSources.size());
  juce::AudioBuffer<float> buffer(kNumChannels, 64);
  for (int ch = 0; ch < kNumChannels; ++ch) {
    juce::FloatVectorOperations::fill(buffer.getWritePointer(ch), ituMarker(ch),
                                      buffer.getNumSamples());
  }
  juce::MidiBuffer midi;
  processor.processBlock(buffer, midi);

  config = fileExportRepository.get();
  config.setManualExport(false);
  fileExportRepository.update(config);
  processor.runPendingTasks();
  EXPECT_EQ(fileExportRepository.get().getExportError(), ExportError::kNoError);

  auto reader = readWav(kOutPath);
  ASSERT_NE(reader, nullptr);
  ASSERT_EQ(static_cast<int>(reader->numChannels), kNumChannels);
  EXPECT_EQ(reader->getChannelLayout(), makeSet(kCase.expectedSet));
  juce::AudioBuffer<float> readBack(kNumChannels, 64);
  reader->read(&readBack, 0, 64, 0, true, true);
  for (int ch = 0; ch < kNumChannels; ++ch) {
    EXPECT_NEAR(readBack.getSample(ch, 10),
                ituMarker(kCase.expectedSources[ch]), 1e-4f)
        << "written channel " << ch;
  }
  reader.reset();
  std::filesystem::remove(kOutPath);
}

INSTANTIATE_TEST_SUITE_P(
    RoomLayouts, WavFileOutputRoomOrder,
    ::testing::Values(
        RoomWavCase{Speakers::kStereo, {L, R}, {0, 1}},
        RoomWavCase{Speakers::k3Point1Point2,
                    {L, R, C, LFE, Ltf, Rtf},
                    {0, 1, 2, 3, 4, 5}},
        RoomWavCase{
            Speakers::k5Point1, {L, R, C, LFE, Ls, Rs}, {0, 1, 2, 3, 4, 5}},
        RoomWavCase{Speakers::k5Point1Point2,
                    {L, R, C, LFE, Ls, Rs, Ltf, Rtf},
                    {0, 1, 2, 3, 4, 5, 6, 7}},
        RoomWavCase{Speakers::k5Point1Point4,
                    {L, R, C, LFE, Ls, Rs, Ltf, Rtf, Ltr, Rtr},
                    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
        RoomWavCase{Speakers::k7Point1,
                    {L, R, C, LFE, Ls, Rs, Lss, Rss},
                    {0, 1, 2, 3, 6, 7, 4, 5}},
        RoomWavCase{Speakers::k7Point1Point2,
                    {L, R, C, LFE, Ls, Rs, Lss, Rss, Ltf, Rtf},
                    {0, 1, 2, 3, 6, 7, 4, 5, 8, 9}},
        RoomWavCase{Speakers::k7Point1Point4,
                    {L, R, C, LFE, Ls, Rs, Lss, Rss, Ltf, Rtf, Ltr, Rtr},
                    {0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10, 11}}),
    [](const ::testing::TestParamInfo<RoomWavCase>& info) {
      return info.param.layout.toString()
          .replaceCharacter('.', 'p')
          .retainCharacters(
              "0123456789abcdefghijklmnopqrstuvwxyz"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZp")
          .toStdString();
    });

// FileWriter writes the WAVE-mapped 5.1.2, 7.1, 7.1.2 and 7.1.4 sets, and a
// reader recovers each set from the file's channel mask.
TEST(WavFileOutputFileWriter, wave_mapped_sets_read_back_from_channel_mask) {
  const std::vector<std::vector<ChannelType>> kSets = {
      {L, R, C, LFE, Ls, Rs, Ltf, Rtf},
      {L, R, C, LFE, Ls, Rs, Lss, Rss},
      {L, R, C, LFE, Ls, Rs, Lss, Rss, Ltf, Rtf},
      {L, R, C, LFE, Ls, Rs, Lss, Rss, Ltf, Rtf, Ltr, Rtr}};
  for (const auto& kTypes : kSets) {
    const juce::AudioChannelSet kSet = makeSet(kTypes);
    const std::filesystem::path kOutPath =
        std::filesystem::current_path() /
        ("wav_mask_" + std::to_string(kSet.size()) + ".wav");
    std::filesystem::remove(kOutPath);
    {
      FileWriter writer(kOutPath.string(), 48000, kSet, 0, 24,
                        AudioCodec::LPCM);
      ASSERT_TRUE(writer.isOpen()) << kSet.getDescription();
      juce::AudioBuffer<float> buffer(kSet.size(), 64);
      buffer.clear();
      EXPECT_TRUE(writer.write(buffer));
      EXPECT_TRUE(writer.close());
    }
    auto reader = readWav(kOutPath);
    ASSERT_NE(reader, nullptr);
    EXPECT_EQ(reader->getChannelLayout(), kSet) << kSet.getDescription();
    reader.reset();
    std::filesystem::remove(kOutPath);
  }
}

// FileWriter::write refuses a buffer with fewer channels than
// firstChannel + numChannels instead of reading past it.
TEST(WavFileOutputFileWriter,
     write_rejects_buffer_narrower_than_channel_range) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_narrow_buffer.wav";
  std::filesystem::remove(kOutPath);
  FileWriter writer(kOutPath.string(), 48000, juce::AudioChannelSet::stereo(),
                    1, 24, AudioCodec::LPCM);
  ASSERT_TRUE(writer.isOpen());
  juce::AudioBuffer<float> buffer(2, 64);
  buffer.clear();
  EXPECT_FALSE(writer.write(buffer));
  EXPECT_EQ(writer.getFramesWritten(), 0);
  writer.close();
  std::filesystem::remove(kOutPath);
}

namespace {
class NullHostProcessor final : public ProcessorBase {
 public:
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
};

constexpr int kBinauralBlockSize = 128;
constexpr int kBinauralBlocks = 4;
constexpr float kMixGain = 0.5f;

// A RenderProcessor with its own repositories, rendering a stereo element in
// binaural headphone mode and a 5.1 element in stereo headphone mode.
struct RenderRig {
  explicit RenderRig(Speakers::AudioElementSpeakerLayout roomLayout,
                     float mixGain = kMixGain)
      : state("rig"),
        roomSetup(
            state.getOrCreateChildWithName(RoomSetup::kTreeType, nullptr)),
        audioElements(
            state.getOrCreateChildWithName(AudioElement::kTreeType, nullptr)),
        mixPresentations(state.getOrCreateChildWithName(
            MixPresentation::kTreeType, nullptr)),
        activeMix(state.getOrCreateChildWithName(
            ActiveMixPresentation::kTreeType, nullptr)),
        renderer(&host, &roomSetup, &audioElements, &mixPresentations,
                 &activeMix, monitorData) {
    setRoomLayout(roomSetup, roomLayout);
    const AudioElement kStereo(juce::Uuid(), "Stereo", Speakers::kStereo, 0);
    const AudioElement kSurround(juce::Uuid(), "5.1", Speakers::k5Point1, 2);
    audioElements.add(kStereo);
    audioElements.add(kSurround);
    const juce::Uuid kMixId;
    MixPresentation mix(kMixId, "Mix", mixGain,
                        LanguageData::MixLanguages::English, {});
    mix.addAudioElement(kStereo.getId(), 1.f, kStereo.getName(), true);
    mix.addAudioElement(kSurround.getId(), 1.f, kSurround.getName(), false);
    mixPresentations.updateOrAdd(mix);
    ActiveMixPresentation active = activeMix.get();
    active.updateActiveMixId(kMixId);
    activeMix.update(active);
    renderer.prepareToPlay(48000, kBinauralBlockSize);
  }

  juce::ValueTree state;
  RoomSetupRepository roomSetup;
  AudioElementRepository audioElements;
  MixPresentationRepository mixPresentations;
  ActiveMixRepository activeMix;
  SpeakerMonitorData monitorData;
  NullHostProcessor host;
  RenderProcessor renderer;
};

// Input for block `block`: a distinct sine on each of the 8 element channels.
juce::AudioBuffer<float> elementInput(int block) {
  juce::AudioBuffer<float> buffer(12, kBinauralBlockSize);
  buffer.clear();
  for (int ch = 0; ch < 8; ++ch) {
    for (int i = 0; i < kBinauralBlockSize; ++i) {
      const int kSample = block * kBinauralBlockSize + i;
      buffer.setSample(
          ch, i,
          0.2f * std::sin(0.01f * static_cast<float>((ch + 1) * kSample)));
    }
  }
  return buffer;
}

class WavFileOutputBinaural : public ::testing::Test {
 protected:
  // Runs a BinauralWav export of kBinauralBlocks blocks with `rig`'s room,
  // feeding each block through the renderer first as RendererProcessor does.
  void exportBinaural(RenderRig& rig, const std::filesystem::path& outPath) {
    TestFileExportRepository fileExportRepository;
    TestableWavFileOutputProcessor processor(fileExportRepository,
                                             rig.roomSetup, rig.renderer);
    processor.prepareToPlay(48000, kBinauralBlockSize);
    FileExport config = fileExportRepository.get();
    config.setAudioFileFormat(AudioFileFormat::BinauralWav);
    config.setExportAudio(true);
    config.setSampleRate(48000);
    config.setBitDepth(32);
    config.setAudioCodec(AudioCodec::LPCM);
    config.setExportFile(outPath.string());
    config.setManualExport(false);
    fileExportRepository.update(config);
    config.setManualExport(true);
    fileExportRepository.update(config);
    processor.runPendingTasks();

    juce::MidiBuffer midi;
    for (int block = 0; block < kBinauralBlocks; ++block) {
      juce::AudioBuffer<float> buffer = elementInput(block);
      rig.renderer.processBlock(buffer, midi);
      processor.processBlock(buffer, midi);
    }

    config = fileExportRepository.get();
    config.setManualExport(false);
    fileExportRepository.update(config);
    processor.runPendingTasks();
    EXPECT_EQ(fileExportRepository.get().getExportError(),
              ExportError::kNoError);
  }
};
}  // namespace

// A BinauralWav export from a 7.1.4 room writes, for every block, the same 2
// channels a renderer with the room set to Binaural plays back: the binaural
// mix times the mix gain.
TEST_F(WavFileOutputBinaural, matches_binaural_room_playback_every_block) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_binaural_714.wav";
  std::filesystem::remove(kOutPath);
  RenderRig exportRig(Speakers::k7Point1Point4);
  exportBinaural(exportRig, kOutPath);

  RenderRig binauralRoom(Speakers::kBinaural);
  RenderRig unityGainBinauralRoom(Speakers::kBinaural, 1.f);
  juce::AudioBuffer<float> expected(2, kBinauralBlocks * kBinauralBlockSize);
  juce::AudioBuffer<float> unityGain(2, kBinauralBlocks * kBinauralBlockSize);
  juce::MidiBuffer midi;
  for (int block = 0; block < kBinauralBlocks; ++block) {
    juce::AudioBuffer<float> buffer = elementInput(block);
    binauralRoom.renderer.processBlock(buffer, midi);
    juce::AudioBuffer<float> unityBuffer = elementInput(block);
    unityGainBinauralRoom.renderer.processBlock(unityBuffer, midi);
    for (int ch = 0; ch < 2; ++ch) {
      expected.copyFrom(ch, block * kBinauralBlockSize, buffer, ch, 0,
                        kBinauralBlockSize);
      unityGain.copyFrom(ch, block * kBinauralBlockSize, unityBuffer, ch, 0,
                         kBinauralBlockSize);
    }
  }

  auto reader = readWav(kOutPath);
  ASSERT_NE(reader, nullptr);
  ASSERT_EQ(reader->numChannels, 2u);
  ASSERT_EQ(reader->lengthInSamples, kBinauralBlocks * kBinauralBlockSize);
  juce::AudioBuffer<float> written(2, kBinauralBlocks * kBinauralBlockSize);
  reader->read(&written, 0, written.getNumSamples(), 0, true, true);
  float peak = 0.f;
  for (int ch = 0; ch < 2; ++ch) {
    for (int i = 0; i < written.getNumSamples(); ++i) {
      ASSERT_FLOAT_EQ(written.getSample(ch, i), expected.getSample(ch, i))
          << "channel " << ch << " block " << i / kBinauralBlockSize;
      ASSERT_NEAR(written.getSample(ch, i),
                  kMixGain * unityGain.getSample(ch, i), 1e-6f)
          << "channel " << ch << " block " << i / kBinauralBlockSize;
      peak = std::max(peak, std::abs(expected.getSample(ch, i)));
    }
  }
  EXPECT_GT(peak, 0.f);
  reader.reset();
  std::filesystem::remove(kOutPath);
}

// A BinauralWav export leaves the room setup untouched.
TEST_F(WavFileOutputBinaural, leaves_room_setup_unchanged) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_binaural_room.wav";
  std::filesystem::remove(kOutPath);
  RenderRig rig(Speakers::k7Point1Point4);
  const juce::ValueTree kBefore =
      rig.state.getChildWithName(RoomSetup::kTreeType).createCopy();
  exportBinaural(rig, kOutPath);
  EXPECT_TRUE(
      rig.state.getChildWithName(RoomSetup::kTreeType).isEquivalentTo(kBefore));
  std::filesystem::remove(kOutPath);
}

// BinauralWav writes a 2-channel stereo file whatever the room layout.
TEST_F(WavFileOutputBinaural, writes_two_channels_for_any_room) {
  for (const auto kLayout :
       {Speakers::kStereo, Speakers::k5Point1, Speakers::k7Point1Point4}) {
    const std::filesystem::path kOutPath =
        std::filesystem::current_path() /
        ("wav_binaural_" + kLayout.toString().toStdString() + ".wav");
    std::filesystem::remove(kOutPath);
    RenderRig rig(kLayout);
    exportBinaural(rig, kOutPath);
    auto reader = readWav(kOutPath);
    ASSERT_NE(reader, nullptr) << kLayout.toString();
    EXPECT_EQ(reader->numChannels, 2u) << kLayout.toString();
    EXPECT_EQ(reader->getChannelLayout(), juce::AudioChannelSet::stereo())
        << kLayout.toString();
    reader.reset();
    std::filesystem::remove(kOutPath);
  }
}

namespace {
constexpr int kRangeBlockSize = 64;
constexpr double kRangeSampleRate = 48000.0;

class WavFileOutputRange : public WavFileOutputTests {
 protected:
  void SetUp() override {
    setRoomLayout(roomSetupRepository, Speakers::kStereo);
  }

  // Runs a Room WAV export of `numBlocks` blocks with no playhead set, as in a
  // host, and returns the written file's length in frames.
  juce::int64 exportBlocks(const std::filesystem::path& outPath,
                           juce::int64 startSampleIdx, juce::int64 endSampleIdx,
                           int numBlocks) {
    std::filesystem::remove(outPath);
    FileExport config = fileExportRepository.get();
    config.setAudioFileFormat(AudioFileFormat::WAV);
    config.setExportAudio(true);
    config.setSampleRate(static_cast<int>(kRangeSampleRate));
    config.setBitDepth(16);
    config.setAudioCodec(AudioCodec::LPCM);
    config.setExportFile(outPath.string());
    config.setStartSampleIdx(startSampleIdx);
    config.setEndSampleIdx(endSampleIdx);
    config.setManualExport(false);
    fileExportRepository.update(config);
    processor.prepareToPlay(kRangeSampleRate, kRangeBlockSize);

    config.setManualExport(true);
    fileExportRepository.update(config);
    processor.runPendingTasks();

    juce::AudioBuffer<float> buffer(2, kRangeBlockSize);
    buffer.clear();
    juce::MidiBuffer midi;
    for (int block = 0; block < numBlocks; ++block) {
      processor.processBlock(buffer, midi);
    }

    config = fileExportRepository.get();
    config.setManualExport(false);
    fileExportRepository.update(config);
    processor.runPendingTasks();
    EXPECT_EQ(fileExportRepository.get().getExportError(),
              ExportError::kNoError);

    auto reader = readWav(outPath);
    const juce::int64 kLength =
        reader == nullptr ? -1 : reader->lengthInSamples;
    reader.reset();
    std::filesystem::remove(outPath);
    return kLength;
  }
};
}  // namespace

// With a start and an end set, only the blocks that start inside the range
// reach the file.
TEST_F(WavFileOutputRange, writes_only_blocks_inside_start_and_end) {
  const juce::int64 kLength =
      exportBlocks(std::filesystem::current_path() / "wav_range_start_end.wav",
                   2 * kRangeBlockSize, 6 * kRangeBlockSize, 10);
  EXPECT_EQ(kLength, 4 * kRangeBlockSize);
}

// With a nonzero start and no end, the export writes the frames from the
// start onward rather than nothing.
TEST_F(WavFileOutputRange, writes_from_nonzero_start_without_end) {
  const juce::int64 kLength =
      exportBlocks(std::filesystem::current_path() / "wav_range_start.wav",
                   3 * kRangeBlockSize, 0, 10);
  EXPECT_EQ(kLength, 7 * kRangeBlockSize);
}

// With a zero start and an end set, blocks at or past the end are dropped.
TEST_F(WavFileOutputRange, stops_writing_at_end_with_zero_start) {
  const juce::int64 kLength =
      exportBlocks(std::filesystem::current_path() / "wav_range_end.wav", 0,
                   5 * kRangeBlockSize, 10);
  EXPECT_EQ(kLength, 5 * kRangeBlockSize);
}

// With no start and no end, every block reaches the file.
TEST_F(WavFileOutputRange, writes_every_block_without_range) {
  const juce::int64 kLength = exportBlocks(
      std::filesystem::current_path() / "wav_range_none.wav", 0, 0, 10);
  EXPECT_EQ(kLength, 10 * kRangeBlockSize);
}

// A second export counts samples from its own start, not from where the
// previous export stopped.
TEST_F(WavFileOutputRange, restarts_sample_count_for_each_export) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_range_repeat.wav";
  const juce::int64 kFirst =
      exportBlocks(kOutPath, 2 * kRangeBlockSize, 6 * kRangeBlockSize, 10);
  const juce::int64 kSecond =
      exportBlocks(kOutPath, 2 * kRangeBlockSize, 6 * kRangeBlockSize, 10);
  EXPECT_EQ(kFirst, 4 * kRangeBlockSize);
  EXPECT_EQ(kSecond, 4 * kRangeBlockSize);
}

namespace {
constexpr int kBounceBlockSize = 64;
constexpr int kBounceBlocks = 6;

class WavFileOutputHostBounce : public WavFileOutputTests {
 protected:
  // Configures an export of `format` to `path` with manual export off, as a
  // host bounce leaves it, and a stale error from an earlier export.
  void configureExport(AudioFileFormat format,
                       const std::filesystem::path& path) {
    setRoomLayout(roomSetupRepository, Speakers::k5Point1);
    FileExport config = fileExportRepository.get();
    config.setAudioFileFormat(format);
    config.setExportAudio(true);
    config.setSampleRate(48000);
    config.setBitDepth(24);
    config.setAudioCodec(AudioCodec::LPCM);
    config.setExportFile(path.string());
    config.setManualExport(false);
    config.setExportError(kFileWriteFailed);
    fileExportRepository.update(config);
    processor.prepareToPlay(48000, kBounceBlockSize);
  }

  void setExportPath(const std::filesystem::path& path) {
    FileExport config = fileExportRepository.get();
    config.setExportFile(path.string());
    fileExportRepository.update(config);
  }

  // Bounces kBounceBlocks blocks through setNonRealtime, editing the export
  // path and draining the start's error reset between blocks.
  void bounceWithExportEdits(const std::filesystem::path& editedPath) {
    processor.setNonRealtime(true);
    juce::AudioBuffer<float> buffer(6, kBounceBlockSize);
    buffer.clear();
    juce::MidiBuffer midi;
    for (int block = 0; block < kBounceBlocks; ++block) {
      processor.processBlock(buffer, midi);
      if (block == 1) {
        setExportPath(editedPath);
      } else if (block == 3) {
        processor.runPendingTasks();
      }
    }
    processor.setNonRealtime(false);
    processor.runPendingTasks();
  }
};
}  // namespace

// Repeated setNonRealtime(true) keeps the one open writer, and
// setNonRealtime(false) while idle creates no file.
TEST_F(WavFileOutputHostBounce, set_non_realtime_follows_its_argument) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_bounce_idempotent.wav";
  std::filesystem::remove(kOutPath);
  configureExport(AudioFileFormat::WAV, kOutPath);

  processor.setNonRealtime(false);
  EXPECT_EQ(processor.writersCreated, 0);
  EXPECT_FALSE(std::filesystem::exists(kOutPath));

  processor.setNonRealtime(true);
  processor.setNonRealtime(true);
  EXPECT_EQ(processor.writersCreated, 1);
  ASSERT_NE(processor.lastFakeWriter, nullptr);
  EXPECT_TRUE(processor.lastFakeWriter->isOpen());

  processor.setNonRealtime(false);
  processor.setNonRealtime(false);
  EXPECT_EQ(processor.writersCreated, 1);
  processor.runPendingTasks();
  std::filesystem::remove(kOutPath);
}

// Path and error updates during a host bounce leave the writer open, so the
// Room WAV holds every bounced frame at the path the bounce started with.
TEST_F(WavFileOutputHostBounce, room_wav_survives_export_edits) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_bounce_room.wav";
  const std::filesystem::path kEditedPath =
      std::filesystem::current_path() / "wav_bounce_room_edited.wav";
  std::filesystem::remove(kOutPath);
  std::filesystem::remove(kEditedPath);
  configureExport(AudioFileFormat::WAV, kOutPath);

  bounceWithExportEdits(kEditedPath);

  EXPECT_EQ(processor.writersCreated, 1);
  EXPECT_EQ(fileExportRepository.get().getExportError(), ExportError::kNoError);
  EXPECT_FALSE(std::filesystem::exists(kEditedPath));
  auto reader = readWav(kOutPath);
  ASSERT_NE(reader, nullptr);
  EXPECT_EQ(reader->numChannels, 6u);
  EXPECT_EQ(reader->lengthInSamples, kBounceBlocks * kBounceBlockSize);
  reader.reset();
  std::filesystem::remove(kOutPath);
}

// A Binaural WAV host bounce also holds every bounced frame despite edits.
TEST_F(WavFileOutputHostBounce, binaural_wav_survives_export_edits) {
  const std::filesystem::path kOutPath =
      std::filesystem::current_path() / "wav_bounce_binaural.wav";
  const std::filesystem::path kEditedPath =
      std::filesystem::current_path() / "wav_bounce_binaural_edited.wav";
  std::filesystem::remove(kOutPath);
  std::filesystem::remove(kEditedPath);
  configureExport(AudioFileFormat::BinauralWav, kOutPath);

  bounceWithExportEdits(kEditedPath);

  EXPECT_EQ(processor.writersCreated, 1);
  EXPECT_FALSE(std::filesystem::exists(kEditedPath));
  auto reader = readWav(kOutPath);
  ASSERT_NE(reader, nullptr);
  EXPECT_EQ(reader->numChannels, 2u);
  EXPECT_EQ(reader->lengthInSamples, kBounceBlocks * kBounceBlockSize);
  reader.reset();
  std::filesystem::remove(kOutPath);
}

// After the host reports realtime playback, editing the export path while
// idle creates no file at the old or the new path.
TEST_F(WavFileOutputHostBounce, idle_path_edits_create_no_file) {
  const std::filesystem::path kOldPath =
      std::filesystem::current_path() / "wav_idle_old.wav";
  const std::filesystem::path kNewPath =
      std::filesystem::current_path() / "wav_idle_new.wav";
  std::filesystem::remove(kOldPath);
  std::filesystem::remove(kNewPath);
  configureExport(AudioFileFormat::WAV, kOldPath);

  processor.setNonRealtime(false);
  setExportPath(kNewPath);
  setExportPath(kOldPath);
  setExportPath(kNewPath);
  processor.runPendingTasks();

  EXPECT_EQ(processor.writersCreated, 0);
  EXPECT_FALSE(std::filesystem::exists(kOldPath));
  EXPECT_FALSE(std::filesystem::exists(kNewPath));
  processor.setNonRealtime(false);
  std::filesystem::remove(kOldPath);
  std::filesystem::remove(kNewPath);
}
