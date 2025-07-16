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

#include <vector>

#include "Coordinates.h"
#include "components/src/room_views/FaceLookup.h"
#include "components/src/room_views/SpeakerLookup.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_structures/src/AudioElementCommunication.h"
#include "data_structures/src/RepositoryCollection.h"
#include "data_structures/src/SpeakerMonitorData.h"

class PerspectiveRoomView : public juce::Component {
 public:
  PerspectiveRoomView(
      const std::vector<FaceLookup::Face>& faces,
      const Coordinates::Mat4& transformMat,
      const std::unordered_set<SpeakerLookup::SpeakerTag>& hiddenSpeakers,
      const juce::Image& figure, const SpeakerMonitorData& monitorData,
      RepositoryCollection repos);

  PerspectiveRoomView(
      const std::vector<FaceLookup::Face>& faces,
      const Coordinates::Mat4& transformMat,
      const std::unordered_set<SpeakerLookup::SpeakerTag>& hiddenSpeakers,
      const juce::Image& figure, const SpeakerMonitorData& monitorData);
  ~PerspectiveRoomView() = default;

  void setDisplaySpeakers(bool enable) { displaySpeakers_ = enable; }
  void setDisplayLabels(bool enable) { displayLabels_ = enable; }
  void setDisplayTracks(bool enable) { displayTracks_ = enable; }
  void setSpeakers(const Speakers::AudioElementSpeakerLayout layout) {
    speakers_ = SpeakerLookup::getRoomViewSpeakers(layout);
    recalculateStaticVertices_ = true;
  }
  void setTracks(const std::vector<AudioElementUpdateData>& tracks) {
    tracks_ = tracks;
  }

  // Derived classes can optionally override paint method if necessary.
  virtual void paint(juce::Graphics& g) override;
  virtual void resized() override { recalculateStaticVertices_ = true; }

 protected:
  // Container to hold transformed room data for convenient drawing`.
  struct DrawableFace {
    juce::Colour faceColour, gridlineColour;
    std::array<Coordinates::Point2D, 4> faceVertPts;
    std::array<std::pair<Coordinates::Point2D, Coordinates::Point2D>,
               FaceLookup::kNumGridLines * 2>
        gridlineVertPts;
  };
  // Container to hold transformed speaker data for convenient drawing.
  struct DrawableSpeaker {
    juce::Colour speakerColour;
    juce::String speakerLabel;
    SpeakerLookup::SpeakerTag tag;
    Coordinates::Point2D pos;
  };
  // Container to hold transformed track data for convenient drawing.
  struct DrawableTrack {
    DrawableTrack() : trackLoudness(-300.f) {}
    float trackLoudness;
    float sizeScale;
    juce::String trackLabel;
    Coordinates::Point2D pos;
  };

  virtual const float getTrackScaling(const Coordinates::Point4D pt) const = 0;
  virtual void drawFace(const std::array<Coordinates::Point2D, 4>& faceVerts,
                        const juce::Colour& c, juce::Graphics& g);
  virtual void drawTrack(const DrawableTrack& track, juce::Graphics& g);
  // Calculate window coordinates for vertices that do not change position.
  void transformStaticVertices();
  // Calculate window coordinates for vertices that do change position.
  void transformDynamicVertices();
  void drawGridlines(const DrawableFace& face, juce::Graphics& g);
  void drawSpeaker(const DrawableSpeaker& spkr, juce::Graphics& g);
  void drawLine(const Coordinates::Point2D& start,
                const Coordinates::Point2D& end, juce::Graphics& g,
                const float thickness = 2.f);
  juce::Colour getLoudnessColour(const float loudness) const;

  const Coordinates::Mat4 kTransformMat_;

  // Data visible to derived classes needing to draw tracks differently.
  std::vector<DrawableTrack> transformedTracks_;

 private:
  const float assignTrackLoudness(
      const AudioElementUpdateData& data,
      const MixPresentationSoloMute& mixPresSoloMute);

  const juce::Uuid getTrackAudioElementUuid(const std::array<char, 16>& uuid);
  void updateSpeakerColours();

  // Data sources. //
  const SpeakerMonitorData& monitorData_;
  const bool rendererPlugin_;  // if true the perspective room view is used
                               // in the renderer plugin.
  MixPresentationSoloMuteRepository* mixPresSMRepo_;
  ActiveMixRepository* activeMixRepo_;
  MultibaseAudioElementSpatialLayoutRepository* aeslRepo_;
  // Data inherent to the room view. //
  const std::vector<FaceLookup::Face> kFaces_;
  // Speakers that are not be drawn for the current room view.
  const std::unordered_set<SpeakerLookup::SpeakerTag> kHiddenSpeakers_;

  bool displaySpeakers_;
  bool displayTracks_;
  bool displayLabels_;

  // Speaker set to draw in the room view.
  std::vector<SpeakerLookup::RoomViewSpeaker> speakers_;
  std::vector<AudioElementUpdateData> tracks_;
  bool recalculateStaticVertices_ = true;
  std::vector<DrawableFace> transformedFaces_;
  std::vector<DrawableSpeaker> transformedSpeakers_;
  juce::ImageComponent imageComponent_;
};