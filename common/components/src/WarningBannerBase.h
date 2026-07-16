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

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../logger/logger.h"
#include "Icons.h"

// Shared visual chrome for dismissible, single-line warning banners: a
// colored strip of text with a close button in the top-right that persists
// dismissal via a subclass-supplied hook. Subclasses provide the message text
// and (optionally) their own dismiss semantics and fill color.
class WarningBannerBase : public juce::Component, public juce::Button::Listener {
 public:
  WarningBannerBase() {
    bool iconLoaded = false;
    if (IconStore::getInstance().getCloseIcon().isValid()) {
      juce::Image closeIcon = IconStore::getInstance().getCloseIcon();
      const int buttonSize = 18;
      closeIcon = closeIcon.rescaled(buttonSize, buttonSize,
                                     juce::Graphics::highResamplingQuality);

      closeButton_.setImages(false, true, true, closeIcon, 1.0f,
                             juce::Colours::black, closeIcon, 1.0f,
                             juce::Colours::black.withAlpha(0.7f), closeIcon,
                             0.8f, juce::Colours::darkgrey);
      iconLoaded = true;
    } else {
      LOG_ERROR(0,
                "WarningBannerBase CONSTRUCTOR: "
                "IconStore::getInstance().getCloseIcon() is not valid.");
    }

    // If icon loading failed, create a text button as fallback
    if (!iconLoaded) {
      closeButton_.setButtonText("✕");
      closeButton_.setColour(juce::TextButton::buttonColourId,
                             juce::Colours::transparentBlack);
      closeButton_.setColour(juce::TextButton::textColourOffId,
                             juce::Colours::black);
    }

    closeButton_.setTooltip("Dismiss this warning");
    closeButton_.addListener(this);
    addAndMakeVisible(closeButton_);
  }

  ~WarningBannerBase() override { closeButton_.removeListener(this); }

  void setVisible(bool shouldBeVisible) override {
    Component::setVisible(shouldBeVisible);
  }

  void paint(juce::Graphics& g) override {
    g.fillAll(fillColour_);

    g.setColour(juce::Colours::black);
    g.setFont(14.0f);

    g.drawText(getMessageText(),
               getLocalBounds().reduced(10, 2).withTrimmedRight(40),
               juce::Justification::centred, true);
  }

  void resized() override {
    const int buttonSize = 18;
    const int padding = 10;
    closeButton_.setBounds(getWidth() - buttonSize - padding,
                           (getHeight() - buttonSize) / 2, buttonSize,
                           buttonSize);
  }

  void buttonClicked(juce::Button* button) override {
    if (button == &closeButton_) {
      onDismiss();
      setVisible(false);

      if (auto* parentComponent = getParentComponent()) {
        parentComponent->repaint();
      }
    }
  }

  void updatePosition(int yPosition, int width) {
    const int bannerHeight = 30;
    setBounds(0, yPosition, width, bannerHeight);
  }

 protected:
  // Subclass supplies the message text drawn in the banner body.
  virtual juce::String getMessageText() const = 0;

  // Subclass hook invoked when the close button is clicked, before the
  // banner is hidden. Default is a no-op.
  virtual void onDismiss() {}

  juce::Colour fillColour_ = juce::Colour(0xFFFFCC66);
  juce::ImageButton closeButton_;
};
