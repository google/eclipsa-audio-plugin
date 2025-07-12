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
#include <unordered_map>

#include "../rendererplugin/src/RendererProcessor.h"
#include "ColouredLight.h"
#include "RoundedRectangle.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_repository/implementation/MixPresentationSoloMuteRepository.h"
#include "data_repository/implementation/MultiChannelGainRepository.h"
#include "data_structures/src/ActiveMixPresentation.h"
#include "data_structures/src/ChannelMonitorData.h"
#include "data_structures/src/RepositoryCollection.h"

// Use this to specify how the S/M buttons should look
class AEStripLookandFeel : public juce::LookAndFeel_V4 {
 public:
  AEStripLookandFeel();
};

class AEStripComponent : public juce::Component,
                         public juce::Timer,
                         public juce::ValueTree::Listener {
 public:
  AEStripComponent(const int countChannel, const juce::String label,
                   const int startingChannel, RepositoryCollection& repos,
                   ChannelMonitorData& channelMonitorData,
                   const juce::Uuid audioelementID, const juce::Uuid mixPresID);
  ~AEStripComponent();

  int getChannelCount() const { return channelCount_; }

  void timerCallback() override;

  void updateOnActiveMixPresentationChange() { updateChannelMutes(); }

  void updateName(const juce::String& name);

  void channelsReroutedCallback(const int& newStartingChannel);

  juce::Uuid getAudioElementID() { return audioelementID_; }

  std::set<int> getChannelSet() { return channelsSet_; }

  juce::Label mainLabel;
  RoundedRectangle indicatorContainer;
  juce::OwnedArray<ColouredLight> channelIndicators;
  juce::OwnedArray<juce::Label> channelLabels;
  juce::OwnedArray<juce::Colour> loudnessColours;

 private:
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;

  void determineSoloMuteButtonColours();

  void assignChannelLabels();

  void paint(juce::Graphics& g) override;

  int determineColourIndex(const int& channelIndex);

  void setupToggleButton(const juce::String& text, juce::TextButton& button,
                         const std::function<void()>& callback,
                         bool initialState = false);

  // these functions are executed when internal S/M buttons are toggled
  void soloButtonClickedCallback();
  void muteButtonClickedCallback();

  void toggleChannelMute(const int& channel);

  void updateChannelMutes();

  void muteAEChannels();

  void unmuteAEChannels();

  // Preserve the centre of the bounds, but adjust the aspect ratio
  juce::Rectangle<int> setBoundsAspectRatio(const juce::Rectangle<int>& bounds,
                                            const float& aspectRatio);

  // Increase spacing between colored lights
  juce::Rectangle<int> setLightsSpacing(juce::Rectangle<int> bounds,
                                        const int& spacing);

  std::set<int> createChannelSet(const int& numChannels,
                                 const int& startingChannel);

  juce::String truncateLabel(
      const juce::String&
          label);  // Truncate the label to fit within the bounds

  const juce::Uuid audioelementID_;
  const juce::Uuid mixPresID_;

  std::function<void()> soloButtonClicked;
  std::function<void()> muteButtonClicked;

  int channelCount_;

  juce::TextButton soloButton_;
  juce::TextButton muteButton_;

  juce::String label_;
  // this holds the indices of the channels (0-15, 0-7, etc..)
  std::set<int> channelsSet_;

  std::vector<float> channelLoudnessesRead_;
  ChannelMonitorData& channelMonitorData_;
  MultiChannelRepository* multichannelGainRepo_;
  MixPresentationRepository* mixPresentationRepository_;
  MixPresentationSoloMuteRepository* mixPresentationSoloMuteRepository_;
  ActiveMixRepository* activeMixRepository_;

  AEStripLookandFeel lookAndFeel_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AEStripComponent)
};