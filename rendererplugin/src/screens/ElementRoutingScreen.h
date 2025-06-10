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

#include <memory>
#include <vector>

#include "components/src/ColouredLight.h"
#include "components/src/EclipsaColours.h"
#include "components/src/HeaderBar.h"
#include "components/src/Icons.h"
#include "components/src/SelectionBox.h"
#include "components/src/SelectionButton.h"
#include "components/src/TitledTextBox.h"
#include "components/src/Viewports.h"
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/FileExport.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class AudioElementColumn;
class PannerRow;
class PannerLabel;

class ElementRoutingScreenLookAndFeel : public juce::LookAndFeel_V4 {
 public:
  ElementRoutingScreenLookAndFeel() {
    setColour(juce::Label::backgroundColourId,
              EclipsaColours::backgroundOffBlack);
    setColour(juce::Label::textColourId, EclipsaColours::headingGrey);
  }

  // Use custom paint instead
  void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                             juce::TextEditor& textEditor) override {}
};

class ElementRoutingScreen : public juce::Component,
                             juce::ComboBox::Listener,
                             juce::ValueTree::Listener {
 public:
  ElementRoutingScreen(MainEditor& editor,
                       AudioElementRepository* audioElementRepository,
                       MultibaseAudioElementSpatialLayoutRepository*
                           audioElementSpatialLayoutRepository,
                       FileExportRepository* fileExportRepository,
                       MixPresentationRepository* mixPresentationRepository,
                       int totalChanneCount);

  ~ElementRoutingScreen();

  void removeAudioElement(AudioElement* element);

  void updateAudioElementName(juce::Uuid& element, juce::String name);

  void paint(juce::Graphics& g) override;

  void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

  void updateAudioElementChannels();

  void removeAudioElement(juce::Uuid& element);

  /* ====================
      Value tree listener functions, listening for updates to the panner value
     tree
   ======================*/
  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override {
    updateAudioElementChannels();
    repaint();
  }

  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override {
    updateAudioElementChannels();
    repaint();
  }

  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override {
    updateAudioElementChannels();
    repaint();
  }

  bool confirmValidNameChange(TitledTextBox& textBox, const juce::Uuid id) {
    // this name is already taken
    // do no update the name
    // ensure the audioElementName text editor is restored to the old name
    std::set<juce::String> existingNames;
    juce::OwnedArray<AudioElement> audioElements;
    audioElementRepository_->getAll(audioElements);
    for (auto audioElement : audioElements) {
      if (audioElement->getName() == textBox.getText()) {
        textBox.setText(audioElementRepository_->get(id)->getName());
        return false;
      }
    }
    return true;
  }

 private:
  juce::String formatAudioElementName(
      const Speakers::AudioElementSpeakerLayout& layout);
  juce::StringArray getAudioElementNames(const FileProfile& profile);
  void updateAddAudioElementButton(const juce::StringArray& audioElementNames);
  Speakers::AudioElementSpeakerLayout getAudioElementLayout(
      const juce::String& name);

  HeaderBar headerBar_;
  SelectionBox profileSelectionBox_;
  SelectionButton addAudioElementButton_;
  juce::Label tracksLabel_;
  juce::Label remainingChannelsLabel_;
  AudioElementRepository* audioElementRepository_;
  MultibaseAudioElementSpatialLayoutRepository*
      audioElementSpatialLayoutRepository_;
  FileExportRepository* fileExportRepository_;
  MixPresentationRepository* mixPresentationRepository_;
  std::map<juce::String, juce::Uuid>* pannerInformation_;
  std::vector<std::unique_ptr<AudioElementColumn>> audioElementColumns_;
  std::vector<std::unique_ptr<PannerLabel>> pannerLabels_;
  std::vector<std::unique_ptr<PannerRow>> pannerRows_;
  const std::vector<Speakers::AudioElementSpeakerLayout> kLayoutsRef_;

  // Viewport containing AE at the top of the screen
  HorizontalViewportContainer audioElementContainer_;
  LinkedViewport audioElementViewport_;

  // Horizontal scrolling viewport showing which audio elements
  // are associated with which panners
  // Linked to the audioElementViewport for scrolling
  VerticalViewportContainer pannerAudioElementTableContainer_;
  juce::Viewport pannerAudioElementTableViewport_;

  // Two containers in the vertical viewport, which handles vertical scrolling
  // through panners. the TrackLabelContainer contains the track labels on
  // the left side and the TrackViewContainer contains the trackLabelContainer
  // and the pannerAudioElementTableViewport for vertical scrolling
  HorizontalViewportContainer trackViewContainer_;
  VerticalViewportContainer trackLabelContainer_;
  juce::Viewport trackVerticalViewport_;

  ElementRoutingScreenLookAndFeel lookAndFeel_;
  FileProfile currentProfile_;
  int channelsInUse_;
  juce::TooltipWindow tooltipWindow_;
  juce::ImageComponent tooltipImage_;
};

class PannerLabel : public juce::Component {
 public:
  PannerLabel(juce::String name, juce::Colour bgColour)
      : backgroundColour_(bgColour) {
    name_.setText(name, juce::NotificationType::dontSendNotification);
    juce::Image labelImage = IconStore::getInstance().getTrackIcon();
    bullet_.setImage(labelImage);
  }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    // Draw the background
    g.setColour(backgroundColour_);
    g.fillRect(bounds);

    // Draw the bullet
    addAndMakeVisible(bullet_);
    bullet_.setBounds(
        bounds.removeFromLeft(25).withTrimmedLeft(5).withTrimmedTop(2));

    // Draw the name
    addAndMakeVisible(name_);
    name_.setBounds(bounds.removeFromLeft(225));
    name_.setColour(juce::Label::backgroundColourId, backgroundColour_);
  }

 private:
  juce::Colour backgroundColour_;
  juce::Label name_;
  juce::ImageComponent bullet_;
};

class PannerRow : public juce::Component {
 public:
  PannerRow(juce::Colour bgColour, int audioElementIndex,
            int totalAudioElements)
      : backgroundColour_(bgColour) {
    for (int i = 0; i < totalAudioElements; i++) {
      if (i == audioElementIndex) {
        audioElementLights_.push_back(
            std::make_unique<ColouredLight>(juce::Colour(212, 123, 71)));
      } else {
        audioElementLights_.push_back(
            std::make_unique<ColouredLight>(juce::Colour(106, 96, 89)));
      }
    }
  }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    // Draw the background
    g.setColour(backgroundColour_);
    g.fillRect(bounds);

    // Draw the audio element lights
    for (auto& light : audioElementLights_) {
      light->setBounds(bounds.removeFromLeft(250)
                           .withTrimmedLeft((250 - 20) / 2)
                           .withTrimmedRight((250 - 20) / 2)
                           .withTrimmedTop(6)
                           .withTrimmedBottom(6));
      bounds.removeFromLeft(50);  // Account for padding
      addAndMakeVisible(light.get());
    }

    bounds.setLeft(bounds.getTopLeft().getX() - 50);
    g.setColour(EclipsaColours::backgroundOffBlack);
    g.fillRect(bounds);
  }

 private:
  juce::Colour backgroundColour_;
  std::vector<std::unique_ptr<ColouredLight>> audioElementLights_;
};

class AudioElementColumn : public juce::Component, juce::ImageButton::Listener {
 public:
  AudioElementColumn(AudioElement& element, ElementRoutingScreen* screen)
      : audioElementName_("Audio Element"),
        elementId_(element.getId()),
        columnScreen_(screen) {
    if (element.getChannelCount() == 1) {
      audioElementChannels_.setText(
          "Channel " + juce::String(element.getFirstChannel() + 1),
          juce::dontSendNotification);
    } else {
      audioElementChannels_.setText(
          "Channels " + juce::String(element.getFirstChannel() + 1) + " - " +
              juce::String(element.getFirstChannel() +
                           element.getChannelCount()),
          juce::dontSendNotification);
    }
    audioElementName_.setText(element.getName());
    audioElementName_.setOnReturnCallback([this]() {
      if (columnScreen_->confirmValidNameChange(audioElementName_,
                                                elementId_)) {
        // Update the audio element name
        columnScreen_->updateAudioElementName(elementId_,
                                              audioElementName_.getText());
      }
    });
    audioElementName_.setOnFocusLostCallback([this]() {
      if (columnScreen_->confirmValidNameChange(audioElementName_,
                                                elementId_)) {
        // Update the audio element name
        columnScreen_->updateAudioElementName(elementId_,
                                              audioElementName_.getText());
      }
    });

    juce::Image deleteImage = IconStore::getInstance().getDeleteIcon();
    deleteButton_.setImages(false, true, true, deleteImage, 1.0f,
                            juce::Colours::transparentBlack, deleteImage, 0.5f,
                            juce::Colours::grey, deleteImage, 0.8f,
                            juce::Colours::white);

    deleteButton_.addListener(this);
  }

  void buttonClicked(juce::Button* btn) {
    // Delete the audio element
    columnScreen_->removeAudioElement(elementId_);
  }

  void paint(juce::Graphics& g) override {
    auto bounds = getLocalBounds();

    // Draw the seperator bar
    g.setColour(juce::Colour(63, 73, 72));
    g.drawRect(bounds.removeFromBottom(2));
    audioElementChannels_.setBounds(bounds);

    // Draw the audio element channels
    addAndMakeVisible(audioElementChannels_);
    audioElementChannels_.setBounds(bounds.removeFromBottom(20));

    // Draw the audio element name
    addAndMakeVisible(audioElementName_);
    bounds = bounds.removeFromTop(65);
    audioElementName_.setBounds(bounds.removeFromLeft(200));

    addAndMakeVisible(deleteButton_);
    deleteButton_.setBounds(bounds.withTrimmedTop(15).reduced(10));
  }

  void disableDelete() { deleteButton_.setEnabled(false); }

 private:
  TitledTextBox audioElementName_;
  juce::Label audioElementChannels_;
  juce::ImageButton removeButton_;
  juce::Image deleteButtonImage_;
  juce::ImageButton deleteButton_;
  ElementRoutingScreen* columnScreen_;
  juce::Uuid elementId_;
};