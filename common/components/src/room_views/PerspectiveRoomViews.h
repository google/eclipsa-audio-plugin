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
#include "PerspectiveRoomView.h"
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

class AudioElementPluginRearView : public PerspectiveRoomView {
 public:
  AudioElementPluginRearView(const SpeakerMonitorData& monitorData);
  void paint(juce::Graphics& g) override;
  const float getTrackScaling(const Coordinates::Point4D pt) const override;
  void drawTrack(const DrawableTrack& track, juce::Graphics& g) override;
  void setElevationPattern(AudioElementSpatialLayout::Elevation elevation);
  void setFlatHeight(float height) { currentFlatHeight_ = height / 50.f; }

 private:
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
  AudioElementSpatialLayout::Elevation currentElevation_;
  float currentFlatHeight_;
};