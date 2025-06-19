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

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../rendererplugin/src/RendererProcessor.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/FileExport.h"
#include "data_structures/src/LoudnessExportData.h"
#include "data_structures/src/MixPresentation.h"
#include "data_structures/src/MixPresentationLoudness.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "logger/logger.h"
#include "processors/mix_monitoring/loudness_standards/MeasureEBU128.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

PremiereProLoudnessExportProcessor::PremiereProLoudnessExportProcessor(
    FileExportRepository& fileExportRepo,
    MixPresentationRepository& mixPresentationRepo,
    MixPresentationLoudnessRepository& loudnessRepo,
    AudioElementRepository& audioElementRepo)
    : performingRender_(false),
      fileExportRepository_(fileExportRepo),
      mixPresentationRepository_(mixPresentationRepo),
      loudnessRepo_(loudnessRepo),
      audioElementRepository_(audioElementRepo),
      currentSamplesPerBlock_(1),
      sampleTally_(0),
      estimatedSamplesToProcess_(0),
      processedSamples_(0) {
  mixPresentationRepository_.registerListener(this);
  LOG_INFO(0, "PremierePro LoudnessExport Processor Instantiated \n");
}

PremiereProLoudnessExportProcessor::~PremiereProLoudnessExportProcessor() {
  FileExport fileExportConfig = fileExportRepository_.get();
  if (fileExportConfig.getInitiatedPremiereProExport()) {
    LOG_ANALYTICS(0,
                  "PremiereProLoudnessExportProcessor destructor called w/ "
                  "Export Initiated");
    // performingRender_ = false;
    // exportCompleted_ = false;
    // fileExportConfig.setInitiatedPremiereProExport(false);
    // fileExportRepository_.update(fileExportConfig);
    // LOG_INFO(0,
    //          "PremiereProLoudnessExportProcessor is being destroyed but
    //          Export " "completed, " "copying loudness data to repository.");
    // for (auto& exportContainer : exportContainers_) {
    //   copyExportContainerDataToRepo(exportContainer);
    // }
  }
  mixPresentationRepository_.deregisterListener(this);
}

void PremiereProLoudnessExportProcessor::setNonRealtime(
    bool isNonRealtime) noexcept {
  LOG_ANALYTICS(0,
                std::string("LoudnessExport Premiere Pro Set Non-Realtime ") +
                    (isNonRealtime ? "true" : "false"));
  if (isNonRealtime == performingRender_) {
    return;
  }

  FileExport config = fileExportRepository_.get();
  // Initialize the writer if we are rendering in offline mode
  if (!performingRender_ && isNonRealtime && !exportCompleted_) {
    if ((config.getAudioFileFormat() == AudioFileFormat::IAMF) &&
        (config.getExportAudio())) {
      performingRender_ = true;

      LOG_INFO(
          0,
          "Beginning loudness metadata calculations for .iamf file export \n");

      sampleRate_ = config.getSampleRate();
      sampleTally_ = 0;
      startTime_ = config.getStartTime();
      endTime_ = config.getEndTime();

      // Get all mix presentation loudnesses from the repository
      loudnessRepo_.getAll(mixPresentationLoudnesses_);

      intializeExportContainers();

      FileExport fileExportConfig = fileExportRepository_.get();
      fileExportConfig.setInitiatedPremiereProExport(true);
      fileExportRepository_.update(fileExportConfig);
    }
    return;
  }

  LOG_ANALYTICS(
      0,
      std::string(
          "LoudnessExport PremierePro Set Non-Realtime, exportCompleted_ = ") +
          (exportCompleted_ ? "true" : "false"));

  LOG_ANALYTICS(
      0, std::string(
             "LoudnessExport PremierePro Set Non-Realtime, setNonRealtime = ") +
             (isNonRealtime ? "true" : "false"));

  // Stop rendering if we are switching back to online mode
  // copy loudness values from the map to the repository
  if (!isNonRealtime && exportCompleted_) {
    LOG_ANALYTICS(0, "copying loudness metadata to repository");
    for (auto& exportContainer : exportContainers_) {
      copyExportContainerDataToRepo(exportContainer);
    }
    performingRender_ = false;
    exportCompleted_ = false;
    FileExport fileExportConfig = fileExportRepository_.get();
    fileExportConfig.setInitiatedPremiereProExport(false);
    fileExportRepository_.update(fileExportConfig);
  }
}

void PremiereProLoudnessExportProcessor::prepareToPlay(double sampleRate,
                                                       int samplesPerBlock) {
  FileExport config = fileExportRepository_.get();

  if (config.getInitiatedPremiereProExport() && config.getManualExport()) {
    performingRender_ = true;
    exportCompleted_ = false;
  }

  LOG_ANALYTICS(0, "LoudnessExport_PremiereProProcessor prepareToPlay");
  sampleRate_ = sampleRate;
  currentSamplesPerBlock_ = samplesPerBlock;
  sampleTally_ = 0;
  intializeExportContainers();

  processedSamples_ = 0;

  int totalDuration = config.getEndTime() - config.getStartTime();

  estimatedSamplesToProcess_ = static_cast<int>(totalDuration * sampleRate);
  // LOG_ANALYTICS(0, "LoudnessExport PremierePro, totalDuration: " +
  //                      std::to_string(totalDuration) +
  //                      ", Estimated samples to process: " +
  //                      std::to_string(estimatedSamplesToProcess_) + "\n");
}

void PremiereProLoudnessExportProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  // If we are not performing a render, return
  if (!performingRender_ || buffer.getNumSamples() < 1) {
    return;
  }

  // calculate the current and next time based on the sample tally
  double currentTime = static_cast<double>(sampleTally_) / sampleRate_;
  sampleTally_ += buffer.getNumSamples();
  double nextTime = static_cast<double>(sampleTally_) / sampleRate_;

  // do not render
  if (currentTime < startTime_ || nextTime > endTime_) {
    return;
  }

  if (processedSamples_ <= estimatedSamplesToProcess_) {
    // LOG_ANALYTICS(0, "Processing an additional " +
    //                      std::to_string(buffer.getNumSamples()) +
    //                      " samples. Already processed " +
    //                      std::to_string(processedSamples_) + " of " +
    //                      std::to_string(estimatedSamplesToProcess_));
    // std::string sampleString = "Processing samples: ";
    // // clear the buffer for each channel
    // sampleString += std::to_string(buffer.getSample(0, 0)) + ", ";
    // sampleString += std::to_string(buffer.getSample(0, 50)) + ", ";
    // sampleString += std::to_string(buffer.getSample(0, 100)) + ", ";
    // sampleString += std::to_string(buffer.getSample(0, 150)) + ", ";
    // sampleString += std::to_string(buffer.getSample(0, 200)) + "\n";

    // LOG_ANALYTICS(0, sampleString);
    processedSamples_ += buffer.getNumSamples();

    for (auto& exportContainer : exportContainers_) {
      exportContainer.process(buffer);
    }
  } else if (!exportCompleted_) {
    exportCompleted_ = true;
    performingRender_ = false;

    setNonRealtime(false);

    LOG_ANALYTICS(0, "explortCompleted_ = true");
  }
}

void PremiereProLoudnessExportProcessor::copyExportContainerDataToRepo(
    const PremiereProExportContainer& exportContainer) {
  EBU128Stats stereoLoudnessStats;
  exportContainer.loudnessExportData->stereoEBU128.read(stereoLoudnessStats);
  // define a minValue for the loudness values
  // ensures that .iamf file output does not fail
  const float minValue = -80.f;
  std::optional<MixPresentationLoudness> loudnessOpt =
      loudnessRepo_.get(exportContainer.mixPresentationId);
  if (!loudnessOpt.has_value()) {
    LOG_ERROR(RendererProcessor::instanceId_,
              "PremiereProLoudnessExportProcessor, "
              "copyExportContainerDataToRepo: Could "
              "not find "
              "MixPresentation in Repository w/ Uuid: " +
                  exportContainer.mixPresentationId.toString().toStdString());
  }

  MixPresentationLoudness mixPresLoudness = loudnessOpt.value();
  mixPresLoudness.setLayoutIntegratedLoudness(
      Speakers::kStereo,
      std::max(minValue, stereoLoudnessStats.loudnessIntegrated));

  mixPresLoudness.setLayoutTruePeak(
      Speakers::kStereo,
      std::max(minValue, stereoLoudnessStats.loudnessTruePeak));

  mixPresLoudness.setLayoutDigitalPeak(
      Speakers::kStereo,
      std::max(minValue, stereoLoudnessStats.loudnessDigitalPeak));

  if (mixPresLoudness.getLargestLayout() != Speakers::kStereo) {
    const Speakers::AudioElementSpeakerLayout layout =
        mixPresLoudness.getLargestLayout();
    EBU128Stats layoutLoudnessStats;
    exportContainer.loudnessExportData->layoutEBU128.read(layoutLoudnessStats);
    mixPresLoudness.setLayoutIntegratedLoudness(
        layout, std::max(minValue, layoutLoudnessStats.loudnessIntegrated));
    mixPresLoudness.setLayoutTruePeak(
        layout, std::max(minValue, layoutLoudnessStats.loudnessTruePeak));
    mixPresLoudness.setLayoutDigitalPeak(
        layout, std::max(minValue, layoutLoudnessStats.loudnessDigitalPeak));
  }
  loudnessRepo_.update(mixPresLoudness);
}

void PremiereProLoudnessExportProcessor::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  // handle the case of adding a new mix presentation
  if (childWhichHasBeenAdded.getType() == MixPresentation::kTreeType) {
    // update the MixPresentationLoudness Repository by adding the new mix
    loudnessRepo_.add(MixPresentationLoudness(
        juce::Uuid(childWhichHasBeenAdded[MixPresentation::kId])));
  } else if (childWhichHasBeenAdded.getType() ==
                 MixPresentation::kAudioElements &&
             parentTree.getType() == MixPresentation::kTreeType) {
    // update the MixPresentationLoudness Repository
    handleNewLayoutAdded(parentTree, childWhichHasBeenAdded);
  }
}

void PremiereProLoudnessExportProcessor::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  if (childWhichHasBeenRemoved.getType() == MixPresentation::kTreeType) {
    // update the MixPresentationLoudness Repository by removing the mix
    juce::Uuid mixPresID =
        juce::Uuid(childWhichHasBeenRemoved[MixPresentation::kId]);
    MixPresentationLoudness mixPresLoudness =
        loudnessRepo_.get(mixPresID).value();
    loudnessRepo_.remove(mixPresLoudness);
  }
}

void PremiereProLoudnessExportProcessor::handleNewLayoutAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  // this function is only for handling a new Audio Element Layout
  jassert(parentTree.getType() == MixPresentation::kTreeType);
  jassert(childWhichHasBeenAdded.getType() == MixPresentation::kAudioElements);

  const juce::Uuid mixPresentationId =
      juce::Uuid(parentTree[MixPresentation::kId]);

  // Retrieve the audio element layout that was added to the mix presentation
  // it should be the last child in the mix presentation audio elements tree
  Speakers::AudioElementSpeakerLayout layout =
      getLargestLayoutFromTree(childWhichHasBeenAdded);

  MixPresentationLoudness mixPresLoudness =
      loudnessRepo_.get(mixPresentationId).value();

  // update the repository
  mixPresLoudness.replaceLargestLayout(layout);
  loudnessRepo_.update(mixPresLoudness);
}

Speakers::AudioElementSpeakerLayout
PremiereProLoudnessExportProcessor::getLargestLayoutFromTree(
    juce::ValueTree& mixPresentationAudioElementsTree) {
  Speakers::AudioElementSpeakerLayout largestLayout = Speakers::kStereo;
  for (int i = 0; i < mixPresentationAudioElementsTree.getNumChildren(); i++) {
    juce::Uuid audioElementId =
        juce::Uuid(mixPresentationAudioElementsTree.getChild(
            i)[MixPresentationAudioElement::kId]);
    AudioElement audioElement =
        audioElementRepository_.get(audioElementId).value();
    Speakers::AudioElementSpeakerLayout layout =
        audioElement.getChannelConfig();

    // If the layout added, is stereo, mono, ambisonics, binaural, or has less
    // channels than the current highest layout, we do nothing
    if (layout == Speakers::kStereo || layout == Speakers::kMono ||
        layout.isAmbisonics() || layout == Speakers::kBinaural ||
        layout.getNumChannels() < largestLayout.getNumChannels()) {
      continue;
    }

    if (layout.getNumChannels() == largestLayout.getNumChannels()) {
      // convert the layout enum to ints
      int layoutInt = static_cast<int>(layout);
      int largestLayoutInt = static_cast<int>(largestLayout);
      // if the layout added is less than or equal to the current largest
      // layout, do nothing
      if (layoutInt <= largestLayoutInt) {
        continue;
      }
    }
    largestLayout = layout;
  }
  return largestLayout;
}

void PremiereProLoudnessExportProcessor::intializeExportContainers() {
  // clear the current renderers
  exportContainers_.clear();

  // get the current mix presentation
  juce::OwnedArray<MixPresentation> mixPresentations;
  mixPresentationRepository_.getAll(mixPresentations);
  if (mixPresentations.size() == 0) {
    return;
  }

  exportContainers_.reserve(mixPresentations.size());

  // for each mix presentation, get all audio elements
  for (int i = 0; i < mixPresentations.size(); i++) {
    std::vector<MixPresentationAudioElement> mixPresAudioElements =
        mixPresentations[i]->getAudioElements();
    std::vector<AudioElement> audioElementsVec(mixPresAudioElements.size());
    for (int j = 0; j < mixPresAudioElements.size(); j++) {
      // get the audio element from the repository
      AudioElement audioElement =
          audioElementRepository_.get(mixPresAudioElements[j].getId()).value();
      audioElementsVec[j] = audioElement;
    }
    exportContainers_.emplace_back(
        mixPresentations[i]->getId(), mixPresentations[i]->getDefaultMixGain(),
        sampleRate_, currentSamplesPerBlock_,
        loudnessRepo_.get(mixPresentations[i]->getId())->getLargestLayout(),
        audioElementsVec);
  }
}
