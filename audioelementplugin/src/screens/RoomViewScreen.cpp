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

#include "RoomViewScreen.h"

RoomViewScreen::RoomViewScreen(
    AudioElementPluginSyncClient* syncClient,
    AudioElementSpatialLayoutRepository* audioElementSpatialLayoutRepo,
    AudioElementParameterTree* tree, const SpeakerMonitorData& monitorData)
    : syncClient_(syncClient),
      audioElementSpatialLayoutRepository_(audioElementSpatialLayoutRepo),
      parameterTree_(tree),
      onRoomElevationChange_([this] { elevationChangeCallback(); }),
      room_(std::make_unique<AudioElementPluginTopView>(monitorData)),
      selRoomElevation_({IconStore::getInstance().getFlatElevationIcon(),
                         IconStore::getInstance().getTentElevationIcon(),
                         IconStore::getInstance().getArchElevationIcon(),
                         IconStore::getInstance().getDomeElevationIcon(),
                         IconStore::getInstance().getCurveElevationIcon()},
                        true),
      spkrData_(monitorData) {
  audioElementSpatialLayoutRepository_->registerListener(this);

  // The panner writes the position parameters when the source is dragged.
  room_->setParameterTree(parameterTree_);

  // If the audio element plugin is implementing a valid audio element, display
  // the speaker layout.
  if (!audioElementSpatialLayoutRepository_->get()
           .getAudioElementId()
           .isNull()) {
    room_->setDisplaySpeakers(true);
    room_->setSpeakers(
        audioElementSpatialLayoutRepository_->get().getChannelLayout());
  }
  room_->setDisplayLabels(true);
  addAndMakeVisible(room_.get());

  // Configure the roof selection, but only make visible if panning is enabled

  addAndMakeVisible(selRoomElevation_);
  selRoomElevation_.onChange(onRoomElevationChange_);
  selRoomElevation_.setToggled(static_cast<int>(
      audioElementSpatialLayoutRepository_->get().getElevation()));
  selRoomElevation_.setVisible(
      audioElementSpatialLayoutRepository_->get().isPanningEnabled());

  // This timer sets the refresh rate at which the room view is redrawn.
  startTimerHz(60);
}

RoomViewScreen::~RoomViewScreen() {
  setLookAndFeel(nullptr);
  audioElementSpatialLayoutRepository_->deregisterListener(this);
}

void RoomViewScreen::paint(juce::Graphics& g) {
  // Split the bounds into the control buttons and the audio element
  // monitoring view
  auto bounds = getLocalBounds();
  // create a copy for reference
  auto viewScreenBounds = bounds;

  auto roomViewBounds =
      bounds.removeFromTop(viewScreenBounds.proportionOfHeight(0.9f));
  room_->setBounds(roomViewBounds);

  auto elevationToggleBounds = bounds;
  elevationToggleBounds.reduce(viewScreenBounds.proportionOfWidth(0.11f), 0.f);
  elevationToggleBounds.removeFromBottom(
      viewScreenBounds.proportionOfHeight(0.03f));
  selRoomElevation_.setBounds(elevationToggleBounds);
}

void RoomViewScreen::elevationChangeCallback() {
  // Update the elevation in the AudioElementSpatialLayout repository to update
  // the elevation listener/calculator.
  AudioElementSpatialLayout::Elevation newElevation =
      static_cast<AudioElementSpatialLayout::Elevation>(
          selRoomElevation_.getToggled());
  AudioElementSpatialLayout toUpdate =
      audioElementSpatialLayoutRepository_->get();
  toUpdate.setElevation(newElevation);
  audioElementSpatialLayoutRepository_->update(toUpdate);
  // If the new elevation is 'Flat', set the starting height to +30.
  if (newElevation == AudioElementSpatialLayout::Elevation::kFlat) {
    parameterTree_->setZPosition(30);
  }

  // Update the room view with the new elevation pattern.
  room_->setElevationPattern(newElevation);
}

// On the same timer for rendering the tracks, add height data if the selected
// elevation is 'Flat'.
void RoomViewScreen::timerCallback() {
  // Update room view track data to be drawn.
  AudioElementUpdateData trackData;
  trackData.x = parameterTree_->getXPosition();
  trackData.y = parameterTree_->getYPosition();
  trackData.z = parameterTree_->getZPosition();
  // Loudness as an average of channels.
  const int kNumCh = audioElementSpatialLayoutRepository_->get()
                         .getChannelLayout()
                         .getNumChannels();
  std::vector<float> loudnesses;
  spkrData_.playbackLoudness.read(loudnesses);
  float avgLoudness = 0.f;
  for (const auto& loudness : loudnesses) {
    avgLoudness += std::max(loudness, -60.0f);
  }
  avgLoudness /= kNumCh;
  trackData.loudness = avgLoudness;
  room_->setTracks({trackData});

  // If the room view is set to 'Flat' elevation, notify the room view so it
  // knows what height to draw the pattern at.
  if (static_cast<Elevation>(selRoomElevation_.getToggled()) ==
      Elevation::kFlat) {
    room_->setFlatHeight(parameterTree_->getZPosition());
  }

  room_->repaint();
}

void RoomViewScreen::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  LOG_ANALYTICS(AudioElementPluginProcessor::instanceId_,
                "RoomViewScreen::updateSpeakerSetup" +
                    audioElementSpatialLayoutRepository_->get()
                        .getChannelLayout()
                        .toString()
                        .toStdString());

  if (property == AudioElementSpatialLayout::kLayout) {
    room_->setSpeakers(
        audioElementSpatialLayoutRepository_->get().getChannelLayout());
    room_->setDisplaySpeakers(true);
  }

  if (property == AudioElementSpatialLayout::kPanningEnabled) {
    selRoomElevation_.setVisible(
        audioElementSpatialLayoutRepository_->get().isPanningEnabled());
  }
}
