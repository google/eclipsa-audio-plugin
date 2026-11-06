/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <data_repository/data_repository.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <functional>
#include <memory>

#include "../processor_base/ProcessorBase.h"
#include "../render/BinauralMixSource.h"
#include "AudioElementFileWriter.h"
#include "FileWriter.h"
#include "data_repository/implementation/RoomSetupRepository.h"
#include "data_structures/src/RoomSetup.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"
#include "user_metadata.pb.h"

//==============================================================================
// Not `final`: a test-only subclass overrides postToMessageThread() to run
// deferred repository updates synchronously (there's no running JUCE
// message loop in the unit test binary). See postToMessageThread()'s
// comment.
class WavFileOutputProcessor : public ProcessorBase,
                               public juce::ValueTree::Listener {
 public:
  static constexpr int kMaxRoomWavChannels = 12;

  // The WAV channel set an export writes, and for each of its channels, in
  // getChannelTypes() order, the source channel it is copied from.
  struct WavChannelMap {
    juce::AudioChannelSet channelSet;
    std::array<int, kMaxRoomWavChannels> sourceChannels{};
  };

  // Rear surrounds and top-side heights have no WAV channel mask bit, so they
  // are written as the WAVE back surrounds and top-front heights.
  static WavChannelMap getRoomWavChannelMap(
      Speakers::AudioElementSpeakerLayout layout);

  //==============================================================================
  WavFileOutputProcessor(FileExportRepository& fileExportRepository,
                         RoomSetupRepository& roomSetupRepository,
                         const BinauralMixSource& binauralMixSource);
  ~WavFileOutputProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void setNonRealtime(bool isNonRealtime) noexcept override;

  //==============================================================================
  void checkManualExportStartStop(const juce::Identifier& property);
  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;

  //==============================================================================
  const juce::String getName() { return "WaveFileOutput"; }

 protected:
  // Posts `task` to run on the message thread. Defaults to
  // juce::MessageManager::callAsync; overridden by tests (which have no
  // running JUCE message loop -- runDispatchLoopUntil isn't even compiled
  // in, since JUCE_MODAL_LOOPS_PERMITTED is 0 for this plugin build) to
  // invoke `task` immediately instead. See deferRepositoryUpdate() for why
  // production code needs this deferred at all.
  virtual void postToMessageThread(std::function<void()> task) {
    juce::MessageManager::callAsync(std::move(task));
  }

  virtual FileWriter* createFileWriter(const juce::String& filename,
                                       double sampleRate,
                                       const juce::AudioChannelSet& channelSet,
                                       int firstChannel, int bitDepth,
                                       AudioCodec codec) {
    return new FileWriter(filename, sampleRate, channelSet, firstChannel,
                          bitDepth, codec);
  }

 private:
  // Records a kFileWriteFailed ExportError (unless a more specific error is
  // already on record) when a FileWriter::write() call fails.
  void recordWriteFailureIfAny(bool writeSucceeded);

  // Advances the export's sample tally past a block of `numSamples` and
  // returns true when the block starts inside the export's start and end
  // sample indices.
  bool isBlockInExportRange(int numSamples);

  // Copies the export's source channels (the room render in `buffer`, or the
  // binaural mix) into exportBuffer_ in WAV order and writes them. Returns
  // false if the write fails.
  bool writeExportBlock(const juce::AudioBuffer<float>& buffer);

  // Defers a FileExport mutation to the next message-loop turn (via
  // postToMessageThread), running `mutator` against a config fetched fresh
  // at execution time (not capture time), then writing it back. This
  // processor is registered as a juce::ValueTree::Listener on
  // fileExportRepository_ (see the constructor), and production callers
  // (e.g. the export button handler) update that same repository with a
  // read-modify-write of a FULL FileExport snapshot
  // (`config = repo.get(); config.setX(...); repo.update(config)`).
  // FileExportRepository::update() fires ValueTree listener callbacks
  // SYNCHRONOUSLY, mid-copyPropertiesFrom, before that caller's own,
  // now-stale snapshot has finished being applied property-by-property. If
  // this method wrote to the repository synchronously from inside that
  // callback chain (checkManualExportStartStop -> setNonRealtime), two
  // things break: (1) setNonRealtime()'s lock_ is non-reentrant, so a
  // synchronous update() that re-enters setNonRealtime() via the listener
  // deadlocks; (2) even without the lock, the caller's own copyPropertiesFrom
  // loop would go on to reapply ITS stale (pre-failure) exportError value
  // for a property processed after the one that triggered this callback
  // (FileExport.h declares manualExport before exportError, so this is not
  // a rare race -- it reproduces every time), silently clobbering whatever
  // this method just wrote. Deferring (the same pattern ExportErrorBanner
  // already uses for this repository, see its valueTreePropertyChanged)
  // guarantees this runs after the entire triggering call stack -- including
  // the caller's copyPropertiesFrom loop -- has fully unwound, so neither
  // hazard applies.
  void deferRepositoryUpdate(std::function<void(FileExport&)> mutator);

  bool performingRender_;  // True if we are rendering in offline mode
  FileExportRepository& fileExportRepository_;
  RoomSetupRepository& roomSetupRepository_;
  const BinauralMixSource& binauralMixSource_;
  FileWriter* fileWriter_;
  bool exportingBinaural_ = false;
  WavChannelMap exportChannelMap_;
  juce::AudioBuffer<float> exportBuffer_;
  int numSamples_;
  juce::int64 sampleTally_ = 0;  // Samples rendered since the export started
  juce::int64 startSampleIdx_;
  juce::int64 endSampleIdx_;
  juce::SpinLock lock_;
  std::atomic<bool> hasRecordedWriteFailure_ = false;
  std::shared_ptr<std::atomic<bool>> isAlive_ =
      std::make_shared<std::atomic<bool>>(
          true);  // Safety valve for deferred updates
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavFileOutputProcessor)
};
