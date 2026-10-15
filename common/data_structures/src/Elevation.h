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
#include <juce_gui_extra/juce_gui_extra.h>

#include <cmath>

#include "components/src/room_views/Coordinates.h"
#include "data_repository/implementation/AudioElementSpatialLayoutRepository.h"
#include "data_structures/src/AudioElementParameterTree.h"
#include "data_structures/src/AudioElementSpatialLayout.h"

using Elevation = AudioElementSpatialLayout::Elevation;

class ElevationListener : public juce::AudioProcessorValueTreeState::Listener,
                          juce::ValueTree::Listener {
  Elevation currentElevation_;
  AudioElementParameterTree* parameterTree_;
  AudioElementSpatialLayoutRepository* AudioElementSpatialLayoutRepository_;
  juce::SpinLock elevationLock_;
  float prevX_, prevY_, prevZ_;

 public:
  ElevationListener() { prevX_ = prevY_ = prevZ_ = 0.0f; };

  void setListeners(AudioElementParameterTree* parameterTree,
                    AudioElementSpatialLayoutRepository*
                        AudioElementSpatialLayoutRepository) {
    parameterTree_ = parameterTree;
    AudioElementSpatialLayoutRepository_ = AudioElementSpatialLayoutRepository;
    AudioElementSpatialLayoutRepository_->registerListener(this);
    parameterTree_->addXPositionListener(this);
    parameterTree_->addYPositionListener(this);
    updateElevation();
  }

  void updateElevation() {
    elevationLock_.enter();
    currentElevation_ =
        AudioElementSpatialLayoutRepository_->get().getElevation();
    // Check if the z position needs to be recomputed for this elevation
    elevationLock_.exit();
    parameterChanged("", 0.0f);
  }

  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override {
    updateElevation();
  }
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override {
    updateElevation();
  }
  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override {
    updateElevation();
  }
  void valueTreeChildOrderChanged(
      juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex,
      int newIndex) override {
    updateElevation();
  }
  void valueTreeParentChanged(
      juce::ValueTree& treeWhoseParentHasChanged) override {
    updateElevation();
  }

  void parameterChanged(const juce::String& parameterID,
                        float newValue) override {
    int currentZ = parameterTree_->getZPosition();
    int newZ = currentZ;
    elevationLock_.enter();
    if (currentElevation_ == Elevation::kTent) {
      Coordinates::Point3D pt = {
          (float)parameterTree_->getXPosition() / 50.f,
          (float)parameterTree_->getYPosition() / 50.f,
          (float)parameterTree_->getZPosition() / 50.f,
      };
      newZ = getTentElevationPt(pt).a[1] * 50.f;
    }

    if (currentElevation_ == Elevation::kArch) {
      Coordinates::Point3D pt = {
          (float)parameterTree_->getXPosition() / 50.f,
          (float)parameterTree_->getYPosition() / 50.f,
          (float)parameterTree_->getZPosition() / 50.f,
      };
      newZ = getArchElevationPt(pt).a[1] * 50.f;
    }

    if (currentElevation_ == Elevation::kDome) {
      Coordinates::Point3D pt = {
          (float)parameterTree_->getXPosition() / 50.f,
          (float)parameterTree_->getYPosition() / 50.f,
          (float)parameterTree_->getZPosition() / 50.f,
      };
      Coordinates::Point3D prevPt = {prevX_, prevY_, prevZ_};
      prevPt.a[0] /= 50.f;

      // Returns a clamped (x,y,z) point.
      Coordinates::Point3D domePt = getDomeElevationPtClamped(pt, prevPt) * 50;
      parameterTree_->removeXPositionListener(this);
      parameterTree_->removeYPositionListener(this);
      parameterTree_->setXPosition(std::round(domePt.a[0]));
      parameterTree_->setYPosition(std::round(domePt.a[2]));
      parameterTree_->addXPositionListener(this);
      parameterTree_->addYPositionListener(this);
      newZ = domePt.a[1];
    }

    if (currentElevation_ == Elevation::kCurve) {
      Coordinates::Point3D pt = {
          (float)parameterTree_->getXPosition() / 50.f,
          -(float)parameterTree_->getYPosition() / 50.f,
          (float)parameterTree_->getZPosition() / 50.f,
      };
      newZ = std::ceil(getCurveElevationPt(pt).a[1] * 50.f);
    }

    elevationLock_.exit();
    if (newZ != currentZ) {
      parameterTree_->setZPosition(newZ);
    }

    // Stash the previous values so that we can clamp the correct coordinate
    // next iteration.
    prevX_ = parameterTree_->getXPosition();
    prevY_ = parameterTree_->getYPosition();
    prevZ_ = parameterTree_->getZPosition();
  }

  // Given an (x,y,z) coordinate, return an (x,y,z) coordinate for each given
  // elevation pattern.
  // NOTE: Y and Z axes are swapped between the AudioElementSpatialLayout UI
  // coordinate system and the backend graphics calculation coordinate system.
  // In terms of the graphics coordinate system, we expect to be given a (x,z,y)
  // coordinate and return a proper (x,y,z) coordinate.
  static Coordinates::Point3D getTentElevationPt(
      const Coordinates::Point3D pt) {
    // Return a value between 1 and -1, decreased
    // by a percentage of the y coordinate to create the tent shape
    float height = 1 - std::abs(pt.a[1]) * 2;
    return {pt.a[0], height, pt.a[1]};
  }

  static Coordinates::Point3D getArchElevationPt(
      const Coordinates::Point3D pt) {
    // Parabola passing through (-1,-1), (1,-1) and (0, 1)
    float height = (-1.0f * pt.a[1] * pt.a[1] / 0.5f) + 1;
    return {pt.a[0], height, pt.a[1]};
  }

  static Coordinates::Point3D getDomeElevationPtClamped(
      const Coordinates::Point3D pt, const Coordinates::Point3D prevPt) {
    float distance = std::sqrt(pt.a[0] * pt.a[0] + pt.a[1] * pt.a[1]);

    double x, y;
    // Clamp the distance if necessary.
    if (distance > 1.f) {
      // If X is the parameter actively being changed, clamp Y.
      if (pt.a[0] != prevPt.a[0]) {
        x = pt.a[0];
        y = std::copysign(std::sqrt(1.f - x * x), pt.a[1]);
      }
      // If Y is the parameter actively being changed, clamp X.
      else {
        y = pt.a[1];
        x = std::copysign(std::sqrt(1.f - y * y), pt.a[0]);
      }
    } else {
      x = pt.a[0];
      y = pt.a[1];
    }

    // Sphere equation.
    double x2 = x * x;
    double y2 = y * y;
    if (std::sqrt(x2 + y2) > 1.0 - kDomeRimBand) {
      return {(float)x, -1.f, (float)y};
    }
    double height = 2 * std::sqrt(std::max(0.0, 1.0 - (x2 + y2))) - 1.f;
    return {(float)x, (float)height, (float)y};
  }

  // Integer x and y almost never land exactly on the dome's rim, and the sphere
  // is near vertical there, so positions within one step of the rim are held
  // at floor height rather than up to 14 steps above it.
  static constexpr double kDomeRimBand = 1.0 / Coordinates::kPositionExtent;

  static Coordinates::Point3D getCurveElevationPt(
      const Coordinates::Point3D pt) {
    const int kAmp = 272;
    const float kOffs = 1.11f;
    const float kScale = 0.336f;
    const float kShift = 0.946f;
    // The curve is logarithmic. Shifts, offsets, and scalings were tweaked with
    // the constant editor.
    float height = kScale * std::log(kAmp * (pt.a[1] + kShift)) - kOffs;
    height = std::max(-1.f, height);
    return {pt.a[0], height, pt.a[2]};
  }
};