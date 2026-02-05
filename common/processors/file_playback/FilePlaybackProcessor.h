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
#include <filesystem>
#include <memory>

#include "BackgroundBuffer.h"
#include "FilePlaybackProcessorTasks.h"
#include "FilePlaybackProcessorWorker.h"
#include "FilePlaybackResampler.h"
#include "data_repository/implementation/FilePlaybackRepository.h"
#include "data_structures/src/FilePlaybackProcessorData.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"
#include "processors/processor_base/ProcessorBase.h"

class FilePlaybackProcessorWorker;

struct PlaybackFileContext {
  std::filesystem::path filePath;
  unsigned sampleRate = 0, frameSize = 0, numChannels = 0;
  size_t numFrames = 0, samplesConsumed = 0;

  float getPlaybackPosition() const {
    const size_t totalSourceSamples = numFrames * frameSize;
    return totalSourceSamples > 0 ? (float)samplesConsumed / totalSourceSamples
                                  : 0.0f;
  }
};

class FilePlaybackProcessor : public ProcessorBase, juce::ValueTree::Listener {
 public:
  using State = FilePlayback::ProcessorState;
  using TaskType = FilePlaybackProcessorEvents::TaskType;
  using TaskResult = FilePlaybackProcessorEvents::TaskResult;
  using TaskData = FilePlaybackProcessorEvents::TaskData;

  FilePlaybackProcessor(FilePlaybackRepository& fpbr,
                        FilePlaybackProcessorData& fpbrd);
  ~FilePlaybackProcessor();

  void reinitializeAfterStateRestore();
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  void setNonRealtime(bool isNonRealtime) noexcept override {}
  void valueTreePropertyChanged(juce::ValueTree& tree,
                                const juce::Identifier& property) override;
  void updateProcessorState(const State newState) {
    state_ = newState;
    fpbData_.processorState.update(newState);
  }
  void abortTask(const bool abort) { abortTask_ = abort; }

  // Tasks
  TaskResult loadFileForPlayback(const std::string& iamfFile);
  TaskResult seekTo(const float pos, const bool wasPlaying = false);
  TaskResult changeLayout(const Speakers::AudioElementSpeakerLayout layout,
                          const bool wasPlaying = false);
  // New Tasks
  void loadFile(const std::filesystem::path& file, std::atomic_bool& cancel) {
    const std::filesystem::path kFilePath(file);
    if (kFilePath.empty() || kFilePath.extension() != ".iamf" ||
        !std::filesystem::exists(kFilePath)) {
      // return TaskResult::kLoadFinished;
    }
    auto reader = IAMFFileReader::createIamfReader(
        file, IAMFFileReader::kDefaultReaderSettings, cancel);
    if (abort) {
      // Aborted
    }
    if (!reader) {
      // Load failed
    }
    auto buffer = new BackgroundBuffer(5, *reader);
    if (!buffer) {
      // Load failed
    }
    // Populate file context which could include data and the buffer and reader?
  }
  void seek(const float pos, BackgroundBuffer& buffer,
            std::atomic_bool& cancel) {
    // TODO: Maybe this takes an existing background buffer object
    buffer.seek(pos, cancel);
  }
  void layout(const Speakers::AudioElementSpeakerLayout layout) {}

  void resetProcessor();
  void handleTaskCompletion(const TaskResult wayFinished);

 private:
  void volumeTo(const float vol);

  double sampleRate_ = 0.0f;
  int samplesPerBlock_ = 0;
  std::atomic<float> gain_ = 1.0f;
  PlaybackFileContext currFile_;
  std::unique_ptr<IAMFFileReader> reader_;
  std::unique_ptr<BackgroundBuffer> decodedBuffer_;
  FilePlaybackResampler resampler_;
  juce::AudioBuffer<float> tempBuffer_;
  juce::SpinLock bbl_;
  std::atomic_bool abortTask_ = false;
  std::unique_ptr<FilePlaybackProcessorWorker> worker_;
  // State
  FilePlaybackRepository& fpbr_;
  FilePlaybackProcessorData& fpbData_;
  std::atomic<State> state_{State::kPaused};
};
