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
#include "DAWCompatibilityChecker.h"
#include "Icons.h"
#include "data_repository/implementation/RoomSetupRepository.h"

class DAWWarningBanner : public juce::Component, public juce::Button::Listener {
 public:
  explicit DAWWarningBanner(RoomSetupRepository* roomSetupRepo)
      : m_roomSetupRepository_(roomSetupRepo),
        m_isDAWSupported(true),
        m_dismissedInRepo(false) {
    juce::PluginHostType hostType;
    m_hostName = hostType.getHostDescription();
    m_isDAWSupported = DAWCompatibilityChecker::isDAWSupported();

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
                "DAW Banner CONSTRUCTOR: "
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

    m_dismissedInRepo = false;
    if (m_roomSetupRepository_) {
      m_dismissedInRepo =
          m_roomSetupRepository_->get().getDawWarningDismissed();
    }

    bool shouldBeVisible = !m_isDAWSupported && !m_dismissedInRepo;
    setVisible(shouldBeVisible);
  }

  void setVisible(bool shouldBeVisible) override {
    Component::setVisible(shouldBeVisible);
  }

  void refreshVisibility() {
    m_isDAWSupported = DAWCompatibilityChecker::isDAWSupported();

    if (m_roomSetupRepository_) {
      m_dismissedInRepo =
          m_roomSetupRepository_->get().getDawWarningDismissed();
    }
    setVisible(!m_isDAWSupported && !m_dismissedInRepo);
  }

  ~DAWWarningBanner() override { closeButton_.removeListener(this); }

  void paint(juce::Graphics& g) override {
    g.fillAll(juce::Colour(0xFFFFCC66));

    g.setColour(juce::Colours::black);
    g.setFont(14.0f);

    juce::String warningText =
        m_hostName +
        " support isn't officially tested yet—functionality may vary.";

    g.drawText(warningText,
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
      if (m_roomSetupRepository_) {
        RoomSetup currentRoomSetup = m_roomSetupRepository_->get();
        currentRoomSetup.setDawWarningDismissed(true);
        m_roomSetupRepository_->update(currentRoomSetup);
        m_dismissedInRepo = true;
      }
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

 private:
  RoomSetupRepository* m_roomSetupRepository_;
  juce::ImageButton closeButton_;
  juce::String m_hostName;
  bool m_isDAWSupported = true;
  bool m_dismissedInRepo = false;
};