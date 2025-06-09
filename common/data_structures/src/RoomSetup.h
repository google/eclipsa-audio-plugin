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
#include <juce_audio_basics/juce_audio_basics.h>

#include "../src/RepositoryItem.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

struct RoomLayout {
 private:
  Speakers::AudioElementSpeakerLayout roomSpeakerLayout;
  juce::String description;

 public:
  RoomLayout(Speakers::AudioElementSpeakerLayout layout, std::string desc)
      : roomSpeakerLayout(layout), description(desc) {}

  Speakers::AudioElementSpeakerLayout getRoomSpeakerLayout() const {
    return roomSpeakerLayout;
  }

  juce::String getDescription() const { return description; }

  bool operator==(const RoomLayout& other) const {
    return this->roomSpeakerLayout == other.roomSpeakerLayout;
  }
};

static const std::array<RoomLayout, 9> speakerLayoutConfigurationOptions = {
    RoomLayout(Speakers::kStereo, "Stereo"),
    RoomLayout(Speakers::k3Point1Point2, "3.1.2"),
    RoomLayout(Speakers::k5Point1, "5.1"),
    RoomLayout(Speakers::k5Point1Point2, "5.1.2"),
    RoomLayout(Speakers::k5Point1Point4, "5.1.4"),
    RoomLayout(Speakers::k7Point1, "7.1"),
    RoomLayout(Speakers::k7Point1Point2, "7.1.2"),
    RoomLayout(Speakers::k7Point1Point4, "7.1.4"),
    RoomLayout(Speakers::kBinaural, "Binaural")};

static RoomLayout fetchLayoutFromDescription(const juce::String& desc) {
  for (const auto& layout : speakerLayoutConfigurationOptions) {
    if (layout.getDescription() == desc) {
      return layout;
    }
  }
  return speakerLayoutConfigurationOptions[0];
}

class RoomSetup final : public RepositoryItemBase {
 public:
  RoomSetup();
  RoomSetup(RoomLayout layout);
  RoomSetup(const RoomLayout layout, const bool drawSpeakers,
            const bool drawSpeakerLabels, const bool drawTracks,
            const juce::String& currRoomView,
            const bool dawWarningDismissed = false);

  virtual juce::ValueTree toValueTree() const override;
  static RoomSetup fromTree(const juce::ValueTree tree);

  RoomLayout getSpeakerLayout() const { return speakerLayout_; }
  void setSpeakerLayout(const RoomLayout& layout) { speakerLayout_ = layout; }

  bool getDrawSpeakers() const { return drawSpeakers_; }
  void setDrawSpeakers(const bool drawSpeakers) {
    drawSpeakers_ = drawSpeakers;
  }

  bool getDrawSpeakerLabels() const { return drawSpeakerLabels_; }
  void setDrawSpeakerLabels(const bool drawSpeakerLabels) {
    drawSpeakerLabels_ = drawSpeakerLabels;
  }

  bool getDrawTracks() const { return drawTracks_; }
  void setDrawTracks(const bool drawTracks) { drawTracks_ = drawTracks; }

  juce::String getCurrentRoomView() const { return currentRoomView_; }
  void setCurrentRoomView(const juce::String& currRoomView) {
    currentRoomView_ = currRoomView;
  }

  bool getDawWarningDismissed() const { return dawWarningDismissed_; }
  void setDawWarningDismissed(const bool dismissed) {
    dawWarningDismissed_ = dismissed;
  }

  inline static const juce::Identifier kTreeType{"room_setup"};
  inline static const juce::Identifier kSpeakerLayout{"speaker_layout"};
  inline static const juce::Identifier kDrawSpeakers{"draw_speakers"};
  inline static const juce::Identifier kDrawSpeakerLabels{"draw_spkr_labels"};
  inline static const juce::Identifier kDrawTracks{"draw_tracks"};
  inline static const juce::Identifier kCurrRoomView{"current_view"};
  inline static const juce::Identifier kDawWarningDismissed{
      "daw_warning_dismissed"};

 private:
  RoomLayout speakerLayout_ = speakerLayoutConfigurationOptions[0];
  bool drawSpeakers_ = false, drawSpeakerLabels_ = false, drawTracks_ = false;
  bool dawWarningDismissed_ = false;
  juce::String currentRoomView_ = "Iso";
};