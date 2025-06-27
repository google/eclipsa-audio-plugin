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

#include "LoudnessMeter.h"

#include <processors/gain/MSProcessor.h>

MeterLookAndFeel::MeterLookAndFeel(MSPlaybackRepository& msPlaybackRepo)
    : msPlaybackRepo_(msPlaybackRepo) {
  // Text colours.
  setColour(juce::TextButton::textColourOffId, EclipsaColours::tabTextGrey);
  setColour(juce::TextButton::ColourIds::textColourOnId,
            EclipsaColours::tabTextGrey);

  // Label colours.
  setColour(juce::Label::textColourId, EclipsaColours::tabTextGrey);
  setColour(juce::Label::backgroundColourId, kBckGnd);

  // Button colours.
  setColour(juce::TextButton::ColourIds::buttonColourId, kBckGnd);
  setColour(juce::TextButton::ColourIds::buttonOnColourId,
            EclipsaColours::onButtonGrey);
}

void MeterLookAndFeel::drawButtonBackground(
    juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour, bool isMouseOverButton,
    bool isButtonDown) {
  PlaybackMS muteSoloState = msPlaybackRepo_.get();
  LoudnessMeter::MSButton* msButton =
      dynamic_cast<LoudnessMeter::MSButton*>(&button);

  auto buttonArea = button.getLocalBounds();
  buttonArea.reduce(2, 2);

  juce::Colour backColour;
  if (isMouseOverButton || button.getToggleState()) {
    backColour = findColour(juce::TextButton::buttonOnColourId);
  }
  // If there is Solo'ing active:
  // - If this is a Mute button and this channel is not Solo'd, fill with a
  //   darker background to indicate implicit muting.
  else if (muteSoloState.getSoloedChannels().any()) {
    if (msButton->type == LoudnessMeter::MSButton::kMute &&
        !muteSoloState.getSoloedChannels()[msButton->chIdx]) {
      backColour = EclipsaColours::semiOnButtonGrey;
    }
  } else {
    backColour = findColour(juce::TextButton::buttonColourId);
  }

  g.setColour(backColour);
  float cornerSize = buttonArea.getHeight() / 6.0f;
  g.fillRoundedRectangle(buttonArea.toFloat(), cornerSize);
  g.setColour(kTextClr);
  g.drawRoundedRectangle(buttonArea.toFloat(), cornerSize, 1.0f);
}

LoudnessMeter::LoudnessMeter(const juce::String chLabel, const int chIdx,
                             MSPlaybackRepository& msPlaybackRepo)
    : kChIdx_(chIdx),
      msPlaybackRepo_(msPlaybackRepo),
      lookAndFeel_(msPlaybackRepo_),
      chLabel_(chLabel, chLabel) {
  msPlaybackRepo_.registerListener(this);
  setLookAndFeel(&lookAndFeel_);

  addAndMakeVisible(loudnessBar_);

  chLabel_.setJustificationType(juce::Justification::centred);
  chLabel_.setMinimumHorizontalScale(0.2f);
  addAndMakeVisible(chLabel_);

  // Pull down mute/solo state from repository.
  PlaybackMS muteSoloState = msPlaybackRepo_.get();

  soloButton_.type = MSButton::kSolo;
  soloButton_.chIdx = kChIdx_;
  soloButton_.setButtonText("S");
  soloButton_.setToggleable(true);
  soloButton_.setClickingTogglesState(true);
  soloButton_.onClick = [this] { toggleSolo(); };
  soloButton_.setToggleState(muteSoloState.getSoloedChannels()[kChIdx_], false);
  addAndMakeVisible(soloButton_);

  muteButton_.type = MSButton::kMute;
  muteButton_.chIdx = kChIdx_;
  muteButton_.setButtonText("M");
  muteButton_.setToggleable(true);
  muteButton_.setClickingTogglesState(true);
  muteButton_.onClick = [this] { toggleMute(); };
  muteButton_.setToggleState(muteSoloState.getMutedChannels()[kChIdx_], false);
  addAndMakeVisible(muteButton_);
}

LoudnessMeter::~LoudnessMeter() {
  msPlaybackRepo_.deregisterListener(this);
  setLookAndFeel(nullptr);
}

// Paint child components of the loudness meter.
void LoudnessMeter::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();

  // 2/3 of the available space should be allocated to the level bar.
  loudnessBar_.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.66f));

  // Remaining space split between 2 buttons and a label with a small offset.
  chLabel_.setBounds(bounds.removeFromTop(bounds.getWidth() + kButtonOffset_));
  bounds.removeFromTop(kButtonOffset_);

  int buttonHeight =
      std::min((bounds.getHeight() - kButtonOffset_) / 2, bounds.getWidth());
  soloButton_.setBounds(bounds.removeFromTop(buttonHeight));
  bounds.removeFromTop(kButtonOffset_);
  muteButton_.setBounds(bounds.removeFromTop(buttonHeight));
}

void LoudnessMeter::setloudness(const float loudness) {
  loudnessBar_.setLoudness(loudness);
}

void LoudnessMeter::resetSoloMute() {
  soloButton_.setToggleState(false, false);
  muteButton_.setToggleState(false, false);
  loudnessBar_.resetResidualPeak();
}

void LoudnessMeter::toggleSolo() {
  PlaybackMS muteSoloState = msPlaybackRepo_.get();
  muteSoloState.toggleSolo(kChIdx_);
  msPlaybackRepo_.update(muteSoloState);
  loudnessBar_.resetResidualPeak();
}

void LoudnessMeter::toggleMute() {
  PlaybackMS muteSoloState = msPlaybackRepo_.get();
  muteSoloState.toggleMute(kChIdx_);
  msPlaybackRepo_.update(muteSoloState);
  loudnessBar_.resetResidualPeak();
}

HeadphonesLoudnessMeter::HeadphonesLoudnessMeter() {
  addAndMakeVisible(leftBar_);
  addAndMakeVisible(rightBar_);
  headphonesImg_.setImage(img_);
  addAndMakeVisible(headphonesImg_);
}

void HeadphonesLoudnessMeter::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds().withTrimmedLeft(kBarOffset_);

  // 2/3 of the available vertical space should be allocated to the level bar.
  auto barBounds = bounds.removeFromTop(bounds.getHeight() * 0.66f);
  auto imgBounds = bounds;

  leftBar_.setBounds(barBounds.removeFromLeft(barWidth_));
  bounds.removeFromLeft(kBarOffset_);
  rightBar_.setBounds(barBounds.removeFromLeft(barWidth_));

  // Compute image bounds and draw.
  imgBounds.removeFromTop(6);
  imgBounds.removeFromBottom(bounds.getHeight() / 1.3f);
  imgBounds.removeFromRight(kBarOffset_);
  headphonesImg_.setBounds(imgBounds);
}

juce::Rectangle<int> LoudnessMeter::getSMButtonsBounds() const {
  // Get the bounds of the solo and mute buttons.
  const auto soloBounds = soloButton_.getBounds();
  const auto muteBounds = muteButton_.getBounds();
  return soloBounds.getUnion(muteBounds);
}

void LoudnessMeter::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  soloButton_.repaint();
  muteButton_.repaint();
}