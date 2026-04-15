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

#include "FilePlaybackProcessor.h"

#include <filesystem>
#include <memory>

#include "FilePlaybackTasks.h"
#include "IAMFBufferedReader.h"
#include "data_structures/src/FilePlayback.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "logger/logger.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

FilePlaybackProcessor::FilePlaybackProcessor(FilePlaybackRepository& fpb,
                                             FilePlaybackProcessorData& fpbd)
    : fpbr_(fpb), fpbData_(fpbd) {
  // Initialize visible data
  fpbData_.processorState.update(FilePlayback::ProcessorState::kPaused);
  fpbData_.currFilePosition.update(0.0f);
  fpbData_.fileDuration_s.update(0);
  fpbr_.registerListener(this);
}

FilePlaybackProcessor::~FilePlaybackProcessor() {
  fpbr_.deregisterListener(this);
  // Reset playback commands
  FilePlayback fpb = fpbr_.get();
  fpb.setReqdDecodeLayout(Speakers::kStereo);
  fpb.setSeekPosition(0.0f);
  fpb.setVolume(1.0f);
  fpb.setPlaybackCommand(FilePlayback::PlaybackCommand::kPause);
  fpbr_.update(fpb);
}

void FilePlaybackProcessor::reinitializeAfterStateRestore() {
  const FilePlayback kFpb = fpbr_.get();
  const std::filesystem::path kFile(kFpb.getPlaybackFile().toStdString());
  const Speakers::AudioElementSpeakerLayout kLayout(kFpb.getReqdDecodeLayout());
  submitLoadTask(kFile, kLayout);
}

void FilePlaybackProcessor::prepareToPlay(double sampleRate,
                                          int samplesPerBlock) {
  currCtx_.daw = {.sampleRate = sampleRate, .bufferSize = samplesPerBlock};
}

void FilePlaybackProcessor::volumeTo(const float vol) {
  jassert(vol >= 0.0f && vol <= 2.0f);
  gain_ = vol;
}

std::unique_ptr<IamfBufferedReader> FilePlaybackProcessor::swapBuffer(
    std::unique_ptr<IamfBufferedReader> newBuffer) {
  juce::SpinLock::ScopedLockType lock(bufferLock_);
  std::unique_ptr<IamfBufferedReader> oldBuffer = std::move(buffer_);
  buffer_ = std::move(newBuffer);
  return oldBuffer;
}

void FilePlaybackProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& mbuffer) {
  if (state_ == FilePlayback::ProcessorState::kPlaying &&
      bufferLock_.tryEnter()) {
    if (buffer_ && buffer_->isReady()) {
      const int kSamplesSourced =
          resampler_.read(*buffer_, tempBuffer_, buffer.getNumSamples());
      currCtx_.srcSamplesConsumed += kSamplesSourced;
      fpbData_.currFilePosition.update(currCtx_.getPlaybackPosition());

      tempBuffer_.applyGain(gain_);

      const int kChToCopy =
          std::min(buffer.getNumChannels(), tempBuffer_.getNumChannels());
      const int kSamplesToCopy =
          std::min(buffer.getNumSamples(), tempBuffer_.getNumSamples());
      for (int ch = 0; ch < kChToCopy; ++ch) {
        buffer.addFrom(ch, 0, tempBuffer_, ch, 0, kSamplesToCopy);
      }
      tempBuffer_.clear();
    }
    bufferLock_.exit();
  }
}

void FilePlaybackProcessor::valueTreePropertyChanged(
    juce::ValueTree& tree, const juce::Identifier& property) {
  LOG_INFO(0, "FilePlaybackProcessor: ValueTree property changed: " +
                  property.toString().toStdString());

  FilePlayback kFpb = fpbr_.get();
  if (property == FilePlayback::kPlaybackFile) {
    const std::filesystem::path kFile(kFpb.getPlaybackFile().toStdString());
    const Speakers::AudioElementSpeakerLayout kLayout(
        kFpb.getReqdDecodeLayout());
    submitLoadTask(kFile, kLayout);
    return;
  }

  if (property == FilePlayback::kReqdDecodeLayout) {
    const std::filesystem::path kFile(kFpb.getPlaybackFile().toStdString());
    const size_t kNumFrames = currCtx_.src.numFrames;
    const Speakers::AudioElementSpeakerLayout kLayout(
        kFpb.getReqdDecodeLayout());
    submitLayoutTask(kFile, kNumFrames, kLayout);
    return;
  }

  if (property == FilePlayback::kSeekPosition) {
    const std::filesystem::path kFile(kFpb.getPlaybackFile().toStdString());
    const size_t kNumFrames = currCtx_.src.numFrames;
    const Speakers::AudioElementSpeakerLayout kLayout(
        kFpb.getReqdDecodeLayout());
    const float kPos = kFpb.getSeekPosition();
    const FilePlayback::ProcessorState kPrevState = state_;
    submitSeekTask(kFile, kNumFrames, kLayout, kPos, kPrevState);
    return;
  }

  if (property == FilePlayback::kVolume) {
    volumeTo(kFpb.getVolume());
    return;
  }

  if (property == FilePlayback::kPlaybackCommand) {
    const auto kCmd = kFpb.getPlaybackCommand();
    bool existingBuffer;
    {
      juce::SpinLock::ScopedLockType lock(bufferLock_);
      existingBuffer = buffer_ != nullptr;
    }
    switch (kCmd) {
      case FilePlayback::PlaybackCommand::kPlay:
        if (existingBuffer) {
          updateState(FilePlayback::ProcessorState::kPlaying);
        }
        break;
      case FilePlayback::PlaybackCommand::kPause:
        if (existingBuffer) {
          updateState(FilePlayback::ProcessorState::kPaused);
        }
        break;
      case FilePlayback::PlaybackCommand::kStop:
        const std::filesystem::path kFile(kFpb.getPlaybackFile().toStdString());
        const size_t kNumFrames = currCtx_.src.numFrames;
        const Speakers::AudioElementSpeakerLayout kLayout(
            kFpb.getReqdDecodeLayout());
        const float kPos = kFpb.getSeekPosition();
        submitSeekTask(kFile, kNumFrames, kLayout, 0.0f,
                       FilePlayback::ProcessorState::kPaused);
        break;
    }
    return;
  }
}

void FilePlaybackProcessor::postTask(Result&& taskRes) {
  if (taskRes.status == PlaybackTasks::kPreempted) {
    return;
  }

  if (taskRes.status == PlaybackTasks::kLoadFailed) {
    transitionState(taskRes.status);
    return;
  }

  if (taskRes.buffer) {
    juce::SpinLock::ScopedLockType lock(bufferLock_);
    if (taskRes.metadata.valid) {
      currCtx_.src = taskRes.metadata;
    }
    const size_t kSrcFrameSz = currCtx_.src.frameSize;
    const size_t kSeekedSamples = taskRes.seekedFrameIdx * kSrcFrameSz;
    currCtx_.srcSamplesConsumed = kSeekedSamples;

    resampler_.prepare(currCtx_.src.sampleRate, currCtx_.daw.sampleRate,
                       currCtx_.src.numChannels);
    tempBuffer_.setSize(currCtx_.src.numChannels, currCtx_.daw.bufferSize);

    fpbData_.currFilePosition.update(currCtx_.getPlaybackPosition());
    fpbData_.fileDuration_s.update(currCtx_.src.numFrames *
                                   currCtx_.src.frameSize /
                                   currCtx_.src.sampleRate);

    buffer_ = std::move(taskRes.buffer);
  }
  transitionState(taskRes.status);
}

void FilePlaybackProcessor::transitionState(const ResultType res) {
  const FilePlayback::ProcessorState kNextState = getNextState(res);
  state_ = kNextState;
  fpbData_.processorState.update(state_);
}

FilePlayback::ProcessorState FilePlaybackProcessor::getNextState(
    const ResultType res) const {
  switch (res) {
    case ResultType::kLoadFailed:
      return FilePlayback::ProcessorState::kError;
    case ResultType::kLoadFinished:
    case ResultType::kLayoutFinished:
    case ResultType::kSeekPausedFinished:
      return FilePlayback::ProcessorState::kPaused;
    case ResultType::kSeekPlayingFinished:
      return FilePlayback::ProcessorState::kPlaying;
    default:
      return FilePlayback::ProcessorState::kPaused;
  }
}

void FilePlaybackProcessor::updateState(
    const FilePlayback::ProcessorState state) {
  state_ = state;
  fpbData_.processorState.update(state_);
}

void FilePlaybackProcessor::submitLoadTask(
    const std::filesystem::path& path,
    const Speakers::AudioElementSpeakerLayout layout) {
  if (!PlaybackTasks::isIamfFile(path)) {
    {
      juce::SpinLock::ScopedLockType lock(bufferLock_);
      buffer_ = nullptr;
    }
    currCtx_.src.numFrames = 0;
    fpbData_.fileDuration_s.update(0);
    fpbData_.currFilePosition.update(0);
    updateState(FilePlayback::ProcessorState::kPaused);
    return;
  }

  currCtx_.src.numFrames = 0;
  worker_.submit(
      [path, layout](Worker::CancelFlag& stop) {
        return PlaybackTasks::loadTask(path, layout, stop);
      },
      [this](Result&& result) { postTask(std::move(result)); });
  updateState(FilePlayback::ProcessorState::kBuffering);
}

void FilePlaybackProcessor::submitLayoutTask(
    const std::filesystem::path& path, const size_t numFrames,
    const Speakers::AudioElementSpeakerLayout layout) {
  if (!PlaybackTasks::isIamfFile(path)) {
    updateState(FilePlayback::ProcessorState::kPaused);
    return;
  }

  worker_.submit(
      [path, layout, numFrames](Worker::CancelFlag& stop) {
        return PlaybackTasks::layoutTask(path, layout, numFrames, stop);
      },
      [this](Result&& result) { postTask(std::move(result)); });
  updateState(FilePlayback::ProcessorState::kBuffering);
}

void FilePlaybackProcessor::submitSeekTask(
    const std::filesystem::path& path, const size_t numFrames,
    const Speakers::AudioElementSpeakerLayout layout, const float position,
    const FilePlayback::ProcessorState prevState) {
  IamfBufferedReader* buffer = swapBuffer(nullptr).release();
  if (buffer) {
    worker_.submit(
        [position, prevState, numFrames, buffer](Worker::CancelFlag& stop) {
          return PlaybackTasks::seekTask(position, prevState, numFrames, buffer,
                                         stop);
        },
        [this](Result&& result) { postTask(std::move(result)); });
  } else {
    worker_.submit(
        [path, layout, position, numFrames,
         prevState](Worker::CancelFlag& stop) {
          return PlaybackTasks::seekTask(path, layout, position, prevState,
                                         stop);
        },
        [this](Result&& result) { postTask(std::move(result)); });
  }

  updateState(FilePlayback::ProcessorState::kBuffering);
}