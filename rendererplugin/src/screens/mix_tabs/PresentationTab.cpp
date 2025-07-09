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

#include "PresentationTab.h"

#include <cstddef>
#include <optional>

#include "data_structures/src/ChannelMonitorData.h"
#include "data_structures/src/RepositoryCollection.h"

PresentationTab::PresentationTab(juce::Uuid mixPresID,
                                 RepositoryCollection repos,
                                 ChannelMonitorData& channelMonitorData)
    : repos_(repos),
      audioElementRepository_(&repos.aeRepo_),
      kmixPresID_(mixPresID),
      multichannelGainRepo_(&repos.chGainRepo_),
      activeMixRepository_(&repos.activeMPRepo_),
      channelMonitorData_(channelMonitorData),
      mixPresentationRepository_(&repos.mpRepo_),
      mixPresentationSoloMuteRepository_(&repos.mpSMRepo_) {
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "PresentationTab created for MixPresentation");

  mixPresentationRepository_->registerListener(this);
  audioElementRepository_->registerListener(this);
  // get the audio elements for this tab
  initializeAudioElements();

  // Create the AE Strips
  createAEStrips();
}

PresentationTab::~PresentationTab() {
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "PresentationTab destroyed for MixPresentation");
  setLookAndFeel(nullptr);
  mixPresentationRepository_->deregisterListener(this);
  audioElementRepository_->deregisterListener(this);
}

void PresentationTab::paint(juce::Graphics& g) {
  // nothing to paint
  if (aeStrips_.isEmpty()) {
    return;
  }
  auto bounds = getLocalBounds();

  g.setColour(juce::Colours::transparentWhite);
  // Assign 90% of the top to aeStripComponentBounds
  auto aeStripComponentBounds = bounds;
  aeStripComponentBounds.removeFromBottom(bottomClearance);
  // Remove the top 10% for additional clearance
  aeStripComponentBounds.removeFromTop(topClearance);

  auto newbounds = aeStripComponentBounds;
  // Remove 10% from the left & right
  newbounds.removeFromLeft(aeStripComponentBounds.getWidth() * 0.08);
  newbounds.removeFromRight(aeStripComponentBounds.getWidth() * 0.08);

  for (int i = 0; i < aeStrips_.size(); i++) {
    // Allocate bounds for the top AE Strip
    auto topBounds = newbounds.removeFromTop(
        stripHeight);  // will allocate 70% of the height to
                       // both AE Strips (35% for each)
    AEStripComponent* strip = aeStrips_[i];
    const float fraction =
        (strip->getChannelCount() + 2.5f) /
        (Speakers::kHOA3.getNumChannels() +
         2.5f);  // third order ambisonics will have the widest strip

    strip->setBounds(topBounds.removeFromLeft(newbounds.getWidth() * fraction));

    // create some space
    newbounds.removeFromTop(stripSpacing);
    strip->getBounds();
  }
}

MultiChannelRepository* PresentationTab::getMultiChannelRepository() {
  return multichannelGainRepo_;
}

std::vector<AudioElement> PresentationTab::getAudioElements() {
  return audioElements_;
}

void PresentationTab::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  if (parentTree.getType() == MixPresentation::kTreeType &&
      juce::Uuid(parentTree[MixPresentation::kId]) == kmixPresID_) {
    resetTab();
  }
}

void PresentationTab::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  if (childWhichHasBeenRemoved.getType() == MixPresentation::kAudioElements &&
      juce::Uuid(parentTree[MixPresentation::kId]) == kmixPresID_) {
    resetTab();
  }
}

void PresentationTab::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  if (treeWhosePropertyHasChanged.getType() == AudioElement::kTreeType &&
      property == AudioElement::kName) {
    updateStripLabel(
        juce::Uuid(treeWhosePropertyHasChanged[AudioElement::kId]));
  } else if (treeWhosePropertyHasChanged.getType() == AudioElement::kTreeType &&
             property == AudioElement::kFirstChannel) {
    // update channel set in strip
    updateStripChannelSet(
        juce::Uuid(treeWhosePropertyHasChanged[AudioElement::kId]),
        treeWhosePropertyHasChanged[property]);
  }
}

void PresentationTab::initializeAudioElements() {
  allAudioElementsArray_.clear();
  audioElementRepository_->getAll(allAudioElementsArray_);

  if (!mixPresentationRepository_->get(kmixPresID_).has_value()) {
    return;
  }

  MixPresentation mixPres =
      mixPresentationRepository_->get(kmixPresID_).value();

  mixpresentationAudioElements_ = mixPres.getAudioElements();
  audioElements_.clear();
  for (int i = 0; i < mixpresentationAudioElements_.size(); i++) {
    juce::Uuid audioElementId = mixpresentationAudioElements_[i].getId();
    audioElements_.push_back(getAudioElement(audioElementId));
  }
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "Initialized audio elements for PresentationTab. Total: " +
                    std::to_string(audioElements_.size()));
}

AudioElement PresentationTab::getAudioElement(const juce::Uuid& Id) {
  for (auto audioElement : allAudioElementsArray_) {
    if (audioElement->getId() == Id) {
      return *audioElement;
    }
  }
}

void PresentationTab::createAEStrips() {
  aeStrips_.clear();
  for (int i = 0; i < audioElements_.size(); i++) {
    AudioElement audioelement = audioElements_[i];
    int channel_count = audioelement.getChannelCount();
    int first_channel = audioelement.getFirstChannel();
    juce::String label_text = audioelement.getName();
    aeStrips_.add(std::make_unique<AEStripComponent>(
        channel_count, label_text, first_channel, repos_, channelMonitorData_,
        mixpresentationAudioElements_[i].getId(), kmixPresID_));
    addAndMakeVisible(aeStrips_.getLast());
  }

  // Log a summary after the loop
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "Created AEStrips for PresentationTab. ");
}

void PresentationTab::paintAEStrips() {
  for (auto aestrip : aeStrips_) {
    aestrip->repaint();
  }
}

int PresentationTab::getNumOfAEStrips() { return aeStrips_.size(); }

int PresentationTab::calculateHeight() {
  int numStrips = aeStrips_.size();
  int height = topClearance + bottomClearance + (numStrips * stripHeight) +
               ((numStrips - 1) * stripSpacing);

  return height;
}

void PresentationTab::updateStripLabel(const juce::Uuid& Id) {
  for (auto strip : aeStrips_) {
    if (strip->getAudioElementID() == Id) {
      AudioElement ae = audioElementRepository_->get(Id).value();
      strip->updateName(ae.getName());
    }
  }
}

void PresentationTab::updateStripChannelSet(const juce::Uuid& Id,
                                            const int& newStartingChannel) {
  for (auto strip : aeStrips_) {
    if (strip->getAudioElementID() == Id) {
      strip->channelsReroutedCallback(newStartingChannel);
    }
  }
}
