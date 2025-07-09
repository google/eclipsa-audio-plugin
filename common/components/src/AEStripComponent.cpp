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

#include "AEStripComponent.h"

#include <vector>

#include "../rendererplugin/src/RendererProcessor.h"
#include "components/src/EclipsaColours.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/ChannelMonitorData.h"
#include "data_structures/src/MixPresentation.h"
#include "data_structures/src/MixPresentationSoloMute.h"
#include "data_structures/src/RepositoryCollection.h"
#include "logger/logger.h"

AEStripLookandFeel::AEStripLookandFeel() {
  setColour(juce::TextButton::buttonOnColourId, EclipsaColours::onButtonGrey);
  setColour(juce::TextButton::buttonColourId,
            EclipsaColours::backgroundOffBlack);
  setColour(juce::TextButton::textColourOnId, EclipsaColours::tabTextGrey);
  setColour(juce::TextButton::textColourOffId, EclipsaColours::tabTextGrey);

  setColour(juce::Label::backgroundColourId, juce::Colours::transparentWhite);
  setColour(juce::Label::textColourId, EclipsaColours::headingGrey);
}

AEStripComponent::AEStripComponent(
    const int channelCount, const juce::String label, const int startingChannel,
    RepositoryCollection& repos, ChannelMonitorData& channelMonitorData,
    const juce::Uuid audioelementID, const juce::Uuid mixPresID)
    : soloButtonClicked([this]() { soloButtonClickedCallback(); }),
      muteButtonClicked([this] { muteButtonClickedCallback(); }),
      label_(label),
      channelCount_(channelCount),
      channelsSet_(createChannelSet(
          channelCount_,
          startingChannel)),  // can try to remove the multistep functions
      multichannelGainRepo_(&repos.chGainRepo_),
      channelMonitorData_(channelMonitorData),
      audioelementID_(audioelementID),
      mixPresID_(mixPresID),
      mixPresentationRepository_(&repos.mpRepo_),
      mixPresentationSoloMuteRepository_(&repos.mpSMRepo_) {
  mixPresentationSoloMuteRepository_->registerListener(this);
  startTimerHz(30);
  setLookAndFeel(&lookAndFeel_);

  // the main label text is a slightly different white
  mainLabel.setText(label_, juce::dontSendNotification);

  mainLabel.setJustificationType(juce::Justification::topLeft);
  addAndMakeVisible(mainLabel);

  MixPresentationSoloMute mixPresSoloMute =
      mixPresentationSoloMuteRepository_->get(mixPresID_).value();

  bool initialSoloState = mixPresSoloMute.isAudioElementSoloed(audioelementID_);
  bool initialMuteState = mixPresSoloMute.isAudioElementMuted(audioelementID_);

  // Setup the solo and mute buttons
  setupToggleButton("S", soloButton_, soloButtonClicked, initialSoloState);
  setupToggleButton("M", muteButton_, muteButtonClicked, initialMuteState);
  determineSoloMuteButtonColours();

  addAndMakeVisible(indicatorContainer);
  assignChannelLabels();
  updateChannelMutes();
}

AEStripComponent::~AEStripComponent() {
  setLookAndFeel(nullptr);
  mixPresentationSoloMuteRepository_->deregisterListener(this);
}

void AEStripComponent::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();
  const float buttonDimReduction = 0.05f;
  // Set the main label bounds
  // Allocate the top 20% for the main label
  auto mainLabelBounds = bounds.removeFromTop(bounds.getHeight() * 0.2f);
  mainLabel.setBounds(mainLabelBounds);
  mainLabel.setText(truncateLabel(label_), juce::dontSendNotification);

  // Calculate the fractional width for the buttons
  float fraction = 2.0f / (channelCount_ + 2.5f);
  // Split the bounds for the buttons and the channel labels
  auto buttonBounds = bounds.removeFromLeft(bounds.getWidth() * fraction);
  buttonBounds.translate(buttonBounds.getWidth() * 0.15f, 0);

  buttonBounds.reduce(0, buttonBounds.getHeight() * 0.05f);
  // Set the bounds of the solo button to the top 50% of the button bounds
  auto soloButton_Bounds =
      buttonBounds.removeFromTop(buttonBounds.getHeight() * 0.5f);

  soloButton_Bounds.removeFromBottom(soloButton_Bounds.getHeight() * 0.1f);
  soloButton_Bounds = setBoundsAspectRatio(soloButton_Bounds, 1.f);
  soloButton_Bounds.reduce(soloButton_Bounds.getWidth() * buttonDimReduction,
                           soloButton_Bounds.getHeight() * buttonDimReduction);
  soloButton_.setBounds(soloButton_Bounds);

  buttonBounds.removeFromTop(buttonBounds.getHeight() * 0.1f);
  // Set the bounds of the mute button
  auto muteButton_Bounds = setBoundsAspectRatio(buttonBounds, 1.f);
  muteButton_Bounds.reduce(muteButton_Bounds.getWidth() * buttonDimReduction,
                           muteButton_Bounds.getHeight() * buttonDimReduction);
  muteButton_.setBounds(muteButton_Bounds);

  indicatorContainer.setBounds(bounds);

  auto channelLabelsBounds = bounds;

  // fractional width of each channel indicator and label
  //  the additional 0.5f is for the left and right edge margins of the
  //  container
  fraction = 1.f / (channelCount_ + .5f);
  // ensure the channel
  channelLabelsBounds.removeFromLeft(bounds.getWidth() * fraction * .25f);
  // Set the bounds of the channel labels and indicators
  // Fix the aspect ratios here
  for (int i = 0; i < channelLabels.size(); i++) {
    auto channelIndicator = channelIndicators[i];
    auto channelLabel = channelLabels[i];
    // get the bounds for the indicator and the channel label
    auto column_bounds =
        channelLabelsBounds.removeFromLeft(bounds.getWidth() * fraction);

    // get top half for the indicator and bottom half for the channel label
    auto indicator_bounds =
        column_bounds.removeFromTop(column_bounds.getHeight() * 0.5f);
    indicator_bounds.translate(0, indicator_bounds.getHeight() * 0.35f);
    indicator_bounds = setLightsSpacing(indicator_bounds, 3);
    auto lightBounds = setBoundsAspectRatio(indicator_bounds, 1.f);
    channelIndicator->setBounds(lightBounds);
    column_bounds.translate(0, -column_bounds.getHeight() * 0.35f);
    channelLabel->setBounds(column_bounds);
  }
}

void AEStripComponent::setupToggleButton(const juce::String& text,
                                         juce::TextButton& button,
                                         const std::function<void()>& callback,
                                         bool initialState) {
  button.setButtonText(text);
  button.setToggleable(true);
  button.setToggleState(initialState, juce::dontSendNotification);
  button.setClickingTogglesState(true);
  button.onClick = callback;
  addAndMakeVisible(button);
  LOG_ANALYTICS(0,
                "Toggle button " + text.toStdString() +
                    " created. Initial state: " + std::to_string(initialState));
}

// Function to store the channel indices of the channels used in the AE
std::set<int> AEStripComponent::createChannelSet(const int& numChannels,
                                                 const int& startingChannel) {
  std::set<int> channels;
  for (int i = startingChannel; i < startingChannel + numChannels; i++) {
    channels.insert(i);
  }
  return channels;
}

void AEStripComponent::soloButtonClickedCallback() {  // handle case that solo
  // button is toggled on
  MixPresentationSoloMute mixPresSoloMute;
  std::optional<MixPresentationSoloMute> mixPresSoloMuteOpt =
      mixPresentationSoloMuteRepository_->get(mixPresID_);
  if (mixPresSoloMuteOpt.has_value()) {
    LOG_ANALYTICS(0, "SoloMute Repo State before solo update: " +
                         mixPresentationSoloMuteRepository_->getValueTree()
                             .toXmlString()
                             .toStdString());
    mixPresSoloMute = mixPresSoloMuteOpt.value();
    mixPresSoloMute.setAudioElementSolo(audioelementID_,
                                        soloButton_.getToggleState());
    mixPresentationSoloMuteRepository_->update(mixPresSoloMute);
    LOG_ANALYTICS(0, "SoloMute Repo State after solo update: " +
                         mixPresentationSoloMuteRepository_->getValueTree()
                             .toXmlString()
                             .toStdString());
    // updateChannelMutes();
    // determineSoloMuteButtonColours();
    // muteButton_.repaint();
    // soloButton_.repaint();
  } else {
    LOG_ERROR(RendererProcessor::instanceId_,
              "AEChannelStrip:: Could not find mix presentation w/ ID: " +
                  mixPresID_.toString().toStdString());
  }
}

void AEStripComponent::muteButtonClickedCallback() {
  MixPresentationSoloMute mixPresSoloMute;
  std::optional<MixPresentationSoloMute> mixPresSoloMuteOpt =
      mixPresentationSoloMuteRepository_->get(mixPresID_);
  if (mixPresSoloMuteOpt.has_value()) {
    mixPresSoloMute = mixPresSoloMuteOpt.value();
    mixPresSoloMute.setAudioElementMute(audioelementID_,
                                        muteButton_.getToggleState());
    mixPresentationSoloMuteRepository_->update(mixPresSoloMute);
    // updateChannelMutes();
    // determineSoloMuteButtonColours();
    // muteButton_.repaint();
    // soloButton_.repaint();
  } else {
    LOG_ERROR(RendererProcessor::instanceId_,
              "AEChannelStrip:: Could not find mix presentation w/ ID: " +
                  mixPresID_.toString().toStdString());
  }
}

juce::Rectangle<int> AEStripComponent::setBoundsAspectRatio(
    const juce::Rectangle<int>& bounds, const float& aspectRatio) {
  auto newBounds = bounds;
  if (bounds.getWidth() < bounds.getHeight()) {
    newBounds.setHeight(bounds.getWidth());
  } else {
    newBounds.setWidth(bounds.getHeight() * aspectRatio);
  }
  return newBounds;
}

juce::Rectangle<int> AEStripComponent::setLightsSpacing(
    juce::Rectangle<int> bounds, const int& spacing) {
  bounds.removeFromRight(spacing);
  bounds.removeFromLeft(spacing);

  return bounds;
}

void AEStripComponent::timerCallback() {
  size_t i = 0;
  channelMonitorData_.channelLoudnesses.read(channelLoudnessesRead_);
  for (auto channelIndex : channelsSet_) {
    channelIndicators[i]->setColour(determineColourIndex(channelIndex));
    channelIndicators[i]->repaint();
    ++i;
  }
}

int AEStripComponent::determineColourIndex(const int& channelIndex) {
  // get the linear RMS
  float loudnessdB = channelLoudnessesRead_[channelIndex];
  // convert to dB
  // determine the colour index based on the loudness
  if (loudnessdB < -60.f) {
    return 0;  // inactive
  } else if (loudnessdB >= -60.f && loudnessdB < -20.f) {
    return 4;  // green
  } else if (loudnessdB >= -20.f && loudnessdB < -6.f) {
    return 3;  // yellow
  } else if (loudnessdB >= -6.f && loudnessdB < 0.f) {
    return 2;  // orange
  } else {
    return 1;  // red
  }
}

void AEStripComponent::toggleChannelMute(const int& channel) {
  ChannelGains channelGains = multichannelGainRepo_->get();
  channelGains.toggleChannelMute(channel);
  multichannelGainRepo_->update(channelGains);
}

void AEStripComponent::muteAEChannels() {
  // get the already muted channels
  const std::unordered_map<int, float> muted_channels =
      multichannelGainRepo_->get().getMutedChannels();
  for (auto channel : channelsSet_) {
    // if the channel is not muted, mute it
    if (muted_channels.find(channel) == muted_channels.end()) {
      toggleChannelMute(channel);
    }
  }
}

void AEStripComponent::unmuteAEChannels() {
  // get the already muted channels
  const std::unordered_map<int, float> muted_channels =
      multichannelGainRepo_->get().getMutedChannels();
  for (auto channel : channelsSet_) {
    // if the channel is not muted, mute it
    if (muted_channels.find(channel) != muted_channels.end()) {
      toggleChannelMute(channel);
    }
  }
}

void AEStripComponent::updateChannelMutes() {
  MixPresentationSoloMute mixPresSoloMute;
  std::optional<MixPresentationSoloMute> mixPresSoloMuteOpt =
      mixPresentationSoloMuteRepository_->get(mixPresID_);
  if (mixPresSoloMuteOpt.has_value()) {
    mixPresSoloMute = mixPresSoloMuteOpt.value();
  } else {
    LOG_ERROR(RendererProcessor::instanceId_,
              "AEChannelStrip:: Could not find mix presentation w/ ID: " +
                  mixPresID_.toString().toStdString());
    return;
  }

  // handle the case where the audio element is muted
  // the mute button overrides the solo button
  // if a different AE is soloed, and this is not soloed, then it should be
  // muted
  if (mixPresSoloMute.isAudioElementMuted(audioelementID_) ||
      (mixPresSoloMute.getAnySoloed() &&
       !mixPresSoloMute.isAudioElementSoloed(audioelementID_))) {
    muteAEChannels();
    LOG_ANALYTICS(0, "called muteAEChannels()");
  } else {
    unmuteAEChannels();
    LOG_ANALYTICS(0, "called unmuteAEChannels()");
  }
  LOG_ANALYTICS(
      0, "updated Channel mutes for audio element: " +
             audioelementID_.toString().toStdString() +
             " in mix presentation: " + mixPresID_.toString().toStdString());
  LOG_ANALYTICS(0,
                multichannelGainRepo_->getTree().toXmlString().toStdString());
}

void AEStripComponent::determineSoloMuteButtonColours() {
  MixPresentationSoloMute mixPresSoloMute;

  std::optional<MixPresentationSoloMute> mixPresSoloMuteOpt =
      mixPresentationSoloMuteRepository_->get(mixPresID_);
  if (mixPresSoloMuteOpt.has_value()) {
    mixPresSoloMute = mixPresSoloMuteOpt.value();
  } else {
    LOG_ERROR(RendererProcessor::instanceId_,
              "AEChannelStrip:: Could not find mix presentation w/ ID: " +
                  mixPresID_.toString().toStdString());
    return;
  }

  // Colour mute button based on toggle state
  if (mixPresSoloMute.isAudioElementMuted(audioelementID_)) {
    muteButton_.setColour(juce::TextButton::buttonColourId,
                          EclipsaColours::onButtonGrey);
  } else {  //  handle cases where the audio element is not muted
    // if a different AE soloed, then the current AE should indicate a faint
    // mute
    if (mixPresSoloMute.getAnySoloed() &&
        !mixPresSoloMute.isAudioElementSoloed(audioelementID_)) {
      muteButton_.setColour(juce::TextButton::buttonColourId,
                            EclipsaColours::semiOnButtonGrey);
    } else if (!mixPresSoloMute.getAnySoloed() ||
               mixPresSoloMute.isAudioElementSoloed(audioelementID_)) {
      muteButton_.setColour(juce::TextButton::buttonColourId,
                            EclipsaColours::backgroundOffBlack);
    }
  }

  // Colour solo button based on toggle state
  if (mixPresSoloMute.isAudioElementSoloed(audioelementID_)) {
    soloButton_.setColour(juce::TextButton::buttonColourId,
                          EclipsaColours::onButtonGrey);
  } else {
    soloButton_.setColour(juce::TextButton::buttonColourId,
                          EclipsaColours::backgroundOffBlack);
  }
}

void AEStripComponent::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  juce::Uuid parentID = juce::Uuid(parentTree[MixPresentationSoloMute::kId]);
  juce::Uuid childID =
      juce::Uuid(childWhichHasBeenAdded[MixPresentationSoloMute::kId]);
  if (parentID != mixPresID_) {
    return;
  }
  LOG_ANALYTICS(0, "valueTreeChildAdded called for mixID: " +
                       mixPresID_.toString().toStdString() +
                       " and audioelementID: " +
                       audioelementID_.toString().toStdString());
  LOG_ANALYTICS(0, " Parent Tree: \n" + parentTree.toXmlString().toStdString());
  LOG_ANALYTICS(0, " Child Tree: \n" +
                       childWhichHasBeenAdded.toXmlString().toStdString());
  if (parentTree.getType() == MixPresentationSoloMute::kTreeType) {
    updateChannelMutes();
    determineSoloMuteButtonColours();
    muteButton_.repaint();
    soloButton_.repaint();
  }
}

void AEStripComponent::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int index) {
  // LOG_ANALYTICS(0, "valueTreeChildRemoved called for mixID: " +
  //                      mixPresID_.toString().toStdString() +
  //                      " and audioelementID: " +
  //                      audioelementID_.toString().toStdString());
  // LOG_ANALYTICS(0, " Parent Tree: \n" +
  // parentTree.toXmlString().toStdString()); LOG_ANALYTICS(0, " Child Tree: \n"
  // +
  //                      childWhichHasBeenRemoved.toXmlString().toStdString());
  // if (parentTree.getType() == MixPresentationSoloMute::kTreeType) {
  //   updateChannelMutes();
  //   determineSoloMuteButtonColours();
  //   muteButton_.repaint();
  //   soloButton_.repaint();
  // }
}

void AEStripComponent::updateName(const juce::String& name) {
  label_ = name;
  mainLabel.setText(label_, juce::dontSendNotification);
  mainLabel.repaint();
}

void AEStripComponent::channelsReroutedCallback(const int& newStartingChannel) {
  // update the channel set
  channelsSet_ = createChannelSet(channelCount_, newStartingChannel);
  // update the channel labels
  assignChannelLabels();
  updateChannelMutes();
}

void AEStripComponent::assignChannelLabels() {
  channelLabels.clear();
  // Add the channel indicators and labels
  for (auto channel : channelsSet_) {
    auto channelLabel = std::make_unique<juce::Label>();
    channelLabel->setColour(juce::Label::backgroundColourId,
                            juce::Colours::transparentWhite);

    channelLabel->setText(juce::String(channel + 1),
                          juce::dontSendNotification);
    channelLabel->setJustificationType(juce::Justification::centredBottom);
    channelLabels.add(std::move(channelLabel));
    addAndMakeVisible(channelLabels.getLast());

    auto channelIndicator =
        std::make_unique<ColouredLight>(EclipsaColours::inactiveGrey);  // 0
    channelIndicator->addColour(EclipsaColours::red);                   // 1
    channelIndicator->addColour(EclipsaColours::orange);                // 2
    channelIndicator->addColour(EclipsaColours::yellow);                // 3
    channelIndicator->addColour(EclipsaColours::green);                 // 4
    channelIndicator->setColour(0);

    channelIndicators.add(std::move(channelIndicator));
    addAndMakeVisible(channelIndicators.getLast());
  }
}

juce::String AEStripComponent::truncateLabel(const juce::String& label) {
  juce::Font font = mainLabel.getFont();
  int labelWidth = font.getStringWidth(label);
  juce::String truncatedLabel = label;
  if (labelWidth > mainLabel.getWidth()) {
    // Truncate the label to fit within the label width
    int endIndex =
        std::floor(static_cast<float>(mainLabel.getWidth()) /
                   static_cast<float>(labelWidth) * (label.length() - 1)) -
        3;  // remove an additional 3 characters for the '...'
    truncatedLabel = label.substring(0, endIndex);
    truncatedLabel += "...";
  }
  return truncatedLabel;
}
