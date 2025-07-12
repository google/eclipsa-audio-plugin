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

#include "../../RendererProcessor.h"
#include "components/src/AEStripComponent.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/ChannelMonitorData.h"
#include "data_structures/src/MixPresentation.h"
#include "data_structures/src/MixPresentationSoloMute.h"
#include "data_structures/src/RepositoryCollection.h"
#include "logger/logger.h"

class PresentationTab : public juce::Component,
                        public juce::ValueTree::Listener {
 public:
  PresentationTab(juce::Uuid mixPresID, RepositoryCollection repos,
                  ChannelMonitorData& channelMonitorData);

  ~PresentationTab() override;

  void paint(juce::Graphics& g) override;

  std::vector<AudioElement> getAudioElements();

  void updateActiveMixPresentation() {
    MixPresentation mixPres =
        mixPresentationRepository_->get(kmixPresID_).value();

    ActiveMixPresentation activeMix = activeMixRepository_->get();
    activeMix.updateActiveMixId(mixPres.getId());
    activeMixRepository_->update(activeMix);

    // Update the solo/mute state of channels, according to the new active mix
    // presentation first fetch the mixPres object from the repository
    MixPresentationSoloMute mixPresSoloMute =
        mixPresentationSoloMuteRepository_->get(kmixPresID_)
            .value_or(MixPresentationSoloMute());
    std::vector<AudioElementSoloMute> audioElementSoloMutes =
        mixPresSoloMute.getAudioElements();

    if (audioElementSoloMutes.size() != aeStrips_.size()) {
      LOG_ERROR(RendererProcessor::instanceId_,
                "Number of audio elements in solo mute repo does not match the "
                "number of AE strips");
      return;
    }

    for (auto aeStrip : aeStrips_) {
      aeStrip->updateOnActiveMixPresentationChange();
    }
  }

  int calculateHeight();

  int getNumOfAEStrips();

  // max number of AEStripComponents
  const int kAEStripScrollThreshold = 2;
  const int bottomClearance = 29;
  const int topClearance = 26;
  const int stripHeight = 92;
  const int stripSpacing = 5;

 private:
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;

  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;

  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;

  void initializeAudioElements();

  void updateStripLabel(const juce::Uuid& Id);

  void updateStripChannelSet(const juce::Uuid& Id,
                             const int& newStartingChannel);

  AudioElement getAudioElement(const juce::Uuid& Id);

  void createAEStrips();

  void paintAEStrips();

  void resetTab() {
    initializeAudioElements();
    createAEStrips();
    repaint();
  }

  const juce::Uuid kmixPresID_;

  RepositoryCollection repos_;
  AudioElementRepository* audioElementRepository_;
  ActiveMixRepository* activeMixRepository_;
  MixPresentationRepository* mixPresentationRepository_;
  MixPresentationSoloMuteRepository* mixPresentationSoloMuteRepository_;
  juce::OwnedArray<AudioElement> allAudioElementsArray_;  // all audio elements
  std::vector<AudioElement>
      audioElements_;  // the audio elements that belong to this mix
  std::vector<MixPresentationAudioElement> mixpresentationAudioElements_;

  ChannelMonitorData& channelMonitorData_;
  juce::OwnedArray<AEStripComponent> aeStrips_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresentationTab)
};
