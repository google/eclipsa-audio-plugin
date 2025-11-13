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

#include "RoomMonitoringScreen.h"

#include <components/components.h>
#include <substream_rdr/substream_rdr.h>

#include "../RendererProcessor.h"
#include "data_structures/src/FileExport.h"

RoomMonitoringScreen::RoomMonitoringScreen(RepositoryCollection repos,
                                           SpeakerMonitorData& data,
                                           MainEditor& editor)
    : repos_(repos),
      monitorData_(data),
      fileExportScreen_(editor, repos),
      speakerSetup_("Speaker Setup"),
      selRoomOpts_({"Speakers", "Tracks", "Labels"}, false),
      selRoomView_({"Iso", "Top", "Side", "Rear"}, true),
      exportButton(IconStore::getInstance().getsettingsIcon()) {
  // Assuming exportButton is a juce::TextButton
  exportButton.setExportLookAndFeel(IconStore::getInstance().getsettingsIcon(),
                                    IconStore::getInstance().getLineIcon());
  initializeSpeakerSetup();
  addAndMakeVisible(speakerSetup_);

  // Set up functionality in the constructor
  exportButton.setButtonOnClick([this, &editor] {
    LOG_ANALYTICS(
        RendererProcessor::instanceId_,
        "Export button clicked; transitioning to File Export screen.");
    // Set the main screen to the routing screen
    editor.setScreen(fileExportScreen_);
  });

  exportButton.setButtonText("Export");
  addAndMakeVisible(exportButton);

  roomView_ = std::make_unique<IsoView>(monitorData_, repos_);
  addAndMakeVisible(roomView_.get());

  selRoomOpts_.onChange([this] { updateRoomOpts(); });
  addAndMakeVisible(selRoomOpts_);

  // Pull last room opts from repo and set accordingly.
  RoomSetup roomSetupData = repos_.roomSetupRepo_.get();
  selRoomOpts_.setOption("Speakers", roomSetupData.getDrawSpeakers());
  selRoomOpts_.setOption("Labels", roomSetupData.getDrawSpeakerLabels());
  selRoomOpts_.setToggleable("Labels", false);
  selRoomOpts_.setOption("Tracks", roomSetupData.getDrawTracks());

  selRoomView_.onChange([this] { updateRoomView(); });
  selRoomView_.toggleOn(roomSetupData.getCurrentRoomView());
  updateRoomView();
  addAndMakeVisible(selRoomView_);

  // Listen for changes in the Active Mix Presentation repository for drawing
  // active tracks / Audio Elements.
  repos_.activeMPRepo_.registerListener(this);
  updateActiveIDs(repos_, roomView_);

  startTimerHz(60);
}

void RoomMonitoringScreen::initializeSpeakerSetup() {
  LOG_ANALYTICS(RendererProcessor::instanceId_, "Initializing speaker setup.");
  for (const auto& channelSet : speakerLayoutConfigurationOptions) {
    speakerSetup_.addOption(channelSet.getDescription());
    // Speaker setup to display at top of box.
    auto currSpkrLayout = repos_.roomSetupRepo_.get().getSpeakerLayout();
    if (channelSet == currSpkrLayout) {
      speakerSetup_.setOption(currSpkrLayout.getDescription());
    }
  }
  speakerSetup_.onChange([this] { updateSpeakerSetup(); });
}

RoomMonitoringScreen::~RoomMonitoringScreen() {
  setLookAndFeel(nullptr);

  // Update repository with the current room view configuration.
  auto roomSetupData = repos_.roomSetupRepo_.get();

  for (const auto& buttonState : selRoomOpts_.getState()) {
    if (buttonState.first == "Speakers") {
      roomSetupData.setDrawSpeakers(buttonState.second);
    } else if (buttonState.first == "Labels") {
      roomSetupData.setDrawSpeakerLabels(buttonState.second);
    } else if (buttonState.first == "Tracks") {
      roomSetupData.setDrawTracks(buttonState.second);
    }
  }

  // Only one room view can be toggled at a time.
  roomSetupData.setCurrentRoomView(selRoomView_.getToggled().back());

  repos_.roomSetupRepo_.update(roomSetupData);
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "RoomMonitoringScreen destroyed; saved state to repository.");
  repos_.activeMPRepo_.deregisterListener(this);
}

void RoomMonitoringScreen::paint(juce::Graphics& g) {
  // Split the bounds into 3 sections:
  // 1. Top dropdowns / buttons.
  // 2. Room view.
  // 3. Bottom buttons.
  auto bounds = getLocalBounds();
  bounds.removeFromRight(40);
  int height = bounds.getHeight();
  auto topBounds = bounds.removeFromTop(height * 0.10f);
  auto botBounds = bounds.removeFromBottom(height * 0.06f);
  auto roomBounds = bounds;

  // Draw speaker speaker dropdown, timecode, and export button.
  int topWidth = topBounds.getWidth();
  speakerSetup_.setBounds(topBounds.removeFromLeft(topWidth * 0.20f));

  // Adjusted button width to accommodate images and text
  int exportbuttonWidth =
      129;  // Width sufficient to fit two images and text with padding
  int buttonHeight = 40;  // Standard button height

  // Calculate the x position: x-coordinate of the right side of topBounds minus
  // the width of the button
  int buttonX = topBounds.getRight() - exportbuttonWidth -
                10;  // 10 pixels padding from the right edge

  // Calculate the y position: Vertically center the button within the topBounds
  int buttonY = topBounds.getY() + (topBounds.getHeight() - buttonHeight) / 2;

  // Now set the bounds of the exportButton
  exportButton.setBounds(buttonX, buttonY, exportbuttonWidth, buttonHeight);

  roomView_->setBounds(roomBounds);

  // Draw room view buttons.
  const int kButtonOffset = 20;
  int buttonWidth = (botBounds.getWidth() - kButtonOffset) / 2;
  selRoomOpts_.setBounds(botBounds.removeFromLeft(buttonWidth));
  botBounds.removeFromLeft(kButtonOffset);
  selRoomView_.setBounds(botBounds);
}

void RoomMonitoringScreen::updateSpeakerSetup() {
  // Update the currently selected speaker layout in the repo.
  int speakerLayoutIdx = speakerSetup_.getSelectedIndex();
  RoomLayout newSpeakerLayout =
      speakerLayoutConfigurationOptions[speakerLayoutIdx];

  RoomSetup currentRoomSetup = repos_.roomSetupRepo_.get();
  currentRoomSetup.setSpeakerLayout(newSpeakerLayout);
  repos_.roomSetupRepo_.update(currentRoomSetup);

  monitorData_.reinitializeLoudnesses(
      newSpeakerLayout.getRoomSpeakerLayout().getNumChannels());
  LOG_ANALYTICS(RendererProcessor::instanceId_, " updated speaker setup.");
  roomView_->setSpeakers(newSpeakerLayout.getRoomSpeakerLayout());
  roomView_->repaint();
}

void RoomMonitoringScreen::updateRoomOpts() {
  // Based on button toggled, update room view state then repaint.
  for (const auto& buttonState : selRoomOpts_.getState()) {
    if (buttonState.first == "Speakers") {
      roomView_->setDisplaySpeakers(buttonState.second);
      toggleLabelsDrawable();
    } else if (buttonState.first == "Labels") {
      roomView_->setDisplayLabels(buttonState.second);
    } else if (buttonState.first == "Tracks") {
      roomView_->setDisplayTracks(buttonState.second);
      updateActiveIDs(repos_, roomView_);
      toggleLabelsDrawable();
    }
  }
  LOG_ANALYTICS(RendererProcessor::instanceId_, " updated room options.");
  repaint();
}

void RoomMonitoringScreen::toggleLabelsDrawable() {
  // Only allow drawing speaker labels when Speakers or Tracks are being drawn.
  if (selRoomOpts_.getOption("Speakers") || selRoomOpts_.getOption("Tracks")) {
    selRoomOpts_.setToggleable("Labels", true);

  } else {
    selRoomOpts_.setOption("Labels", false);
    selRoomOpts_.setToggleable("Labels", false);
  }
}

void RoomMonitoringScreen::updateRoomView() {
  std::vector<juce::String> selViews = selRoomView_.getToggled();
  // Catch disaster case.
  if (selViews.empty()) {
    // NOTE: Iso view enabled by default.
    selRoomView_.toggleOn("Iso");
    selViews = selRoomView_.getToggled();
  }

  juce::String selView = selViews.back();
  LOG_ANALYTICS(RendererProcessor::instanceId_,
                "room setup changed to: " + selView.toStdString());
  if (selView == "Iso") {
    roomView_ = std::make_unique<IsoView>(monitorData_, repos_);
  } else if (selView == "Top") {
    roomView_ = std::make_unique<TopView>(monitorData_, repos_);
  } else if (selView == "Side") {
    roomView_ = std::make_unique<SideView>(monitorData_, repos_);
  } else {
    roomView_ = std::make_unique<RearView>(monitorData_, repos_);
  }

  addAndMakeVisible(roomView_.get());
  updateRoomOpts();
  updateSpeakerSetup();
  roomView_->repaint();
}

// Updates the room view with the IDs of the audio element plugins associated
// with audio elements that are a part of the active mix presentation.
void RoomMonitoringScreen::updateActiveIDs(
    RepositoryCollection& repos, std::unique_ptr<PerspectiveRoomView>& prv) {
  const juce::Uuid kActiveMix = repos.activeMPRepo_.get().getActiveMixId();
  const MixPresentation kActiveMixPres = repos.mpRepo_.get(kActiveMix).value();
  activeAudioElementIDs_.clear();
  for (const MixPresentationAudioElement& ae :
       kActiveMixPres.getAudioElements()) {
    activeAudioElementIDs_.insert(ae.getId());
  }
}

void RoomMonitoringScreen::updateActiveTrackData() {
  std::vector<AudioElementUpdateData> activeTracks;
  repos_.audioElementSubscriber_.getData([&](AudioElementUpdateData data) {
    // First check if this track is part of the active mix, with a memcpy to
    // circumvent illegal pointer conversions.
    juce::uint8 rawUUID[sizeof(AudioElementUpdateData::uuid)];
    std::memcpy(rawUUID, data.uuid.data(),
                sizeof(AudioElementUpdateData::uuid));

    const std::optional<AudioElementSpatialLayout> kAudioElementSpatialLayout =
        repos_.audioElementSpatialLayoutRepo_.get(juce::Uuid(rawUUID));

    if (kAudioElementSpatialLayout &&
        activeAudioElementIDs_.contains(
            kAudioElementSpatialLayout->getAudioElementId())) {
      activeTracks.push_back(data);
    }
  });
  roomView_->setTracks(activeTracks);
}

void RoomMonitoringScreen::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  if (treeWhosePropertyHasChanged.getType() ==
      ActiveMixPresentation::kTreeType) {
    updateActiveIDs(repos_, roomView_);
  }
}
void RoomMonitoringScreen::valueTreeChildAdded(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) {
  if (parentTree.getType() == MixPresentation::kTreeType ||
      parentTree.getType() == AudioElement::kTreeType) {
    updateActiveIDs(repos_, roomView_);
  }
}
void RoomMonitoringScreen::valueTreeChildRemoved(
    juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved,
    int indexFromWhichChildWasRemoved) {
  if (parentTree.getType() == MixPresentation::kTreeType ||
      parentTree.getType() == AudioElement::kTreeType) {
    updateActiveIDs(repos_, roomView_);
  }
}

void RoomMonitoringScreen::timerCallback() {
  updateActiveTrackData();
  roomView_->repaint();
}