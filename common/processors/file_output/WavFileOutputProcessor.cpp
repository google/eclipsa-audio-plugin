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
    RoomSetupRepository& roomSetupRepository,
    const BinauralMixSource& binauralMixSource)
    : fileExportRepository_(fileExportRepository),
      roomSetupRepository_(roomSetupRepository),
      binauralMixSource_(binauralMixSource),
      numSamples_(0),
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

WavFileOutputProcessor::WavChannelMap
WavFileOutputProcessor::getRoomWavChannelMap(
    Speakers::AudioElementSpeakerLayout layout) {
  using ChannelType = juce::AudioChannelSet::ChannelType;
  const juce::Array<ChannelType> kItuOrder = layout.getITUChannelOrdering();
  WavChannelMap map;
  // Binaural, the one room layout without an ITU order, renders to stereo.
  if (kItuOrder.isEmpty()) {
    map.channelSet = layout.getChannelSet();
    for (int i = 0; i < map.channelSet.size(); ++i) {
      map.sourceChannels[i] = i;
    }
    return map;
  }

  const auto toWavChannel = [](ChannelType type) {
    switch (type) {
      case ChannelType::leftSurroundRear:
        return ChannelType::leftSurround;
      case ChannelType::rightSurroundRear:
        return ChannelType::rightSurround;
      case ChannelType::topSideLeft:
        return ChannelType::topFrontLeft;
      case ChannelType::topSideRight:
        return ChannelType::topFrontRight;
      default:
        return type;
    }
  };
  for (const ChannelType kType : kItuOrder) {
    map.channelSet.addChannel(toWavChannel(kType));
  }
  for (int ituIdx = 0; ituIdx < kItuOrder.size(); ++ituIdx) {
    const int kWavIdx =
        map.channelSet.getChannelIndexForType(toWavChannel(kItuOrder[ituIdx]));
    map.sourceChannels[kWavIdx] = ituIdx;
  }
  return map;
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
  exportBuffer_.setSize(kMaxRoomWavChannels, samplesPerBlock);
}

bool WavFileOutputProcessor::writeExportBlock(
    const juce::AudioBuffer<float>& buffer) {
  const juce::AudioBuffer<float>& source =
      exportingBinaural_ ? binauralMixSource_.getBinauralMix() : buffer;
  const int kNumChannels = exportChannelMap_.channelSet.size();
  const int kNumSamples = buffer.getNumSamples();
  if (source.getNumChannels() < kNumChannels ||
      source.getNumSamples() < kNumSamples) {
    jassertfalse;
    return false;
  }
  // Reallocates only if the host exceeds the block size it prepared with.
  exportBuffer_.setSize(kNumChannels, kNumSamples, false, false, true);
  for (int ch = 0; ch < kNumChannels; ++ch) {
    exportBuffer_.copyFrom(ch, 0, source, exportChannelMap_.sourceChannels[ch],
                           0, kNumSamples);
  }
  return fileWriter_->write(exportBuffer_);
}

void WavFileOutputProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessage) {
  juce::ignoreUnused(midiMessage);
  if (!lock_.tryEnter()) {
    return;  // If someone is changing the realtime setting, we don't need to
             // process anything and should avoid blocking
  }

  bool writeFailed = false;
  if (performingRender_) {
    const bool kInRange = isBlockInExportRange(buffer.getNumSamples());
    if (kInRange && fileWriter_ != nullptr) {
      writeFailed = !writeExportBlock(buffer);
    }
  }
  lock_.exit();

  recordWriteFailureIfAny(!writeFailed);
}

bool WavFileOutputProcessor::isBlockInExportRange(int numSamples) {
  const juce::int64 kCurrentSample = sampleTally_;
  sampleTally_ += numSamples;
  // Same bounds as FileOutputProcessor::shouldBufferBeWritten: a block is
  // written when it starts at or after the start and before the end, and a
  // bound of 0 is unset.
  if (startSampleIdx_ > 0 && kCurrentSample < startSampleIdx_) {
    return false;
  }
  if (endSampleIdx_ > 0 && kCurrentSample >= endSampleIdx_) {
    return false;
  }
  return true;
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
  const juce::SpinLock::ScopedLockType kLock(lock_);
  if (isNonRealtime == performingRender_) {
    return;
  }
  if (isNonRealtime) {
    // Start Rendering
    FileExport configParams = fileExportRepository_.get();
    RoomSetup roomSetup = roomSetupRepository_.get();
    startSampleIdx_ = configParams.getStartSampleIdx();
    endSampleIdx_ = configParams.getEndSampleIdx();
    sampleTally_ = 0;
    const AudioFileFormat kFormat = configParams.getAudioFileFormat();
    if ((kFormat == AudioFileFormat::WAV ||
         kFormat == AudioFileFormat::BinauralWav) &&
        configParams.getExportAudio()) {
      // Every export begins from a clean slate, mirroring
      // FileOutputProcessor::initializeFileExport. Deferred via
      // deferRepositoryUpdate() -- see its declaration for why this can't
      // call fileExportRepository_.update() synchronously.
      deferRepositoryUpdate(
          [](FileExport& config) { config.setExportError(kNoError); });
      hasRecordedWriteFailure_ = false;

      exportingBinaural_ = kFormat == AudioFileFormat::BinauralWav;
      if (exportingBinaural_) {
        exportChannelMap_ = {juce::AudioChannelSet::stereo(), {0, 1}};
      } else {
        exportChannelMap_ = getRoomWavChannelMap(
            roomSetup.getSpeakerLayout().getRoomSpeakerLayout());
      }
      fileWriter_ = createFileWriter(
          configParams.getExportFile(), configParams.getSampleRate(),
          exportChannelMap_.channelSet, 0, configParams.getBitDepth(),
          configParams.getAudioCodec());
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
}

void WavFileOutputProcessor::checkManualExportStartStop(
    const juce::Identifier& property) {
  // Only the manual-export flag starts or stops an export, so a host bounce
  // survives other FileExport edits and an idle processor never opens a file.
  if (property != FileExport::kManualExport) {
    return;
  }
  setNonRealtime(fileExportRepository_.get().getManualExport());
}

void WavFileOutputProcessor::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  checkManualExportStartStop(property);
}
