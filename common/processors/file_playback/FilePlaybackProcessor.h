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

#include <atomic>

#include "FilePlaybackTasks.h"
#include "FilePlaybackWorker.h"
#include "IAMFBufferedReader.h"
#include "data_repository/implementation/FilePlaybackRepository.h"
#include "data_structures/src/FilePlayback.h"
#include "data_structures/src/FilePlaybackProcessorData.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"
#include "processors/file_playback/FilePlaybackResampler.h"
#include "processors/processor_base/ProcessorBase.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class FilePlaybackProcessor : public ProcessorBase,
                              public juce::ValueTree::Listener {
 public:
  using ResultType = PlaybackTasks::ResultType;
  using Result = PlaybackTasks::Result;
  FilePlaybackProcessor(FilePlaybackRepository& fpb,
                        FilePlaybackProcessorData& fpbd);
  ~FilePlaybackProcessor();

  void reinitializeAfterStateRestore();
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void volumeTo(const float vol);
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  void setNonRealtime(bool isNonRealtime) noexcept override {}
  void valueTreePropertyChanged(juce::ValueTree& tree,
                                const juce::Identifier& property) override;

 private:
  struct DawContext {
    double sampleRate;
    int bufferSize;
  };
  struct FileContext {
    IAMFFileReader::StreamData src;
    DawContext daw;
    size_t srcSamplesConsumed;

    float getPlaybackPosition() const {
      const size_t kNumSrcSamples = src.numFrames * src.frameSize;
      return kNumSrcSamples > 0 ? (float)srcSamplesConsumed / kNumSrcSamples
                                : 0.0f;
    }
  };

  std::unique_ptr<IamfBufferedReader> swapBuffer(
      std::unique_ptr<IamfBufferedReader> newBuffer);
  void submitLoadTask(const std::filesystem::path& path,
                      const Speakers::AudioElementSpeakerLayout layout);
  void submitLayoutTask(const std::filesystem::path& path,
                        const size_t numFrames,
                        const Speakers::AudioElementSpeakerLayout layout);
  void submitSeekTask(const std::filesystem::path& path, const size_t numFrames,
                      const Speakers::AudioElementSpeakerLayout layout,
                      const float position,
                      const FilePlayback::ProcessorState prevState);
  void postTask(Result&& taskRes);
  void transitionState(const ResultType res);
  void updateState(const FilePlayback::ProcessorState state);
  FilePlayback::ProcessorState getNextState(const ResultType res) const;

  // Data
  std::unique_ptr<IamfBufferedReader> buffer_;
  juce::AudioBuffer<float> tempBuffer_;
  std::atomic<float> gain_ = 1;
  FileContext currCtx_;
  FilePlaybackResampler resampler_;
  Worker worker_;
  // State
  FilePlaybackProcessorData& fpbData_;
  FilePlaybackRepository& fpbr_;
  // Synchronization
  juce::SpinLock bufferLock_;
  std::atomic<FilePlayback::ProcessorState> state_ =
      FilePlayback::ProcessorState::kPaused;
};