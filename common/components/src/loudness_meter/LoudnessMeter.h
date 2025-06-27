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
#include "LoudnessLevelBar.h"
#include "components/src/EclipsaColours.h"
#include "data_structures/src/RepositoryCollection.h"
#include "data_structures/src/SpeakerMonitorData.h"

class MeterLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  MeterLookAndFeel(MSPlaybackRepository& msPlaybackRepo_);

  void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool isMouseOverButton, bool isButtonDown) override;

  const juce::Colour kTextClr = EclipsaColours::buttonMSTextColour;
  const juce::Colour kButtonOn = EclipsaColours::onButtonGrey;
  const juce::Colour kBckGnd = EclipsaColours::backgroundOffBlack;
  MSPlaybackRepository& msPlaybackRepo_;
};

class LoudnessMeter : public juce::Component, juce::ValueTree::Listener {
 public:
  struct MSButton : public juce::TextButton {
    enum ButtonType { kSolo, kMute };
    ButtonType type;
    int chIdx;
  };

  LoudnessMeter(const juce::String chLabel, const int chIdx,
                MSPlaybackRepository& msPlaybackRepo);

  ~LoudnessMeter();

  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;

  void paint(juce::Graphics& g) override;

  void setloudness(const float loudness);
  void resetSoloMute();
  void toggleSolo();
  void toggleMute();
  juce::Rectangle<int> getSMButtonsBounds() const;

 private:
  const int kButtonOffset_ = 2;
  const int kChIdx_;

  MSPlaybackRepository& msPlaybackRepo_;
  MeterLookAndFeel lookAndFeel_;
  LoudnessLevelBar loudnessBar_;
  juce::Label chLabel_;
  MSButton soloButton_;
  MSButton muteButton_;
};

class HeadphonesLoudnessMeter : public juce::Component {
 public:
  HeadphonesLoudnessMeter();

  void paint(juce::Graphics& g) override;

  void setBarWidth(const int barWidth) { barWidth_ = barWidth; }
  void setLoudness(const float leftLoudness, const float rightLoudness) {
    leftBar_.setLoudness(leftLoudness);
    rightBar_.setLoudness(rightLoudness);
  }

 private:
  const int kBarOffset_ = 4;
  const juce::Image img_ = IconStore::getInstance().getHeadphonesIcon();

  int barWidth_ = 0;
  LoudnessLevelBar leftBar_, rightBar_;
  juce::ImageComponent headphonesImg_;
};