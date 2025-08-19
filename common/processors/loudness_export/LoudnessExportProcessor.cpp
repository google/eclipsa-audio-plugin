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

#include "LoudnessExportProcessor.h"

#include "../rendererplugin/src/RendererProcessor.h"
#include "data_structures/src/FileExport.h"

LoudnessExportProcessor::LoudnessExportProcessor(
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
      sampleTally_(0) {
  mixPresentationRepository_.registerListener(this);
}

LoudnessExportProcessor::~LoudnessExportProcessor() {
  mixPresentationRepository_.deregisterListener(this);
}

void LoudnessExportProcessor::setNonRealtime(bool isNonRealtime) noexcept {
  if (isNonRealtime == performingRender_) {
    return;
  }

  // Initialize the writer if we are rendering in offline mode
  if (!performingRender_) {
    FileExport config = fileExportRepository_.get();
    if ((config.getAudioFileFormat() == AudioFileFormat::IAMF) &&
        (config.getExportAudio())) {
      initializeLoudnessExport(config);
    }
    return;
  }

  // Stop rendering if we are switching back to online mode
  // copy loudness values from the map to the repository
  if (performingRender_) {
    for (auto& exportContainer : exportContainers_) {
      copyExportContainerDataToRepo(exportContainer);
    }
    performingRender_ = false;
    LOG_INFO(0, "Copied loudness metadata to repository \n");
  }
}

void LoudnessExportProcessor::prepareToPlay(double sampleRate,
                                            int samplesPerBlock) {
  sampleRate_ = sampleRate;
  currentSamplesPerBlock_ = samplesPerBlock;
  sampleTally_ = 0;
  intializeExportContainers();
}

void LoudnessExportProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages) {
  // kick out of process block if there is no nothing to render
  if (!areLoudnessCalcsRequired(buffer)) {
    return;
  }

  for (auto& exportContainer : exportContainers_) {
    exportContainer.process(buffer);
  }
}

void LoudnessExportProcessor::copyExportContainerDataToRepo(
    const MixPresentationLoudnessExportContainer& exportContainer) {
  EBU128Stats stereoLoudnessStats;
  exportContainer.loudnessExportData->stereoEBU128.read(stereoLoudnessStats);
  // define a minValue for the loudness values
  // ensures that .iamf file output does not fail
  const float minValue = -80.f;
  std::optional<MixPresentationLoudness> loudnessOpt =
      loudnessRepo_.get(exportContainer.mixPresentationId);
  if (!loudnessOpt.has_value()) {
    LOG_ERROR(RendererProcessor::instanceId_,
              "LoudnessExportProcessor, copyExportContainerDataToRepo: Could "
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

void LoudnessExportProcessor::valueTreeChildAdded(
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

void LoudnessExportProcessor::valueTreeChildRemoved(
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

void LoudnessExportProcessor::handleNewLayoutAdded(
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
LoudnessExportProcessor::getLargestLayoutFromTree(
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

void LoudnessExportProcessor::intializeExportContainers() {
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

void LoudnessExportProcessor::initializeLoudnessExport(FileExport& config) {
  performingRender_ = true;

  LOG_INFO(0,
           "Beginning loudness metadata calculations for .iamf file export \n");

  sampleRate_ = config.getSampleRate();
  sampleTally_ = 0;
  startTime_ = config.getStartTime();
  endTime_ = config.getEndTime();

  intializeExportContainers();
}

bool LoudnessExportProcessor::areLoudnessCalcsRequired(
    const juce::AudioBuffer<float>& buffer) {
  if (!performingRender_ || buffer.getNumSamples() < 1) {
    return false;
  }

  // Safety check to prevent division by zero during auval testing
  if (sampleRate_ <= 0) {
    return false;
  }

  // Calculate the current time with the existing number of samples that have
  // been processed
  long currentTime = sampleTally_ / sampleRate_;
  // update the sample tally
  sampleTally_ += buffer.getNumSamples();
  // with the updated sample tally, calculate the next time
  long nextTime = sampleTally_ / sampleRate_;

  if (startTime_ != 0 || endTime_ != 0) {
    // Handle the case where startTime and endTime are set, implying we
    // are only bouncing a subset of the mix

    // do not render
    if (currentTime < startTime_ || nextTime > endTime_) {
      return false;
    }
  }
  return true;
}
