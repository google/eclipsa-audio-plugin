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
#include <components/components.h>

#include "MixPresentationTagScreen.h"
#include "components/src/MainEditor.h"
#include "data_repository/implementation/MixPresentationRepository.h"

class EditPresentationScreenLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  EditPresentationScreenLookAndFeel() {
    setColour(juce::Label::backgroundColourId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::Label::textColourId, EclipsaColours::selectCyan);
    setColour(juce::TextButton::buttonColourId, EclipsaColours::selectCyan);
    setColour(juce::TextButton::textColourOffId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::TextButton::textColourOnId,
              EclipsaColours::backgroundOffBlack);
  }

  // Use custom paint instead
  void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                             juce::TextEditor& textEditor) override {}
};

class EditPresentationTabbedComponent : public juce::TabbedComponent {
 public:
  EditPresentationTabbedComponent(
      MixPresentationRepository* mixPresentationRepository,
      MixPresentationTagScreen* tagScreen);

  ~EditPresentationTabbedComponent() override;

  void currentTabChanged(int newCurrentTabIndex,
                         const juce::String& newCurrentTabName) override;

 private:
  MixPresentationRepository* mixPresentationRepository_;
  MixPresentationTagScreen* tagScreen_;
};

class EditPresentationScreen : public juce::Component,
                               public juce::ValueTree::Listener {
 public:
  EditPresentationScreen(MainEditor& editor,
                         AudioElementRepository* ae_repository,
                         MixPresentationRepository* mixPresentationRepository,
                         ActiveMixRepository* activeMixRepository);

  ~EditPresentationScreen();

  void paint(juce::Graphics& g) override;

 private:
  void test_addMixPresentation();
  void updateTabButtonBounds(const juce::Rectangle<int>& mixEditingBounds);
  void updateMixPresentations();
  void updatePresentationTabs();
  std::function<void()> addMixPresentation;

  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;
  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;

  int initialTabIndex_;
  HeaderBar headerBar_;
  MixPresentationRepository* mixPresentationRepository_;
  AudioElementRepository* audioElementRepository_;
  ActiveMixRepository* activeMixRepository_;
  juce::OwnedArray<MixPresentation> mixPresentationArray_;
  int numMixes_ = 0;

  MixPresentationTagScreen tagScreen_;
  std::unique_ptr<EditPresentationTabbedComponent> presentationTabs_;
  EditPresentationScreenLookAndFeel lookAndFeel_;

  // Add a control button to return to the main screen
  ImageTextButton addMixPresentationButton_;
  juce::Label titleLabel_;
};