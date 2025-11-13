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

#include "ChannelMonitorProcessor.h"

#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_repository/implementation/MixPresentationSoloMuteRepository.h"
#include "data_structures/src/ChannelMonitorData.h"
#include "data_structures/src/MixPresentation.h"

ChannelMonitorProcessor::ChannelMonitorProcessor(
    ChannelMonitorData& channelMonitorData,
    MixPresentationRepository* mixPresentationRepository,
    MixPresentationSoloMuteRepository* mixPresentationSoloMuteRepository)
    : numChannels_(juce::AudioChannelSet::ambisonic(5).size()),
      channelMonitorData_(channelMonitorData),
      loudness_(std::vector<float>(numChannels_, -300.f)),
      mixPresentationRepository_(mixPresentationRepository),
      mixPresentationSoloMuteRepository_(mixPresentationSoloMuteRepository) {
  channelMonitorData_.reinitializeLoudnesses(numChannels_);
  mixPresentationRepository_->registerListener(this);
}

ChannelMonitorProcessor::~ChannelMonitorProcessor() {
  mixPresentationRepository_->deregisterListener(this);
}

const juce::String ChannelMonitorProcessor::getName() const {
  return {"Channel Monitor"};
}

void ChannelMonitorProcessor::prepareToPlay(double sampleRate,
                                            int samplesPerBlock) {}

void ChannelMonitorProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;

  for (int i = 0; i < buffer.getNumChannels(); i++) {
    loudness_[i] =
        20.0f * std::log10(buffer.getRMSLevel(i, 0, buffer.getNumSamples()));
  }

  for (int i = buffer.getNumChannels(); i < numChannels_; i++) {
    loudness_[i] = -120.0f;
  }

  channelMonitorData_.channelLoudnesses.update(loudness_);
}

bool ChannelMonitorProcessor::hasEditor() const {
  return false;  // (change this to false if you choose to not supply an editor)
}

void ChannelMonitorProcessor::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  // if a mix presentation is added, add it to the mpSM repository
  if (childWhichHasBeenAdded.getType() == MixPresentation::kTreeType) {
    const juce::var mixPresentationSoloMuteId =
        childWhichHasBeenAdded[MixPresentationSoloMute::kId];
    MixPresentationSoloMute mixPresentationSoloMute(
        juce::Uuid(mixPresentationSoloMuteId.toString()));

    mixPresentationSoloMuteRepository_->updateOrAdd(mixPresentationSoloMute);
  }
  // if an audio element is added to a mix presentation, add it to the
  // mpSM repository
  else if (parentTree.getType() == MixPresentation::kTreeType &&
           childWhichHasBeenAdded.getType() ==
               MixPresentation::kAudioElements) {
    juce::Uuid mixPresID = juce::Uuid(parentTree[MixPresentation::kId]);

    // add Audio Element to SoloMute Repository
    MixPresentationSoloMute mixPresSoloMute =
        mixPresentationSoloMuteRepository_->get(mixPresID).value_or(
            MixPresentationSoloMute());

    for (auto audioElementNode : childWhichHasBeenAdded) {
      mixPresSoloMute.addAudioElement(
          juce::Uuid(audioElementNode[MixPresentationAudioElement::kId]),
          audioElementNode[MixPresentationAudioElement::kReferenceId],
          audioElementNode[MixPresentation::kPresentationName]);
    }

    mixPresentationSoloMuteRepository_->update(mixPresSoloMute);
  }
}

void ChannelMonitorProcessor::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  // if a mix presentation is removed, remove it from the mpSM repository
  if (childWhichHasBeenRemoved.getType() == MixPresentation::kTreeType) {
    const juce::var mixPresentationSoloMuteId =
        childWhichHasBeenRemoved[MixPresentationSoloMute::kId];
    // if a mix presentation is removed, remove it from the mpSM repository
    // this includes removing the audio elements
    MixPresentationSoloMute mixPresentationSoloMute(
        juce::Uuid(mixPresentationSoloMuteId.toString()));

    mixPresentationSoloMuteRepository_->remove(mixPresentationSoloMute);
  } else if (parentTree.getType() == MixPresentation::kTreeType &&
             childWhichHasBeenRemoved.getType() ==
                 MixPresentation::kAudioElements) {
    juce::Uuid mixPresID = juce::Uuid(parentTree[MixPresentation::kId]);

    // remove Audio Element from SoloMute Repository
    MixPresentationSoloMute mixPresSoloMute =
        mixPresentationSoloMuteRepository_->get(mixPresID).value_or(
            MixPresentationSoloMute());

    for (auto audioElementNode : childWhichHasBeenRemoved) {
      mixPresSoloMute.removeAudioElement(
          juce::Uuid(audioElementNode[MixPresentationAudioElement::kId]));
    }

    mixPresentationSoloMuteRepository_->update(mixPresSoloMute);
  }
}