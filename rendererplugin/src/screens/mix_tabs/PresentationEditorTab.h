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

#include "PresentationEditorViewPort.h"
#include "components/src/ControlKnobSkewed.h"
#include "components/src/GainControlTextEditor.h"
#include "components/src/ImageTextButton.h"
#include "components/src/MixAEContainer.h"
#include "components/src/SelectionBox.h"
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"

class PresentationEditorTabLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  PresentationEditorTabLookAndFeel() {
    setColour(juce::TabbedButtonBar::ColourIds::tabTextColourId,
              EclipsaColours::tabTextGrey);
    setColour(juce::TabbedButtonBar::ColourIds::frontTextColourId,
              EclipsaColours::selectCyan);
    setColour(juce::TabbedButtonBar::ColourIds::tabOutlineColourId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::TabbedButtonBar::ColourIds::frontOutlineColourId,
              EclipsaColours::backgroundOffBlack);
  }

  // Use custom paint instead
  void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                             juce::TextEditor& textEditor) override {}
};

class PresentationEditorTab : public juce::Component,
                              public juce::Timer,
                              public juce::ValueTree::Listener,
                              public juce::Button::Listener {
 public:
  PresentationEditorTab(const juce::Uuid mixPresentationId,
                        MixPresentationRepository* mixPresentationRepository,
                        AudioElementRepository* aeRepository,
                        ActiveMixRepository* activeMixRepository);

  ~PresentationEditorTab() override;

  void paint(juce::Graphics& g) override;

  const juce::ComboBox* const getAudioElementComboBox() const {
    return addAudioElement_.getComboBox();
  };

  juce::Uuid getMixPresentationUuid() { return mixPresentationId_; }

  void updateLocalMap(const int& index);

  const std::set<juce::Uuid> const getAudioElementsToDraw() const {
    std::set<juce::Uuid> audioElementsToDraw;
    for (auto& audioElement : audioElementsAlreadyDrawn_) {
      audioElementsToDraw.insert(audioElement.first);
    }
    return audioElementsToDraw;
  }

 private:
  void updateDeleteMixPresButton();
  void addToAlreadyDrawnMap(const juce::Uuid audioElementId);
  void removeFromAlreadyDrawnMap(const juce::Uuid audioElementId);
  int initializeCurrentMixGain(const juce::Uuid mixPresID);

  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;

  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;

  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;

  void getAudioElements();

  AudioElement* getAudioElement(const juce::Uuid& Id);

  void addAudioElementsToDropDown();

  std::function<void()> audioElementChanged_;

  std::function<void()> languageChanged_;

  void audioElementChangedCallback();

  void languageChangedCallback();

  void buttonClicked(juce::Button* button) override;

  void ensureComboBoxNothingSelected();

  void updateSelectionBoxVisuals();

  void adjustDialAspectRatio(juce::Rectangle<int>& dialBounds);

  void configureLanguageDropDownBox();

  std::function<void()> mixPresentationRemoved_;

  void deleteMixPresentation();

  std::function<void()> changeMixPresentationName_;

  void changeMixPresentationNameCallback();

  void setupTitleTextBox(TitledTextBox& titleTextBox,
                         const std::function<void()>& callback);

  void mixGainChangedCallback();

  void timerCallback();

  int calculateViewPortHeight();

  const juce::Uuid mixPresentationId_;
  const int timerFrequency = 10;

  int sliderValueUnchangedTally_ =
      0;  // increments each timerCallback if the slider value is unchanged
  int latestSliderValue_;    // this is the most recent value retrieved from the
                             // slider
  int previousSliderValue_;  // this is the previous value retrieved from the
                             // slider
  MixPresentationRepository* mixPresentationRepository_;
  ActiveMixRepository* activeMixPresentationRepository_;

  AudioElementRepository* audioElementRepository_;
  juce::OwnedArray<AudioElement> allAudioElementsArray_;  // all audio elements
  TitledTextBox presentationName_;

  std::function<void()> mixGainChanged_;
  GainControlTextEditor gainControl_;
  std::pair<int, int> gainBounds_;
  SelectionBox presentationLanguage_;

  SelectionBox addAudioElement_;
  juce::String currentMixGain_;
  ControlKnobSkewed presentationGainKnob_;

  std::map<juce::Uuid, std::unique_ptr<MixAEContainer>>
      audioElementsAlreadyDrawn_;

  ImageTextButton deleteMixPresentationButton_;

  const juce::String addAudioElementDefaultText_ = "Add Audio Element";

  PresentationEditorTabLookAndFeel lookAndFeel_;

  juce::Rectangle<int> addAudioElementBounds_ =
      juce::Rectangle<int>(0, 0, 0, 0);
  juce::Rectangle<int> containersBounds_ = juce::Rectangle<int>(0, 0, 0, 0);

  PresentationEditorViewPort viewPort_;

  const int deleteButtonHeight_ = 30;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresentationEditorTab)
};
