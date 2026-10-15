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
#include <vector>

#include "PerspectiveRoomView.h"
#include "components/src/room_views/HeightIndicator.h"
#include "components/src/room_views/ListenerHead.h"
#include "components/src/room_views/PannerInput.h"
#include "data_structures/src/AudioElementParameterTree.h"
#include "data_structures/src/AudioElementSpatialLayout.h"
#include "data_structures/src/Elevation.h"
#include "data_structures/src/RepositoryCollection.h"

class TopView : public PerspectiveRoomView {
 public:
  TopView(const SpeakerMonitorData& monitorData, RepositoryCollection repos);
  const float getTrackScaling(const Coordinates::Point4D pt) const override;
};

class SideView : public PerspectiveRoomView {
 public:
  SideView(const SpeakerMonitorData& monitorData, RepositoryCollection repos);
  const float getTrackScaling(const Coordinates::Point4D pt) const override;
};

class RearView : public PerspectiveRoomView {
 public:
  RearView(const SpeakerMonitorData& monitorData, RepositoryCollection repos);
  const float getTrackScaling(const Coordinates::Point4D pt) const override;
};

class IsoView : public PerspectiveRoomView {
 public:
  IsoView(const SpeakerMonitorData& monitorData, RepositoryCollection repos);
  void drawFace(const std::array<Coordinates::Point2D, 4>& faceVerts,
                const juce::Colour& c, juce::Graphics& g) override;
  const float getTrackScaling(const Coordinates::Point4D pt) const override;
};

// The audio element plugin's panner: a top-down (plan) projection of the room,
// with left/right on the horizontal screen axis and front/back on the
// vertical. Replaces the rear projection the panner used previously.
class AudioElementPluginTopView : public PerspectiveRoomView {
 public:
  AudioElementPluginTopView(const SpeakerMonitorData& monitorData);
  void paint(juce::Graphics& g) override;
  const float getTrackScaling(const Coordinates::Point4D pt) const override;
  void drawTrack(const DrawableTrack& track, juce::Graphics& g) override;
  void drawFace(const std::array<Coordinates::Point2D, 4>& faceVerts,
                const juce::Colour& c, juce::Graphics& g) override;
  void setElevationPattern(AudioElementSpatialLayout::Elevation elevation);
  void setFlatHeight(float height) {
    currentFlatHeight_ = Coordinates::toRoomNdc(0.f, 0.f, height).a[1];
  }

  // Input handling stays inert until this is set.
  void setParameterTree(AudioElementParameterTree* tree) {
    parameterTree_ = tree;
  }

  // A press on a drawn speaker snaps the source to it; a press on a speaker or
  // on the source starts a drag in left/right and front/back.
  void mouseDown(const juce::MouseEvent& event) override;
  void mouseDrag(const juce::MouseEvent& event) override;
  void mouseUp(const juce::MouseEvent& event) override;

  // The wheel adjusts height, the axis the mouse otherwise cannot reach: the
  // drag owns left/right and front/back, and the height indicator is
  // display-only.
  void mouseWheelMove(const juce::MouseEvent& event,
                      const juce::MouseWheelDetails& wheel) override;
  // Arrow keys nudge the source one step. Several hosts keep arrow keys for
  // their own transport and never deliver them here, in which case nudge is
  // simply absent for that host and drag and wheel are unaffected.
  bool keyPressed(const juce::KeyPress& key) override;

 private:
  // Shared by paint and the drag so both use one mapping.
  Coordinates::WindowData currentWindow() const;
  bool sourceMarkerContains(const juce::Point<float>& windowPoint) const;
  // An index into speakers_, or PannerInput::kNoSpeaker.
  int speakerIndexAt(const juce::Point<float>& windowPoint) const;
  void snapToSpeaker(int speakerIndex);
  // Gesture bracketing needs the parameter itself. The tree's setters go
  // through getParameterAsValue and emit no gesture markers.
  juce::RangedAudioParameter* positionParameter(
      const juce::String& parameterName) const;
  void writeDragPosition(const juce::Point<float>& windowPoint);
  // One discrete step on one axis, gesture-bracketed on its own.
  void nudgeAxis(PannerInput::Axis axis, int steps);
  float elevationHeightAt(float leftRight, float frontBack) const;
  Coordinates::Point4D indicatorPosition(
      const DrawableTrack& source, const Coordinates::WindowData& window) const;
  Coordinates::Point4D sourceHeightPosition(
      const Coordinates::Point4D& sourceNdc) const;
  bool elevationClampsTheSource() const;
  bool elevationVariesAcrossLeftRight() const;
  bool elevationPositionsInPlan() const;
  Coordinates::Point4D trackDrawPosition(
      const Coordinates::Point4D& ndcPos) const override;
  void paintListenerHead(const Coordinates::WindowData& window,
                         juce::Graphics& g);
  void paintIndicatorRuns(const Coordinates::WindowData& window,
                          const std::vector<HeightIndicator::Segment>& runs,
                          float thickness, juce::Graphics& g);
  void paintFlatElevation(const Coordinates::WindowData& window,
                          juce::Graphics& g);
  void paintTentElevation(const Coordinates::WindowData& window,
                          juce::Graphics& g);
  void paintArchElevation(const Coordinates::WindowData& window,
                          juce::Graphics& g);
  void paintDomeElevation(const Coordinates::WindowData& window,
                          juce::Graphics& g);
  void paintCurveElevation(const Coordinates::WindowData& window,
                           juce::Graphics& g);

 private:
  // Initialized here, not left to the constructor: paint() reads both on the
  // first repaint, which happens before RoomViewScreen calls
  // setElevationPattern or setFlatHeight.
  AudioElementSpatialLayout::Elevation currentElevation_ =
      AudioElementSpatialLayout::Elevation::kNone;
  float currentFlatHeight_ = 0.f;

  AudioElementParameterTree* parameterTree_ = nullptr;
  bool draggingSource_ = false;  // true between press and release
  // A speaker press holds the snapped position until the pointer has moved
  // past JUCE's drag threshold.
  bool holdingSnap_ = false;
  // Sub-notch wheel travel, carried between events so a trackpad's stream of
  // small deltas adds up rather than being discarded.
  float wheelAccumulator_ = 0.f;
};
