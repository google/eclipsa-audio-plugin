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

#include "LoudnessStats.h"

LoudnessStats::LoudnessStats(SpeakerMonitorData& data)
    : statsToDisp_("Loudness Standard"), rtData_(data) {
  statsToDisp_.setText("ITU-R BS.1770-4");
  statsToDisp_.reduceTitleBuffer(kLabelOffset_);
  addAndMakeVisible(statsToDisp_);

  resetButton_.setImages(false, true, true, kResetImg_, 1.0f,
                         EclipsaColours::tabTextGrey, kResetImg_, 1.0f,
                         EclipsaColours::tabTextGrey, kResetImg_, 1.0f,
                         EclipsaColours::tabTextGrey);
  resetButton_.addListener(this);
  addAndMakeVisible(resetButton_);

  momentary_.first.setText("Momentary", juce::dontSendNotification);
  configureLabels(momentary_);

  shortTerm_.first.setText("Short Term", juce::dontSendNotification);
  configureLabels(shortTerm_);

  integrated_.first.setText("Integrated", juce::dontSendNotification);
  configureLabels(integrated_);

  peak_.first.setText("True Peak", juce::dontSendNotification);
  configureLabels(peak_);

  range_.first.setText("Range", juce::dontSendNotification);
  configureLabels(range_);

  target_.first.setText("Target-YouTube", juce::dontSendNotification);
  configureLabels(target_);
  target_.second.setText("-14.0", juce::dontSendNotification);

  startTimerHz(1);
};

void LoudnessStats::configureLabels(LabelWithVal& label) {
  juce::BorderSize<int> border(1, 0, 1, 0);
  label.first.setBorderSize(border);
  label.second.setBorderSize(border);
  label.first.setMinimumHorizontalScale(0.1f);
  label.second.setMinimumHorizontalScale(0.1f);
  label.first.setJustificationType(juce::Justification::left);
  label.second.setJustificationType(juce::Justification::right);
  label.first.setColour(juce::Label::textColourId, kTextClr_);
  label.second.setColour(juce::Label::textColourId, kTextClr_);
  label.second.setText("--", juce::dontSendNotification);
  addAndMakeVisible(label.first);
  addAndMakeVisible(label.second);
}

void LoudnessStats::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();
  auto topBounds = bounds.removeFromTop(bounds.getHeight() * 0.3f);
  bounds.removeFromTop(5);
  auto botBounds = bounds;

  // Adjust the Y position to move the loudnessLabel upwards.
  int offsetY =
      -10;  // You can adjust this value to move it as per your requirement.
  auto loudnessLabelBounds =
      topBounds;  // Create a copy of topBounds for the loudnessLabel.
  loudnessLabelBounds.setY(loudnessLabelBounds.getY() +
                           offsetY);  // Move the loudnessLabelBounds upward.

  // Draw loudnessLabel.
  statsToDisp_.setBounds(
      loudnessLabelBounds.removeFromLeft(134).withHeight(72));

  // Draw measurement labels.
  botBounds.translate(0, offsetY * 3);
  botBounds.removeFromTop(kLabelOffset_);
  auto botLeftBounds = botBounds.removeFromLeft(botBounds.getWidth() * 0.75f);

  auto botRightBounds = botBounds;
  int labelHeight = (botLeftBounds.getHeight() / 2) / kNumMeasurements_ - 3;
  momentary_.first.setBounds(botLeftBounds.removeFromTop(labelHeight));
  botLeftBounds.removeFromTop(kLabelOffset_);

  shortTerm_.first.setBounds(botLeftBounds.removeFromTop(labelHeight));
  botLeftBounds.removeFromTop(kLabelOffset_);

  integrated_.first.setBounds(botLeftBounds.removeFromTop(labelHeight));
  botLeftBounds.removeFromTop(kLabelOffset_);

  peak_.first.setBounds(botLeftBounds.removeFromTop(labelHeight));
  botLeftBounds.removeFromTop(kLabelOffset_);

  range_.first.setBounds(botLeftBounds.removeFromTop(labelHeight));
  botLeftBounds.removeFromTop(kLabelOffset_);

  target_.first.setBounds(botLeftBounds.removeFromTop(labelHeight));
  botLeftBounds.removeFromTop(kLabelOffset_);

  // Update measurement values and draw.
  drawStatValues(labelHeight, botRightBounds);

  // Draw reset button.
  auto resetButtonBounds = getLocalBounds();
  resetButtonBounds.removeFromLeft(resetButtonBounds.getWidth() * 0.7f);
  resetButtonBounds.setTop(range_.first.getBottom() + kLabelOffset_);
  resetButtonBounds.translate(15, 0);
  resetButtonBounds.setHeight(loudnessLabelBounds.getHeight() * 0.8f);

  // Apply the new bounds to the reset button.
  resetButton_.setBounds(resetButtonBounds);
}

void LoudnessStats::drawStatValues(const int labelHeight,
                                   juce::Rectangle<int>& bounds) {
  MeasureEBU128::LoudnessStats stats{};
  rtData_.loudnessEBU128.read(stats);
  // Check values are valid and within a logical range.
  const auto valid = [](const float x) {
    if (isinf(x) || isnan(x)) {
      return false;
    }
    if (x < -100.f || x > 60.f) {
      return false;
    }
    return true;
  };

  juce::String measuredVal =
      valid(stats.loudnessMomentary)
          ? juce::String(stats.loudnessMomentary, kDecimalPlaces_)
          : kInvalid_;
  momentary_.second.setText(measuredVal, juce::dontSendNotification);
  momentary_.second.setBounds(bounds.removeFromTop(labelHeight));
  bounds.removeFromTop(kLabelOffset_);

  measuredVal = valid(stats.loudnessShortTerm)
                    ? juce::String(stats.loudnessShortTerm, kDecimalPlaces_)
                    : kInvalid_;
  shortTerm_.second.setText(measuredVal, juce::dontSendNotification);
  shortTerm_.second.setBounds(bounds.removeFromTop(labelHeight));
  bounds.removeFromTop(kLabelOffset_);

  measuredVal = valid(stats.loudnessIntegrated)
                    ? juce::String(stats.loudnessIntegrated, kDecimalPlaces_)
                    : kInvalid_;
  integrated_.second.setText(measuredVal, juce::dontSendNotification);
  integrated_.second.setBounds(bounds.removeFromTop(labelHeight));
  bounds.removeFromTop(kLabelOffset_);

  measuredVal = valid(stats.loudnessTruePeak)
                    ? juce::String(stats.loudnessTruePeak, kDecimalPlaces_)
                    : kInvalid_;
  peak_.second.setText(measuredVal, juce::dontSendNotification);
  peak_.second.setBounds(bounds.removeFromTop(labelHeight));
  bounds.removeFromTop(kLabelOffset_);

  measuredVal = valid(stats.loudnessRange)
                    ? juce::String(stats.loudnessRange, kDecimalPlaces_)
                    : kInvalid_;
  range_.second.setText(measuredVal, juce::dontSendNotification);
  range_.second.setBounds(bounds.removeFromTop(labelHeight));
  bounds.removeFromTop(kLabelOffset_);

  target_.second.setBounds(bounds.removeFromTop(labelHeight));
}

void LoudnessStats::buttonClicked(juce::Button* btn) {
  momentary_.second.setText(kInvalid_, juce::dontSendNotification);
  shortTerm_.second.setText(kInvalid_, juce::dontSendNotification);
  integrated_.second.setText(kInvalid_, juce::dontSendNotification);
  peak_.second.setText(kInvalid_, juce::dontSendNotification);
  range_.second.setText(kInvalid_, juce::dontSendNotification);
  rtData_.resetStats.store(true);
  repaint();
}

void LoudnessStats::timerCallback() { repaint(); }
