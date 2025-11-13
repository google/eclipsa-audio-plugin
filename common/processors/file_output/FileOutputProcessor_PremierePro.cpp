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

#include "FileOutputProcessor_PremierePro.h"

#include <string>

#include "data_structures/src/FileExport.h"
#include "logger/logger.h"

//==============================================================================
PremiereProFileOutputProcessor::PremiereProFileOutputProcessor(
    FileExportRepository& fileExportRepository,
    AudioElementRepository& audioElementRepository,
    MixPresentationRepository& mixPresentationRepository,
    MixPresentationLoudnessRepository& mixPresentationLoudnessRepository)
    : FileOutputProcessor(fileExportRepository, audioElementRepository,
                          mixPresentationRepository,
                          mixPresentationLoudnessRepository) {}

PremiereProFileOutputProcessor::~PremiereProFileOutputProcessor() {
  LOG_ANALYTICS(0, "FileOutputProcessor_PremierePro destructor called");
  if (performingRender_) {
    FileExport config = fileExportRepository_.get();
    closeFileExport(config);
    performingRender_ = false;
  }
}

//==============================================================================
void PremiereProFileOutputProcessor::prepareToPlay(double sampleRate,
                                                   int samplesPerBlock) {
  FileExport config = fileExportRepository_.get();

  if (sampleRate != config.getSampleRate()) {
    LOG_ANALYTICS(0, "FileOutputProcessor_PremierePro sample rate changed to " +
                         std::to_string(sampleRate));
    config.setSampleRate(sampleRate);

    fileExportRepository_.update(config);
  }

  numSamples_ = samplesPerBlock;
  sampleTally_ = 0;
  sampleRate_ = sampleRate;
}

void PremiereProFileOutputProcessor::setNonRealtime(
    bool isNonRealtime) noexcept {
  FileExport config = fileExportRepository_.get();

  if (!config.getManualExport()) {
    performingRender_ = false;
    return;
  }

  // Initialize the writer if we are rendering in offline mode
  if (isNonRealtime && !performingRender_) {
    if ((config.getAudioFileFormat() == AudioFileFormat::IAMF) &&
        (config.getExportAudio())) {
      initializeFileExport(config);
    }
    return;
  }
}

void PremiereProFileOutputProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  if (!shouldBufferBeWritten(buffer)) {
    return;
  }

  //  Write the audio data to the wav file writers
  for (auto& writer : iamfWavFileWriters_) {
    writer->write(buffer);
  }

  iamfFileWriter_->writeFrame(buffer);
}
