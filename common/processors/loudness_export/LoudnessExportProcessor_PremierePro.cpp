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

#include "LoudnessExportProcessor_PremierePro.h"

#include "../rendererplugin/src/RendererProcessor.h"
#include "data_structures/src/FileExport.h"
#include "processors/loudness_export/LoudnessExportProcessor.h"

PremiereProLoudnessExportProcessor::PremiereProLoudnessExportProcessor(
    FileExportRepository& fileExportRepo,
    MixPresentationRepository& mixPresentationRepo,
    MixPresentationLoudnessRepository& loudnessRepo,
    AudioElementRepository& audioElementRepo)
    : LoudnessExportProcessor(fileExportRepo, mixPresentationRepo, loudnessRepo,
                              audioElementRepo) {
  LOG_INFO(0, "PremierePro LoudnessExport Processor Instantiated \n");
}

PremiereProLoudnessExportProcessor::~PremiereProLoudnessExportProcessor() {
  LOG_ANALYTICS(0, "LoudnessExportProcessor_PremierePro destructor called");

  if (performingRender_) {
    LOG_ANALYTICS(0, "copying loudness metadata to repository");
    for (auto& exportContainer : exportContainers_) {
      copyExportContainerDataToRepo(exportContainer);
    }
    performingRender_ = false;
  }
}

void PremiereProLoudnessExportProcessor::setNonRealtime(
    bool isNonRealtime) noexcept {
  LOG_ANALYTICS(0, std::string("LoudnessExport_PremierePro Set Non-Realtime ") +
                       (isNonRealtime ? "true" : "false"));
  FileExport config = fileExportRepository_.get();

  if (!config.getManualExport()) {
    performingRender_ = false;
    return;
  }

  // Initialize the writer if we are rendering in offline mode
  if (!performingRender_ && isNonRealtime) {
    FileExport config = fileExportRepository_.get();
    if ((config.getAudioFileFormat() == AudioFileFormat::IAMF) &&
        (config.getExportAudio())) {
      initializeLoudnessExport(config);
    }
    return;
  }
}

void PremiereProLoudnessExportProcessor::prepareToPlay(double sampleRate,
                                                       int samplesPerBlock) {
  sampleRate_ = sampleRate;
  currentSamplesPerBlock_ = samplesPerBlock;
  sampleTally_ = 0;
  intializeExportContainers();
}

void PremiereProLoudnessExportProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  // If we are not performing a render, return
  if (!areLoudnessCalcsRequired(buffer)) {
    return;
  }

  for (auto& exportContainer : exportContainers_) {
    exportContainer.process(buffer);
  }
}
