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
                              audioElementRepo),
      estimatedSamplesToProcess_(0),
      processedSamples_(0),
      exportCompleted_(false) {
  // mixPresentationRepository_.registerListener(this);
  LOG_INFO(0, "PremierePro LoudnessExport Processor Instantiated \n");
}

PremiereProLoudnessExportProcessor::~PremiereProLoudnessExportProcessor() {
  LOG_ANALYTICS(0, "LoudnessExportProcessor_PremierePro destructor called");
}

void PremiereProLoudnessExportProcessor::releaseResources() {
  // Release any resources held by the processor
  LOG_ANALYTICS(0, "LoudnessExportProcessor_PremierePro releasing resources");
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
      exportCompleted_ = false;
    }
    return;
  }

  // Stop rendering if we are switching back to online mode
  // copy loudness values from the map to the repository
  if (!isNonRealtime && exportCompleted_ && performingRender_) {
    performingRender_ = false;
    exportCompleted_ = false;  // ready for the next export
    LOG_ANALYTICS(0, "copying loudness metadata to repository");
    for (auto& exportContainer : exportContainers_) {
      copyExportContainerDataToRepo(exportContainer);
    }
  }
}

void PremiereProLoudnessExportProcessor::prepareToPlay(double sampleRate,
                                                       int samplesPerBlock) {
  FileExport config = fileExportRepository_.get();
  sampleRate_ = sampleRate;
  currentSamplesPerBlock_ = samplesPerBlock;
  sampleTally_ = 0;
  intializeExportContainers();

  processedSamples_ = 0;

  int totalDuration = config.getEndTime() - config.getStartTime();

  estimatedSamplesToProcess_ = static_cast<int>(totalDuration * sampleRate);
}

void PremiereProLoudnessExportProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  // If we are not performing a render, return
  if (!areLoudnessCalcsRequired(buffer)) {
    return;
  }

  if (processedSamples_ + buffer.getNumSamples() < estimatedSamplesToProcess_) {
    for (auto& exportContainer : exportContainers_) {
      exportContainer.process(buffer);
    }
  }

  processedSamples_ += buffer.getNumSamples();

  if (processedSamples_ >= estimatedSamplesToProcess_ && !exportCompleted_) {
    LOG_ANALYTICS(0, "exportCompleted_ = true");
    exportCompleted_ = true;
    setNonRealtime(false);
  }
}
