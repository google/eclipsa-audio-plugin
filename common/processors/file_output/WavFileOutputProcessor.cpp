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
  fileExportRepository_.deregisterListener(this);
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

  if (performingRender_ && fileWriter_ != nullptr) {
    // If the current position is between the configured start and end position,
    // write to the file. Else do nothing
    auto playHead = getPlayHead();
    if (playHead != NULL) {  // Happens in debug
      auto position = getPlayHead()->getPosition();
      if ((endTime_ == 0) || (position->getTimeInSeconds() >= startTime_ &&
                              position->getTimeInSeconds() <= endTime_)) {
        fileWriter_->write(buffer);
      }
    } else {
      fileWriter_->write(buffer);
    }
  }
  lock_.exit();
}

void WavFileOutputProcessor::setNonRealtime(bool isNonRealtime) noexcept {
  lock_.enter();
  if (!performingRender_) {
    // Start Rendering
    FileExport configParams = fileExportRepository_.get();
    RoomSetup roomSetup = roomSetupRepository_.get();
    startTime_ = configParams.getStartTime();
    endTime_ = configParams.getEndTime();
    if ((configParams.getAudioFileFormat() == AudioFileFormat::WAV) &&
        configParams.getExportAudio()) {
      fileWriter_ = new FileWriter(
          configParams.getExportFile(), configParams.getSampleRate(),
          roomSetup.getSpeakerLayout().getRoomSpeakerLayout().getNumChannels(),
          0, configParams.getBitDepth(), configParams.getAudioCodec());
      performingRender_ = true;
    }
  } else {
    // Complete Rendering
    if (fileWriter_ != nullptr) {
      fileWriter_->close();
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