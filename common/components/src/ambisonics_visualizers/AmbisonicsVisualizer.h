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

#include <Eigen/Dense>
#include <optional>

#include "data_structures/src/AmbisonicsData.h"

class AmbisonicsVisualizer : public juce::Component, public juce::Timer {
 public:
  enum class VisualizerView {
    kLeft = 0,
    kRight = 1,
    kFront = 2,
    kRear = 3,
    kTop = 4,
    kBottom = 5
  };
  // contains the x, y, z coordinates of a point on a unit sphere's surface
  class CartesianPoint3D {
   public:
    // converts a spherical coordinate on a unit sphere to cartesian coordinates
    CartesianPoint3D(const float& azimuth, const float& elevation);
    // converts a point in polar coordinates, on a 2D circle, to cartesian
    // coordinates theta is measured from the top of the circle, following JUCE
    // convention r must be normalized to the circle's radius
    CartesianPoint3D(const float& r, const float& theta,
                     const VisualizerView& view);

    // calculate the geodesic distance between two points on a unit sphere
    static float geoDesicDistance(const CartesianPoint3D& vec1,
                                  const CartesianPoint3D& vec2);

    float x;
    float y;
    float z;

   private:
    // convert 2D polar to 2D cartesian
    // theta is measured from the top of the circle, following JUCE convention
    // r must be normalized to the circle's radius
    std::pair<float, float> polarToCartesian(const float& r,
                                             const float& theta);

    // convert 2D cartesian to 3D Cartesian
    // depends on view
    // uses the equation of a unit sphere
    void calculateSurfacePosition(const std::pair<float, float>& point,
                                  const VisualizerView& view);

    // returns the 3rd coordinates on the surface of a unit sphere
    float unitSphere(const float& x, const float& y);

    // perform a dot product between two vectors
    static float dotProduct(const CartesianPoint3D& vec1,
                            const CartesianPoint3D& vec2);
  };

  AmbisonicsVisualizer(AmbisonicsData* ambisonicsData,
                       const VisualizerView& view);

  ~AmbisonicsVisualizer();

  void paint(juce::Graphics& g) override;

  juce::Point<int> upperCirclePoint() const {
    return juce::Point<int>(circleBounds_.getCentreX(), circleBounds_.getY());
  }

  juce::Point<int> upperLabelPoint() const {
    return juce::Point<int>(circleBounds_.getCentreX(),
                            label_.getBounds().getY());
  }

  juce::Point<int> lowerLabelPoint() const {
    return juce::Point<int>(circleBounds_.getCentreX(),
                            label_.getBounds().getBottom());
  }

  void timerCallback() override;

 private:
  void adjustDialAspectRatio(juce::Rectangle<int>& dialBounds);
  void drawCircle(juce::Graphics& g, juce::Rectangle<int>& bounds);
  void drawCarat(juce::Graphics& g);
  void drawHeatmap(juce::Graphics& g, const juce::Rectangle<int>& bounds);
  void updateSmoothedLoudnesses(const std::vector<float>& currentLoudnesses);

  std::optional<std::pair<float, float>> projectSpeakerToView(
      const CartesianPoint3D& speaker3D, const VisualizerView& view,
      float centreX, float centreY, float radius);

  // called in initializer list
  float getCaratRotation(const VisualizerView& view);
  juce::String getViewText(const VisualizerView& view);

  std::vector<CartesianPoint3D> getSpeakerPositions(
      AmbisonicsData* ambisonicsData);

  AmbisonicsData* ambisonicsData_;
  VisualizerView view_;
  juce::AffineTransform caratTransform_;

  const std::vector<CartesianPoint3D> speakerPositions_;

  // Smoothed loudness values for speakers
  std::vector<float> smoothedLoudnesses_;
  static constexpr int kRefreshRate_ = 30;
  const juce::Image carat_ = IconStore::getInstance().getCaratIcon();

  juce::Label label_;                  // label holds the position text
  juce::Image image_;                  // image holds the carat image
  juce::Point<int> caratPointOffset_;  // carat point
  juce::Rectangle<int> circleBounds_;  // circle bounds
};