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

#include "data_structures/src/FilePlayback.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "logger/logger.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

FilePlaybackProcessor::FilePlaybackProcessor(FilePlaybackRepository& fpbr,
                                             FilePlaybackProcessorData& fpbrd)
    : fpbr_(fpbr),
      fpbData_(fpbrd),
      worker_(std::make_unique<FilePlaybackProcessorWorker>(*this)) {
  // Initialize visible data
  fpbData_.processorState.update(State::kPaused);
  fpbData_.currFilePosition.update(0.0f);
  fpbData_.fileDuration_s.update(0);
}

FilePlaybackProcessor::~FilePlaybackProcessor() {
  abortTask_ = true;
  fpbr_.deregisterListener(this);
  fpbData_.processorState.update(State::kPaused);
  fpbData_.currFilePosition.update(0.0f);
  fpbData_.fileDuration_s.update(0);
}

void FilePlaybackProcessor::reinitializeAfterStateRestore() {
  fpbr_.registerListener(this);

  const juce::String kPlaybackFile = fpbr_.get().getPlaybackFile();
  if (kPlaybackFile.isNotEmpty()) {
    worker_->submitTask(FilePlaybackProcessorEvents::TaskType::kLoad,
                        {.filePath = kPlaybackFile.toStdString()});
  }
}

void FilePlaybackProcessor::prepareToPlay(double sampleRate,
                                          int samplesPerBlock) {
  sampleRate_ = sampleRate;
  samplesPerBlock_ = samplesPerBlock;
}

void FilePlaybackProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer&) {
  if (state_ == State::kPlaying && bbl_.tryEnter()) {
    if (decodedBuffer_ && decodedBuffer_->isReady()) {
      const int kSamplesSourced =
          resampler_.read(*decodedBuffer_, tempBuffer_, buffer.getNumSamples());
      // Update current position in file
      currFile_.samplesConsumed += static_cast<size_t>(kSamplesSourced);
      fpbData_.currFilePosition.update(currFile_.getPlaybackPosition());

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
    bbl_.exit();
  }
}

void FilePlaybackProcessor::volumeTo(const float vol) {
  jassert(vol >= 0.0f && vol <= 2.0f);
  gain_ = vol;
}

FilePlaybackProcessor::TaskResult FilePlaybackProcessor::loadFileForPlayback(
    const std::string& iamfFile) {
  juce::SpinLock::ScopedLockType lock(bbl_);

  // Check validity
  const std::filesystem::path kFilePath(iamfFile);
  if (kFilePath.empty() || kFilePath.extension() != ".iamf" ||
      !std::filesystem::exists(kFilePath)) {
    resetProcessor();
    return TaskResult::kLoadFinished;
  }

  decodedBuffer_.reset();

  // Construct the reader and buffer
  reader_ = IAMFFileReader::createIamfReader(
      iamfFile, IAMFFileReader::kDefaultReaderSettings, abortTask_);
  if (!reader_) {
    TaskResult completionType;
    if (abortTask_) {
      completionType = TaskResult::kLoadFinished;
      LOG_INFO(0, "FilePlaybackProcessor: File load pre-empted");
    } else {
      completionType = TaskResult::kLoadFailed;
      LOG_INFO(0, "FilePlaybackProcessor: Invalid or corrupted IAMF file");
    }
    return completionType;
  }

  decodedBuffer_ = std::make_unique<BackgroundBuffer>(5, *reader_);
  if (!decodedBuffer_) {
    LOG_ERROR(0, "FilePlaybackProcessor: Failed to create background buffer");
    return TaskResult::kLoadFailed;
  }

  // Populate current file context
  const IAMFFileReader::StreamData kData = reader_->getStreamData();
  currFile_ = {.filePath = iamfFile,
               .sampleRate = kData.sampleRate,
               .frameSize = kData.frameSize,
               .numChannels = static_cast<unsigned>(kData.numChannels),
               .numFrames = kData.numFrames,
               .samplesConsumed = 0};

  const juce::int64 kDuration = static_cast<juce::int64>(
      kData.numFrames * kData.frameSize / kData.sampleRate);
  fpbData_.fileDuration_s.update(kDuration);
  fpbData_.currFilePosition.update(0.0f);

  // Prepare resampler and temp buffer for playback
  resampler_.prepare(currFile_.sampleRate, sampleRate_, currFile_.numChannels);
  tempBuffer_ =
      juce::AudioBuffer<float>(currFile_.numChannels, samplesPerBlock_);
  return TaskResult::kLoadFinished;
}

FilePlaybackProcessor::TaskResult FilePlaybackProcessor::seekTo(
    const float pos, const bool wasPlaying) {
  juce::SpinLock::ScopedLockType lock(bbl_);
  float actualPos = 0.0f;
  if (reader_ && decodedBuffer_) {
    const auto kStreamData = reader_->getStreamData();
    const size_t kTargetFrame =
        static_cast<size_t>(pos * kStreamData.numFrames);
    decodedBuffer_->seek(kTargetFrame, abortTask_);
    if (abortTask_) {
      LOG_INFO(0, "FilePlaybackProcessor: Seek operation pre-empted");
      return wasPlaying ? TaskResult::kSeekPlayingFinished
                        : TaskResult::kSeekPausedFinished;
    }

    // Update with the actual position
    currFile_.samplesConsumed = kTargetFrame * kStreamData.frameSize;
    actualPos = currFile_.getPlaybackPosition();
  }

  // Wait for the buffer to be ready after seeking
  if (decodedBuffer_) {
    LOG_INFO(0,
             "FilePlaybackProcessor: Waiting for buffer to be ready after "
             "seek");
    decodedBuffer_->waitUntilReady();
    LOG_INFO(0, "FilePlaybackProcessor: Buffer ready after seek.");
  }

  // Restore the previous playback state after seeking
  fpbData_.currFilePosition.update(actualPos);
  return wasPlaying ? TaskResult::kSeekPlayingFinished
                    : TaskResult::kSeekPausedFinished;
}

FilePlaybackProcessor::TaskResult FilePlaybackProcessor::changeLayout(
    const Speakers::AudioElementSpeakerLayout layout, const bool wasPlaying) {
  juce::SpinLock::ScopedLockType lock(bbl_);
  decodedBuffer_.reset();

  if (!reader_) {
    return FilePlaybackProcessorEvents::TaskResult::kLoadFailed;
  }

  reader_->resetLayout(layout);
  decodedBuffer_ = std::make_unique<BackgroundBuffer>(5, *reader_);
  if (!decodedBuffer_) {
    LOG_ERROR(0,
              "FilePlaybackProcessor: Failed to create background buffer after "
              "layout change");
    return FilePlaybackProcessorEvents::TaskResult::kLoadFailed;
  }

  // Reset current file context
  currFile_.samplesConsumed = 0;
  currFile_.numChannels = layout.getNumChannels();

  // Prepare resampler and temp buffer for playback
  resampler_.prepare(currFile_.sampleRate, sampleRate_, currFile_.numChannels);
  tempBuffer_ =
      juce::AudioBuffer<float>(currFile_.numChannels, samplesPerBlock_);
  return wasPlaying
             ? FilePlaybackProcessorEvents::TaskResult::kLayoutPlayingFinished
             : FilePlaybackProcessorEvents::TaskResult::kLayoutPausedFinished;
}

void FilePlaybackProcessor::resetProcessor() {
  decodedBuffer_.reset();
  reader_.reset();

  // Reset the file context
  currFile_ = {};

  // Reset processor data to initial state
  fpbData_.currFilePosition.update(0.0f);
  fpbData_.fileDuration_s.update(0);
}

void FilePlaybackProcessor::handleTaskCompletion(const TaskResult wayFinished) {
  LOG_INFO(0, "FilePlaybackProcessor::handleTaskCompletion: Task completed: " +
                  std::string(FilePlaybackProcessorEvents::taskResultToString(
                      wayFinished)));

  switch (wayFinished) {
    case TaskResult::kLoadFailed:
      updateProcessorState(State::kError);
      break;
    case TaskResult::kLoadFinished:
    case TaskResult::kLayoutPausedFinished:
    case TaskResult::kSeekPausedFinished:
      updateProcessorState(State::kPaused);
      break;
    case TaskResult::kLayoutPlayingFinished:
    case TaskResult::kSeekPlayingFinished:
      updateProcessorState(State::kPlaying);
      break;
    default:
      break;
  }
}

void FilePlaybackProcessor::valueTreePropertyChanged(
    juce::ValueTree& tree, const juce::Identifier& property) {
  LOG_INFO(0, "FilePlaybackProcessor: ValueTree property changed: " +
                  property.toString().toStdString());

  FilePlayback fpb = fpbr_.get();
  if (property == FilePlayback::kPlaybackFile) {
    const juce::String kFile = fpb.getPlaybackFile();
    worker_->submitTask(FilePlaybackProcessorEvents::TaskType::kLoad,
                        {.filePath = kFile.toStdString()});
  } else if (property == FilePlayback::kReqdDecodeLayout) {
    worker_->submitTask(
        FilePlaybackProcessorEvents::TaskType::kLayout,
        {.prevState = state_, .layout = fpb.getReqdDecodeLayout()});
  } else if (property == FilePlayback::kSeekPosition) {
    const State kCurrState = state_;
    worker_->submitTask(
        FilePlaybackProcessorEvents::TaskType::kSeek,
        {.prevState = kCurrState, .val = fpb.getSeekPosition()});
  } else if (property == FilePlayback::kVolume) {
    const float kVol = fpb.getVolume();
    volumeTo(kVol);
  } else if (property == FilePlayback::kPlaybackCommand) {
    const auto kCmd = fpb.getPlaybackCommand();
    switch (kCmd) {
      case FilePlayback::PlaybackCommand::kPlay:
        updateProcessorState(State::kPlaying);
        break;
      case FilePlayback::PlaybackCommand::kPause:
        updateProcessorState(State::kPaused);
        break;
      case FilePlayback::PlaybackCommand::kStop:
        worker_->submitTask(FilePlaybackProcessorEvents::TaskType::kSeek,
                            {.prevState = State::kPaused});
        break;
    }
  }
}