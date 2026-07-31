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

#include "WavFileOutputProcessor.h"

#include <logger/logger.h>

#include "WriteFailureClassifier.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/RoomSetupRepository.h"
#include "data_structures/src/FileExport.h"
#include "data_structures/src/RoomSetup.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

WavFileOutputProcessor::WavFileOutputProcessor(
    FileExportRepository& fileExportRepository,
    RoomSetupRepository& roomSetupRepository)
    : fileExportRepository_(fileExportRepository),
      roomSetupRepository_(roomSetupRepository),
      numSamples_(0),
      sampleRate_(0),
      performingRender_(false),
      fileWriter_(nullptr) {
  fileExportRepository_.registerListener(this);
}

WavFileOutputProcessor::~WavFileOutputProcessor() {
  // Ensure this is either deleted by a unit test (nullptr check) or
  // by the message thread since deletion by the audio thread is not safe (lock_
  // is non-reentrant)
  jassert(juce::MessageManager::getInstanceWithoutCreating() == nullptr ||
          juce::MessageManager::getInstanceWithoutCreating()
              ->isThisTheMessageThread());
  *isAlive_ = false;
  fileExportRepository_.deregisterListener(this);
}

void WavFileOutputProcessor::deferRepositoryUpdate(
    std::function<void(FileExport&)> mutator) {
  auto isAlive = isAlive_;
  postToMessageThread([this, isAlive, mutator] {
    if (!*isAlive) return;
    FileExport config = fileExportRepository_.get();
    mutator(config);
    fileExportRepository_.update(config);
  });
}

//==============================================================================
void WavFileOutputProcessor::prepareToPlay(double sampleRate,
                                           int samplesPerBlock) {
  FileExport configParams = fileExportRepository_.get();
  if (sampleRate != configParams.getSampleRate()) {
    configParams.setSampleRate(sampleRate);
    fileExportRepository_.update(configParams);
  }

  numSamples_ = samplesPerBlock;
}

void WavFileOutputProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessage) {
  juce::ignoreUnused(midiMessage);
  if (!lock_.tryEnter()) {
    return;  // If someone is changing the realtime setting, we don't need to
             // process anything and should avoid blocking
  }

  bool writeFailed = false;
  if (performingRender_ && fileWriter_ != nullptr) {
    // If the current position is between the configured start and end position,
    // write to the file. Else do nothing
    auto playHead = getPlayHead();
    if (playHead != NULL) {  // Happens in debug
      auto position = getPlayHead()->getPosition();
      const juce::int64 currentSample =
          static_cast<juce::int64>(*position->getTimeInSeconds() * sampleRate_);
      if ((endSampleIdx_ == 0) || (currentSample >= startSampleIdx_ &&
                                   currentSample <= endSampleIdx_)) {
        writeFailed = !fileWriter_->write(buffer);
      }
    } else {
      writeFailed = !fileWriter_->write(buffer);
    }
  }
  lock_.exit();

  recordWriteFailureIfAny(!writeFailed);
}

void WavFileOutputProcessor::recordWriteFailureIfAny(bool writeSucceeded) {
  if (writeSucceeded) {
    return;
  }
  // A persistent failure (disk full, WAV size limit exceeded) fails every
  // subsequent block. Only the first failure of an export needs to be
  // recorded.' exchange() both checks and claims that slot atomically;
  // hasRecordedWriteFailure_ is reset in setNonRealtime() at the start of the
  // next export.
  if (hasRecordedWriteFailure_.exchange(true)) {
    return;
  }
  // Record it unless a more specific error is already on record, mirroring
  // the guard used by FileOutputProcessor for the IAMF/per-element paths.
  // Deferred via deferRepositoryUpdate() -- see its declaration for why this
  // can't call fileExportRepository_.update() synchronously.
  deferRepositoryUpdate([](FileExport& config) {
    config.recordExportErrorIfUnset(kFileWriteFailed);
  });
}

void WavFileOutputProcessor::setNonRealtime(bool isNonRealtime) noexcept {
  lock_.enter();
  if (!performingRender_) {
    // Start Rendering
    FileExport configParams = fileExportRepository_.get();
    RoomSetup roomSetup = roomSetupRepository_.get();
    startSampleIdx_ = configParams.getStartSampleIdx();
    endSampleIdx_ = configParams.getEndSampleIdx();
    if ((configParams.getAudioFileFormat() == AudioFileFormat::WAV) &&
        configParams.getExportAudio()) {
      // Every export begins from a clean slate, mirroring
      // FileOutputProcessor::initializeFileExport. Deferred via
      // deferRepositoryUpdate() -- see its declaration for why this can't
      // call fileExportRepository_.update() synchronously.
      deferRepositoryUpdate(
          [](FileExport& config) { config.setExportError(kNoError); });
      hasRecordedWriteFailure_ = false;

      fileWriter_ = createFileWriter(
          configParams.getExportFile(), configParams.getSampleRate(),
          roomSetup.getSpeakerLayout().getRoomSpeakerLayout().getNumChannels(),
          0, configParams.getBitDepth(), configParams.getAudioCodec());
      if (!fileWriter_->isOpen()) {
        const std::string kFailedFilePath = fileWriter_->getFilePath();
        LOG_ERROR(0,
                  "WavFileOutputProcessor: Failed to open file for writing: " +
                      kFailedFilePath);
        deferRepositoryUpdate([kFailedFilePath](FileExport& config) {
          config.recordExportErrorIfUnset(
              classifyWriteFailure(juce::String(kFailedFilePath)));
        });
      }
      performingRender_ = true;
    }
  } else {
    // Complete Rendering
    if (fileWriter_ != nullptr) {
      if (!fileWriter_->close()) {
        // The writer was opened successfully but failed to close/flush --
        // an unreported write failure. Only escalate if nothing more
        // specific has already been recorded earlier in this export.
        deferRepositoryUpdate([](FileExport& config) {
          config.recordExportErrorIfUnset(kFileWriteFailed);
        });
      }
      delete fileWriter_;
      fileWriter_ = nullptr;
    }
    performingRender_ = false;
  }
  lock_.exit();
}

void WavFileOutputProcessor::checkManualExportStartStop() {
  FileExport configParams = fileExportRepository_.get();
  if (performingRender_ != configParams.getManualExport()) {
    setNonRealtime(configParams.getManualExport());
  }
}
void WavFileOutputProcessor::valueTreeRedirected(
    juce::ValueTree& treeWhichHasBeenChanged) {
  checkManualExportStartStop();
}
void WavFileOutputProcessor::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  checkManualExportStartStop();
}
void WavFileOutputProcessor::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  checkManualExportStartStop();
}
void WavFileOutputProcessor::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  checkManualExportStartStop();
}