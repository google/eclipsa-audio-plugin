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

#include "AudioElementPluginEditor.h"

#include <unistd.h>

#include "AudioElementPluginProcessor.h"
#include "components/src/EclipsaColours.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/AudioElementSpatialLayout.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

CustomLookAndFeel::CustomLookAndFeel() {
  setColour(juce::ResizableWindow::backgroundColourId,
            EclipsaColours::backgroundOffBlack);
  setColour(juce::Label::textColourId, EclipsaColours::textWhite);
  setColour(juce::Label::backgroundColourId,
            findColour(juce::ResizableWindow::backgroundColourId));
  setColour(juce::TextButton::ColourIds::buttonColourId,
            EclipsaColours::backgroundOffBlack);
  setColour(juce::TextButton::ColourIds::buttonOnColourId,
            EclipsaColours::rolloverGrey);
  setColour(juce::TextButton::textColourOffId, EclipsaColours::selectCyan);
  setColour(juce::TextButton::ColourIds::textColourOnId,
            EclipsaColours::selectCyan);
}

void CustomLookAndFeel::drawButtonBackground(
    juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour, bool isMouseOverButton,
    bool isButtonDown) {
  auto buttonArea = button.getLocalBounds();

  juce::Colour backColour;
  if (isMouseOverButton) {
    backColour = findColour(juce::TextButton::buttonOnColourId);
  } else {
    backColour = findColour(juce::TextButton::buttonColourId);
  }

  if (isButtonDown) {
    // Darken the background colour
    backColour = backColour.darker(0.5f);
  }

  g.setColour(backColour);
  float cornerSize = buttonArea.getHeight() / 2.0f;
  g.fillRoundedRectangle(buttonArea.toFloat(), cornerSize);
  g.setColour(juce::Colour(136, 147, 146));
  g.drawRoundedRectangle(buttonArea.toFloat(), cornerSize, 2.0f);
}

//==============================================================================

AudioElementPluginEditor::AudioElementPluginEditor(
    AudioElementPluginProcessor& p)
    : juce::AudioProcessorEditor(&p),
      audioElementSpatialLayoutRepository_(
          &p.getRepositories().audioElementSpatialLayoutRepository_),
      syncClient_(&p.getSyncClient()),
      spkrData_(&p.getRepositories().monitorData_),
      trackNameTextBox_("Track name"),
      outputModeTypeLabel_("Output Mode"),
      audioElementSelectionBox_("Audio Element"),
      positionSelectionScreen_(audioElementSpatialLayoutRepository_,
                               p.automationParametersTreeState),
      roomViewScreen_(syncClient_, audioElementSpatialLayoutRepository_,
                      &p.automationParametersTreeState,
                      p.getRepositories().monitorData_),
      trackMonitorScreen_(syncClient_, p.getRepositories()),
      layout_(Speakers::k3Point1Point2, "3.1.2") {
  setResizable(true, true);

  // Get screen dimensions and calculate appropriate size
  auto displays = juce::Desktop::getInstance().getDisplays();
  auto mainDisplay = displays.getPrimaryDisplay();

  if (mainDisplay != nullptr) {
    auto screenArea = mainDisplay->userArea;

    // Calculate plugin size as percentage of screen, with reasonable limits
    int maxWidth = static_cast<int>(screenArea.getWidth() * 1.0f);
    int maxHeight = static_cast<int>(screenArea.getHeight() * 1.0f);

    // Preferred size but constrained by screen
    int preferredWidth = 1552;
    int preferredHeight = 724;

    int actualWidth = juce::jmin(preferredWidth, maxWidth);
    int actualHeight = juce::jmin(preferredHeight, maxHeight);

    // Round to even numbers to avoid rendering artifacts
    actualWidth = (actualWidth + 1) & ~1;    // Round up to even number
    actualHeight = (actualHeight + 1) & ~1;  // Round up to even number

    setSize(actualWidth, actualHeight);

    // Set resize limits that force even dimensions to prevent artifacts
    int minWidth =
        juce::jmin(800, static_cast<int>(screenArea.getWidth() * 0.5f));
    int minHeight =
        juce::jmin(500, static_cast<int>(screenArea.getHeight() * 0.35f));

    // Ensure all resize limits are even numbers
    minWidth = (minWidth + 1) & ~1;
    minHeight = (minHeight + 1) & ~1;
    maxWidth = (maxWidth + 1) & ~1;
    maxHeight = (maxHeight + 1) & ~1;

    setResizeLimits(minWidth, minHeight, maxWidth, maxHeight);
  } else {
    // Fallback if screen detection fails
    setSize(1200, 650);
    setResizeLimits(800, 500, 1600, 900);
  }

  // Listen for updates to the audio elements
  syncClient_->registerListener(this);

  // Fetch setup information from the AudioElementSpatialLayout repository
  AudioElementSpatialLayout config =
      audioElementSpatialLayoutRepository_->get();

  // Set up the look and feel information here
  setLookAndFeel(&customLookAndFeel_);

  // Configure the main editor labels
  panningControlsLabel_.setText("Panning Controls", juce::dontSendNotification);
  panningControlsLabel_.setFont(juce::Font("Roboto", 14.0f, juce::Font::plain));
  panningControlsLabel_.setJustificationType(juce::Justification::centredRight);
  panningControlsLabel_.setColour(juce::Label::ColourIds::textColourId,
                                  EclipsaColours::tabTextGrey);

  titleLabel_.setText("Eclipsa Audio Panner", juce::dontSendNotification);
  titleLabel_.setFont(juce::Font("Audiowide", 30.0f, juce::Font::plain));

  // Handle the panning controls
  panningControls_.setToggleState(config.isPanningEnabled(),
                                  juce::dontSendNotification);
  panningControls_.onClick = [this]() {
    auto repo = audioElementSpatialLayoutRepository_->get();
    repo.setPanningEnabled(panningControls_.getToggleState());
    audioElementSpatialLayoutRepository_->update(repo);
    this->repaint();
  };

  // Set up the default track name
  trackNameTextBox_.setText(config.getName());

  // Determine the input type
  Speakers::AudioElementSpeakerLayout inputLayout =
      Speakers::AudioElementSpeakerLayout(
          p.getBusesLayout().getMainInputChannelSet());

  outputModeTypeLabel_.setEnabled(false);

  // Configure the audio element selection box
  setAudioElementSelection();

  // Update the audio element selection box to update the renderer plugin on
  // change
  audioElementSelectionBox_.onChange([this]() {
    int selected = audioElementSelectionBox_.getSelectedIndex();
    int firstChannel = 0;
    Speakers::AudioElementSpeakerLayout channelLayout = Speakers::kMono;
    juce::Uuid audioElementId = 0;
    if (selected != -1) {
      // Find the audio element and determine what it's first channel is and
      // total channel count is
      juce::OwnedArray<AudioElement> elements;
      syncClient_->getAudioElements(elements);
      AudioElement* selectedElement = elements[selected];
      spkrData_->reinitializeLoudnesses(
          selectedElement->getChannelConfig().getNumChannels());
      firstChannel = selectedElement->getFirstChannel();
      channelLayout = selectedElement->getChannelConfig();
      audioElementId = selectedElement->getId();
    }

    // Push this information down to the processor via the
    // AudioElementSpatialLayout repository
    AudioElementSpatialLayout toUpdate =
        audioElementSpatialLayoutRepository_->get();
    toUpdate.setFirstChannel(firstChannel);
    toUpdate.setLayout(channelLayout);
    toUpdate.setAudioElementId(audioElementId);

    if (selected != -1) {
      toUpdate.setLayoutSelected(true);
    } else {
      toUpdate.setLayoutSelected(false);
    }
    audioElementSpatialLayoutRepository_->update(toUpdate);
  });

  trackNameTextBox_.onTextChanged([this]() {
    AudioElementSpatialLayout toUpdate =
        audioElementSpatialLayoutRepository_->get();
    toUpdate.setName(trackNameTextBox_.getText());
    audioElementSpatialLayoutRepository_->update(toUpdate);
  });

  // Update the panning type and listen for any changes
  setMode();
  audioElementSpatialLayoutRepository_->registerListener(this);
}

AudioElementPluginEditor::~AudioElementPluginEditor() {
  setLookAndFeel(nullptr);
  syncClient_->removeListener(this);
  audioElementSpatialLayoutRepository_->deregisterListener(this);
}

void AudioElementPluginEditor::audioElementsUpdated() {
  setAudioElementSelection();
  repaint();
}

void AudioElementPluginEditor::setAudioElementSelection() {
  audioElementSelectionBox_.clear();
  juce::Uuid toSelect =
      audioElementSpatialLayoutRepository_->get().getAudioElementId();
  juce::OwnedArray<AudioElement> elements;
  syncClient_->getAudioElements(elements);
  juce::String selectedElementName = "";
  audioElementEnabled_.clear();
  static const bool isLogic = juce::PluginHostType().isLogic();
  int hostOutChans = 0;
  if (auto* proc =
          dynamic_cast<AudioElementPluginProcessor*>(getAudioProcessor())) {
    hostOutChans = proc->getBusesLayout().getMainOutputChannelSet().size();
  }
  for (int i = 0; i < elements.size(); i++) {
    bool enable = true;
    if (isLogic) {
      int layoutChannels = elements[i]->getChannelConfig().getNumChannels();
      // Allow selecting layouts whose channel count does not exceed host output
      if (hostOutChans > 0 && layoutChannels > hostOutChans) enable = false;
    }
    if (elements[i]->getId() == toSelect) {
      selectedElementName = elements[i]->getName();
      roomViewScreen_.updateSpeakerSetup(elements[i]->getChannelConfig());
    }
    // Add option with enabled/disabled state (disabled are non-clickable)
    audioElementSelectionBox_.addOption(elements[i]->getName(), enable);
    audioElementEnabled_.push_back(enable);
  }
  audioElementSelectionBox_.setOption(selectedElementName);
}

void AudioElementPluginEditor::paint(juce::Graphics& g) {
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  // First, create some padding around our widgets
  auto bounds = getLocalBounds();
  bounds.removeFromTop(20);
  bounds.removeFromBottom(20);
  bounds.removeFromLeft(40);
  bounds.removeFromRight(40);

  // Add the slider button for the panning controls
  auto titleBarBounds = bounds.removeFromTop(40);
  addAndMakeVisible(panningControls_);
  panningControls_.setBounds(titleBarBounds.removeFromRight(55).reduced(5));
  addAndMakeVisible(panningControlsLabel_);
  panningControlsLabel_.setBounds(titleBarBounds.removeFromRight(200));

  // Add the title label
  addAndMakeVisible(titleLabel_);
  titleLabel_.setText("Eclipsa Audio Element Plugin",
                      juce::dontSendNotification);
  titleLabel_.setFont(juce::Font("Audiowide", 30.0f, juce::Font::plain));
  titleLabel_.setBounds(titleBarBounds);

  // Add the title seperator line
  juce::Colour gradientWhite =
      getLookAndFeel().findColour(juce::Label::textColourId);
  juce::Colour gradientBrown(140, 78, 41);
  g.setGradientFill(juce::ColourGradient(
      gradientWhite, bounds.getX(), bounds.getY(), gradientBrown,
      bounds.getWidth(), bounds.getY(), false));
  g.drawRect(bounds.removeFromTop(2));
  bounds.removeFromTop(20);  // Add padding under the seperator

  // Add in the panning controls
  auto rightBounds = bounds.removeFromRight(bounds.getWidth() / 2);
  auto leftBounds = bounds;

  auto controlBounds = leftBounds.removeFromTop(80);
  addAndMakeVisible(trackNameTextBox_);
  trackNameTextBox_.setBounds(controlBounds.removeFromLeft(210));
  controlBounds.removeFromLeft(20);

  addAndMakeVisible(outputModeTypeLabel_);
  outputModeTypeLabel_.setBounds(controlBounds.removeFromLeft(170));
  controlBounds.removeFromLeft(20);

  addAndMakeVisible(audioElementSelectionBox_);
  audioElementSelectionBox_.setBounds(controlBounds.removeFromLeft(170));

  // Add in the 3 sub-screens
  addAndMakeVisible(roomViewScreen_);
  roomViewScreen_.setBounds(leftBounds);

  addAndMakeVisible(trackMonitorScreen_);

  int trackMonitorHeightTrim =
      panningControls_.getToggleState() == true ? 0 : 110;
  auto trackMonitorBounds = rightBounds.removeFromTop(
      (rightBounds.getHeight() / 2) + trackMonitorHeightTrim);
  trackMonitorBounds.removeFromTop(trackMonitorHeightTrim);
  trackMonitorBounds.setRight(trackMonitorBounds.getRight() + 20);
  trackMonitorScreen_.setBounds(trackMonitorBounds);

  if (panningControls_.getToggleState()) {
    addAndMakeVisible(positionSelectionScreen_);
    positionSelectionScreen_.setBounds(rightBounds);
  } else {
    positionSelectionScreen_.setVisible(false);
  }
}

void AudioElementPluginEditor::resized() {
  // Snap to even dimensions to prevent rendering artifacts
  auto currentBounds = getBounds();
  int width = currentBounds.getWidth();
  int height = currentBounds.getHeight();

  // Round to even numbers
  int evenWidth = (width + 1) & ~1;
  int evenHeight = (height + 1) & ~1;

  // Only update if dimensions changed
  if (width != evenWidth || height != evenHeight) {
    setBounds(currentBounds.getX(), currentBounds.getY(), evenWidth,
              evenHeight);
    return;  // Avoid infinite recursion
  }

  // Continue with normal layout logic
  repaint();
}

void AudioElementPluginEditor::setMode() {
  if (audioElementSpatialLayoutRepository_->get().isPanningEnabled()) {
    outputModeTypeLabel_.setText("Panning Mode");
  } else {
    outputModeTypeLabel_.setText("Passthrough Mode");
  }
}
