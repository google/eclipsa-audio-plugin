/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include "MixPresentationLoudnessExportContainer.h"

class LoudnessExportProcessor : public ProcessorBase,
                                public juce::ValueTree::Listener {
 public:
  using EBU128Stats = MeasureEBU128::LoudnessStats;

  void setNonRealtime(bool isNonRealtime) noexcept override;

  LoudnessExportProcessor(FileExportRepository& fileExportRepo,
                          MixPresentationRepository& mixPresentationRepo,
                          MixPresentationLoudnessRepository& loudnessRepo,
                          AudioElementRepository& audioElementRepo);

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  void processBlock(juce::AudioBuffer<float>& buffer,
                    juce::MidiBuffer& midiMessages) override;

  const std::vector<const MixPresentationLoudnessExportContainer*>
  getExportContainers() const {
    std::vector<const MixPresentationLoudnessExportContainer*> containers(
        exportContainers_.size());
    for (int i = 0; i < exportContainers_.size(); i++) {
      containers[i] = &exportContainers_[i];
    }
    return containers;
  }

 protected:
  void copyExportContainerDataToRepo(
      const MixPresentationLoudnessExportContainer& exportContainer);

  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;

  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;

  void handleNewLayoutAdded(juce::ValueTree& parentTree,
                            juce::ValueTree& childWhichHasBeenAdded);

  Speakers::AudioElementSpeakerLayout getLargestLayoutFromTree(
      juce::ValueTree& mixPresentationAudioElementsTree);

  void initializeLoudnessExport();

  void intializeExportContainers();

  bool performingRender_;

  FileExportRepository& fileExportRepository_;
  MixPresentationRepository& mixPresentationRepository_;
  MixPresentationLoudnessRepository& loudnessRepo_;
  AudioElementRepository& audioElementRepository_;

  long sampleRate_;
  int currentSamplesPerBlock_;
  int sampleTally_;
  int startTime_;
  int endTime_;

  std::vector<MixPresentationLoudnessExportContainer> exportContainers_;
  juce::OwnedArray<MixPresentationLoudness> mixPresentationLoudnesses_;
};