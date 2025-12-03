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
#include <queue>

#include "../EclipsaColours.h"
#include "ColourLegend.h"
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

  struct VisualizerElement {
    VisualizerElement(
        const juce::Path& tesselatedPatch, const CartesianPoint3D& position,
        const std::priority_queue<std::pair<float, int>>& closestSpeakers);

    const juce::Path tesselationPatch_;
    const CartesianPoint3D position_;
    const std::priority_queue<std::pair<float, int>> closestSpeakers_;
    const float twoSigmaSquared_ = 2.f * 0.25f * 0.25f;  // sigma = 0.25
    const std::vector<float> gaussianFilterWeights_;
    const float denominator_;
    juce::Colour prevColour_ = EclipsaColours::inactiveGrey;

   private:
    std::vector<float> calculateWeights();
    float calculateDenominator();
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

  // called in initializer list
  float getCaratRotation(const VisualizerView& view);
  juce::String getViewText(const VisualizerView& view);

  void writeVisualizerElements(const juce::Path& path,
                               const CartesianPoint3D& point);
  void tesselateCircle(juce::Graphics& g, const juce::Rectangle<int>& bounds);
  void repaintTesselatedCircle(juce::Graphics& g);

  // returns loudness, using a gaussian filter to smooth the values across the
  // kNearestSpeakers_
  float gaussianFilter(const VisualizerElement& element,
                       const std::vector<float>& loudnessValues);

  std::vector<CartesianPoint3D> getSpeakerPositions(
      AmbisonicsData* ambisonicsData);

  AmbisonicsData* ambisonicsData_;
  VisualizerView view_;
  juce::OwnedArray<VisualizerElement> visualizerElements_;
  juce::AffineTransform caratTransform_;

  const std::vector<CartesianPoint3D> speakerPositions_;
  const juce::Image carat_ = IconStore::getInstance().getCaratIcon();
  const int kNearestSpeakers_ = 8;  // number of nearest speakers to display

  juce::Label label_;                  // label holds the position text
  juce::Image image_;                  // image holds the carat image
  juce::Point<int> caratPointOffset_;  // carat point
  juce::Rectangle<int> circleBounds_;  // circle bounds
};