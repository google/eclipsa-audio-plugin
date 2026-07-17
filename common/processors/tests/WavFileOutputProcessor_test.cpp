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
#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <functional>
#include <vector>

#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/RoomSetupRepository.h"
#include "data_structures/src/FileExport.h"
#include "processors/file_output/WavFileOutputProcessor.h"

namespace {
class TestFileExportRepository : public FileExportRepository {
 public:
  TestFileExportRepository() : FileExportRepository(juce::ValueTree{"test"}) {}
};

class TestRoomSetupRepository : public RoomSetupRepository {
 public:
  TestRoomSetupRepository() : RoomSetupRepository(juce::ValueTree{"test"}) {}
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

 protected:
  void postToMessageThread(std::function<void()> task) override {
    pendingTasks_.push_back(std::move(task));
  }

 private:
  std::vector<std::function<void()>> pendingTasks_;
};

class WavFileOutputTests : public ::testing::Test {
 protected:
  TestFileExportRepository fileExportRepository;
  TestRoomSetupRepository roomSetupRepository;
  TestableWavFileOutputProcessor processor{fileExportRepository,
                                           roomSetupRepository};
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
TEST_F(WavFileOutputTests, open_failure_via_manual_export_toggle_does_not_deadlock) {
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
