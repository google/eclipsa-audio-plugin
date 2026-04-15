#include "processors/file_playback/FilePlaybackProcessor.h"

#include <gtest/gtest.h>

#include <chrono>

#include "data_structures/src/FilePlayback.h"
#include "processors/tests/FileOutputTestFixture.h"

class FilePlaybackProcessorTest : public FileOutputTests {
 protected:
  FilePlaybackProcessorTest() : FileOutputTests() {
    proc = std::make_unique<FilePlaybackProcessor>(fpbr, fpbData);
    setExportCompleted(true);
    proc->reinitializeAfterStateRestore();
    proc->prepareToPlay(48e3, 512);
    buff = juce::AudioBuffer<float>(2, 256);
    buff.clear();
  }

  // Helper methods to issue commands and progress state machine
  void setFile(const juce::String& path) {
    fpb = fpbr.get();
    fpb.setPlaybackFile(path);
    fpbr.update(fpb);
  }

  void setCommand(FilePlayback::PlaybackCommand cmd) {
    fpb = fpbr.get();
    fpb.setPlaybackCommand(cmd);
    fpbr.update(fpb);
  }

  void setSeek(float position) {
    fpb = fpbr.get();
    fpb.setSeekPosition(position);
    fpbr.update(fpb);
  }

  void setLayout(Speakers::AudioElementSpeakerLayout layout) {
    fpb = fpbr.get();
    fpb.setReqdDecodeLayout(layout);
    fpbr.update(fpb);
  }

  void setExportCompleted(bool completed) {
    fe = fileExportRepository.get();
    fe.setExportCompleted(completed);
    fileExportRepository.update(fe);
  }

  void waitForBuffering() {
    fpbData.processorState.read(procState);
    const std::chrono::seconds kMaxWaitTime(30);
    const std::chrono::milliseconds kWaitDuration(10);
    auto startTime = std::chrono::steady_clock::now();

    while (procState == FilePlayback::ProcessorState::kBuffering) {
      auto elapsed = std::chrono::steady_clock::now() - startTime;
      if (elapsed > kMaxWaitTime) {
        FAIL() << "Buffering timeout exceeded 30 seconds";
      }
      std::this_thread::sleep_for(kWaitDuration);
      fpbData.processorState.read(procState);
    }
  }

  FilePlayback fpb;
  FileExport fe;
  FilePlayback::ProcessorState procState;
  FilePlaybackProcessorData fpbData;
  std::unique_ptr<FilePlaybackProcessor> proc;
  juce::AudioBuffer<float> buff;
  juce::MidiBuffer mbuff;
};

// By default expect no audio through the processor
TEST_F(FilePlaybackProcessorTest, defaults) {
  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());
}

// The template for the remaining tests:
// - issue a command
// - check the state resulting from the command
// - poll and wait to validate the result if it's a non-deterministic command

// Issue a file update then a play command with no file set
TEST_F(FilePlaybackProcessorTest, no_file) {
  setFile("");
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());
}

// Expect no audio when an invalid path is set
TEST_F(FilePlaybackProcessorTest, invalid_path) {
  setFile("/this/path/does/not/exist.iamf");
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());
}

// Expect no audio when the IAMF file is invalid
TEST_F(FilePlaybackProcessorTest, invalid_iamf) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path().parent_path() /
      "common/processors/tests/test_resources" / "Invalid.iamf";
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kError);

  // Buffer empty in error state
  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());
}

// Expect audio when file and playback enabled
TEST_F(FilePlaybackProcessorTest, play_valid_file) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createBasicIAMFFile(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPlaying);

  // Verify buffer properties
  const int kNumChannels = buff.getNumChannels();
  const int kNumSamples = buff.getNumSamples();

  proc->processBlock(buff, mbuff);

  EXPECT_FALSE(buff.hasBeenCleared());
  EXPECT_EQ(buff.getNumChannels(), kNumChannels);
  EXPECT_EQ(buff.getNumSamples(), kNumSamples);
}

// Expect no audio when file and playback paused
TEST_F(FilePlaybackProcessorTest, pause_valid_file) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createBasicIAMFFile(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPlaying);

  setCommand(FilePlayback::PlaybackCommand::kPause);

  buff.clear();
  proc->processBlock(buff, mbuff);
  // Verify buffer properties
  const int kNumChannels = buff.getNumChannels();
  const int kNumSamples = buff.getNumSamples();
  EXPECT_TRUE(buff.hasBeenCleared());
  EXPECT_EQ(buff.getNumChannels(), kNumChannels);
  EXPECT_EQ(buff.getNumSamples(), kNumSamples);
}

// Expect no audio when file and playback stopped
TEST_F(FilePlaybackProcessorTest, stop_valid_file) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createBasicIAMFFile(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPlaying);

  proc->processBlock(buff, mbuff);
  EXPECT_FALSE(buff.hasBeenCleared());

  setCommand(FilePlayback::PlaybackCommand::kStop);
  waitForBuffering();

  // Verify buffer properties
  buff.clear();
  proc->processBlock(buff, mbuff);
  const int kNumChannels = buff.getNumChannels();
  const int kNumSamples = buff.getNumSamples();

  EXPECT_TRUE(buff.hasBeenCleared());
  EXPECT_EQ(buff.getNumChannels(), kNumChannels);
  EXPECT_EQ(buff.getNumSamples(), kNumSamples);
}

// Expect playing audio updates the current file position
TEST_F(FilePlaybackProcessorTest, play_updates_file_position) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPlaying);

  // Call process block and verify each file position is > the last
  float lastFilePosition = 0.0f;
  fpbData.currFilePosition.read(lastFilePosition);
  for (int i = 0; i < 100; ++i) {
    proc->processBlock(buff, mbuff);
    float currentFilePosition;
    fpbData.currFilePosition.read(currentFilePosition);
    ASSERT_GT(currentFilePosition, lastFilePosition);
    lastFilePosition = currentFilePosition;
  }
}

// Expect changing the file during playback sequences processor states as
// expected
TEST_F(FilePlaybackProcessorTest, change_file_during_playback) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  const std::filesystem::path kNewReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor_new.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  createIAMFFile30SecStereo(kNewReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPlaying);

  proc->processBlock(buff, mbuff);
  EXPECT_FALSE(buff.hasBeenCleared());

  // When we change the file, we should read the buffering state,
  // processBlock
  // should produce no audio, and then we should return to stopped after
  // buffering
  buff.clear();
  setFile(juce::String(kNewReferenceFilePath.string()));

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kBuffering);

  buff.clear();
  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());

  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);
}

// Expect issuing other commands while in buffering state to be ignored
TEST_F(FilePlaybackProcessorTest, commands_while_buffering) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kBuffering);

  // Issue play command while buffering
  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kBuffering);

  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);
}

// Expect a valid seek to continue producing audio after buffering
TEST_F(FilePlaybackProcessorTest, seek_while_playing) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPlaying);

  setSeek(0.8f);
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPlaying);

  proc->processBlock(buff, mbuff);
  EXPECT_FALSE(buff.hasBeenCleared());
}

// Valid seek while stopped
TEST_F(FilePlaybackProcessorTest, seek_while_stopped) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setSeek(0.8f);
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());
}

// Expect chained seek commands to finish at the correct seek position and in
// a paused state
TEST_F(FilePlaybackProcessorTest, chained_seeks) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setSeek(0.8f);
  setSeek(0.1f);
  setSeek(0.5f);

  waitForBuffering();

  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());
  float filePos = -1.0f;
  fpbData.currFilePosition.read(filePos);
  EXPECT_NEAR(filePos, 0.5f, 0.01f);
}

// Expect changing the file while seeking to finish at the start of the new
// file in a stopped state
TEST_F(FilePlaybackProcessorTest, change_file_while_seeking) {
  const std::filesystem::path kReferenceFilePath1 =
      std::filesystem::current_path() / "test_fpb_processor_1.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath1);
  const std::filesystem::path kReferenceFilePath2 =
      std::filesystem::current_path() / "test_fpb_processor_2.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath2);
  setFile(juce::String(kReferenceFilePath1.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setSeek(0.99f);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kBuffering);

  // Change the file while seeking
  setFile(juce::String(kReferenceFilePath2.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);
  float currFilePos = -1.0f;
  fpbData.currFilePosition.read(currFilePos);
  EXPECT_FLOAT_EQ(currFilePos, 0.0f);

  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());
}

// Expect starting an export during playback to stop playback and release
// file
TEST_F(FilePlaybackProcessorTest, start_export_while_playing) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPlaying);

  // Start export
  setFile("");
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  // Validate processor data
  float currFilePos = -1.0f;
  juce::uint64 fileDuration = -1;
  fpbData.currFilePosition.read(currFilePos);
  EXPECT_FLOAT_EQ(currFilePos, 0.0f);
  fpbData.fileDuration_s.read(fileDuration);
  EXPECT_EQ(fileDuration, 0);
}

// Expect starting an export while buffering to release file and end buffering
TEST_F(FilePlaybackProcessorTest, start_export_while_buffering) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kBuffering);

  // Start export
  setFile("");
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  // Validate processor data
  float currFilePos = -1.0f;
  juce::uint64 fileDuration = -1;
  fpbData.currFilePosition.read(currFilePos);
  EXPECT_FLOAT_EQ(currFilePos, 0.0f);
  fpbData.fileDuration_s.read(fileDuration);
  EXPECT_EQ(fileDuration, 0);
}

// Expect destroying the processor while playing to be safe
TEST_F(FilePlaybackProcessorTest, destroy_while_playing) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPlaying);

  // Destroy processor
  proc.reset();
}

// Expect destroying the processor while buffering to be safe
TEST_F(FilePlaybackProcessorTest, destroy_while_buffering) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kBuffering);

  // Destroy processor
  proc.reset();
}

// Expect changing layout on a fresh file to buffer and return to paused
TEST_F(FilePlaybackProcessorTest, change_layout_on_fresh_file) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  // Change layout to 5.1
  setLayout(Speakers::k5Point1);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kBuffering);

  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());
}

// Validates that OBU traversal indexing produces the correct frame count for a
// file with multiple audio elements and multiple mix presentations.
// A 5.1 audio element has multiple substreams per temporal unit; the traversal
// must count temporal units (not individual substream OBUs) to be correct.
TEST_F(FilePlaybackProcessorTest, index_complex_iamf_file) {
  const std::filesystem::path kFilePath =
      std::filesystem::current_path() / "test_fpb_complex.iamf";
  createIAMFFile30Sec2AE2MP(kFilePath);

  // Expected: 30s * 16kHz / 128 samples-per-frame = 3750 frames
  const juce::uint64 kExpectedDurationSec = 30;
  const size_t kExpectedFrames = static_cast<size_t>(kExpectedDurationSec) *
                                 FileOutputTests::kSampleRate /
                                 FileOutputTests::kSamplesPerFrame;

  setFile(juce::String(kFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  ASSERT_EQ(procState, FilePlayback::ProcessorState::kPaused)
      << "File failed to load — check OBU traversal handles multiple "
         "substreams";

  juce::uint64 fileDuration = 0;
  fpbData.fileDuration_s.read(fileDuration);
  EXPECT_EQ(fileDuration, kExpectedDurationSec)
      << "Frame count from OBU traversal is wrong: expected " << kExpectedFrames
      << " frames (" << kExpectedDurationSec << "s)";
}

// Expect changing layout while playing to buffer and return to paused
TEST_F(FilePlaybackProcessorTest, change_layout_while_playing) {
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  setCommand(FilePlayback::PlaybackCommand::kPlay);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPlaying);

  proc->processBlock(buff, mbuff);
  EXPECT_FALSE(buff.hasBeenCleared());

  // Change layout to 7.1
  setLayout(Speakers::k7Point1);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kBuffering);

  buff.clear();
  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());

  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());
  float currFilePos = -1.0f;
  fpbData.currFilePosition.read(currFilePos);
  EXPECT_FLOAT_EQ(currFilePos, 0.0f);
}

// Expect seeking after changing layout to work as expected
TEST_F(FilePlaybackProcessorTest, seek_after_changing_layout) {
  // Load the file and wait for buffering
  const std::filesystem::path kReferenceFilePath =
      std::filesystem::current_path() / "test_fpb_processor.iamf";
  createIAMFFile30SecStereo(kReferenceFilePath);
  setFile(juce::String(kReferenceFilePath.string()));
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  // Change the layout
  setLayout(Speakers::k5Point1);

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kBuffering);

  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  // Do some seeks and validate
  setSeek(0.5f);
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);

  float currFilePos = -1.0f;
  fpbData.currFilePosition.read(currFilePos);
  EXPECT_NEAR(currFilePos, 0.5f, 0.01f);

  proc->processBlock(buff, mbuff);
  EXPECT_TRUE(buff.hasBeenCleared());

  // Seek behind
  setSeek(0.1f);
  waitForBuffering();

  fpbData.processorState.read(procState);
  EXPECT_EQ(procState, FilePlayback::ProcessorState::kPaused);
  fpbData.currFilePosition.read(currFilePos);
  EXPECT_NEAR(currFilePos, 0.1f, 0.01f);
}
