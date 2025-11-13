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

#include "AmbisonicsVisualizer.h"

#include <cmath>
#include <queue>

#include "components/src/EclipsaColours.h"
#include "components/src/ambisonics_visualizers/ColourLegend.h"
#include "components/src/loudness_meter/LoudnessScale.h"

AmbisonicsVisualizer::AmbisonicsVisualizer(AmbisonicsData* ambisonicsData,
                                           const VisualizerView& view)
    : ambisonicsData_(ambisonicsData),
      view_(view),
      speakerPositions_(getSpeakerPositions(ambisonicsData)) {
  label_.setText(getViewText(view), juce::dontSendNotification);
  label_.setJustificationType(juce::Justification::centred);
  label_.setColour(juce::Label::textColourId, EclipsaColours::headingGrey);
  label_.setColour(juce::Label::backgroundColourId,
                   juce::Colours::transparentBlack);
  addAndMakeVisible(label_);
  startTimerHz(10);
}

AmbisonicsVisualizer::~AmbisonicsVisualizer() { setLookAndFeel(nullptr); }

void AmbisonicsVisualizer::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();

  // create a copy for reference later
  const auto visualizerBounds = bounds;

  // alocate the bottom 10% for the label
  auto labelBounds =
      bounds.removeFromBottom(visualizerBounds.proportionOfHeight(0.1f));
  // labelBounds.reduce(visualizerBounds.proportionOfWidth(0.1f), 0);
  label_.setBounds(labelBounds);

  const auto topComponentsBounds = bounds;
  // create some space for the carat image
  bounds.reduce(10, 10);
  // ensure there is an aspect ratio 1:1
  adjustDialAspectRatio(bounds);
  // translate the circle to be centre aligned with the label
  bounds.translate(label_.getBounds().getCentreX() - bounds.getCentreX(), 0);
  circleBounds_ = bounds;

  g.setColour(EclipsaColours::ambisonicsFillGrey);
  g.fillEllipse(bounds.toFloat());

  if (visualizerElements_.isEmpty()) {
    tesselateCircle(g, bounds);
  } else {
    repaintTesselatedCircle(g);
  }

  if (view_ != VisualizerView::kFront && view_ != VisualizerView::kRear) {
    if (caratTransform_.isIdentity()) {
      drawCarat(g);  // writes caratTransform_ on first call
    } else {
      g.drawImageTransformed(carat_, caratTransform_);  // use existing
                                                        // transform
    }
  }

  if (view_ != VisualizerView::kTop && view_ != VisualizerView::kBottom) {
    g.setColour(EclipsaColours::headingGrey);
    juce::Path notchPath;
    float radius = 0.98f * bounds.toFloat().getWidth() / 2;
    notchPath.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), radius,
                            radius, 0, juce::MathConstants<float>::pi / 64.f,
                            -1.f * juce::MathConstants<float>::pi / 64.f, true);
    g.strokePath(notchPath,
                 juce::PathStrokeType(1.25f, juce::PathStrokeType::curved,
                                      juce::PathStrokeType::rounded));
  } else if (view_ == VisualizerView::kTop) {
    g.setColour(EclipsaColours::headingGrey);
    drawCircle(g, bounds);
  } else {
    g.setColour(EclipsaColours::headingGrey.withAlpha(0.5f));
    drawCircle(g, bounds);
  }
}

void AmbisonicsVisualizer::drawCircle(juce::Graphics& g,
                                      juce::Rectangle<int>& bounds) {
  const int radius = 5;
  juce::Rectangle<int> circleBounds(bounds.getCentreX() - radius,
                                    bounds.getCentreY() - radius, radius * 2.f,
                                    radius * 2.f);

  g.fillEllipse(circleBounds.toFloat());
}

void AmbisonicsVisualizer::adjustDialAspectRatio(
    juce::Rectangle<int>& dialBounds) {
  if (dialBounds.getWidth() < dialBounds.getHeight()) {
    dialBounds.setHeight(dialBounds.getWidth());
  } else {
    dialBounds.setWidth(dialBounds.getHeight());
  }
}

void AmbisonicsVisualizer::drawCarat(juce::Graphics& g) {
  const float scale = 1.f;
  int heightOffset = carat_.getHeight() * scale / 2;
  int widthOffset = carat_.getWidth() * scale / 2;
  juce::Point<int> caratPoint;
  switch (view_) {
    case VisualizerView::kLeft:
      caratPoint = {circleBounds_.getX(),
                    circleBounds_.getCentreY() - heightOffset};
      break;
    case VisualizerView::kRight:
      caratPoint = {circleBounds_.getX() + circleBounds_.getWidth(),
                    circleBounds_.getCentreY() - heightOffset};
      break;
    case VisualizerView::kTop:
      caratPoint = {circleBounds_.getCentreX() - widthOffset,
                    circleBounds_.getY()};
      break;
    case VisualizerView::kBottom:
      caratPoint = {circleBounds_.getCentreX() + widthOffset,
                    circleBounds_.getBottom()};
      break;
  }

  juce::AffineTransform transform;
  transform = transform.scaled(scale, scale);
  transform = transform.rotated(getCaratRotation(view_));
  transform = transform.translated(caratPoint.getX(), caratPoint.getY());

  caratTransform_ = transform;  // save the transform

  g.drawImageTransformed(carat_, transform);
}

float AmbisonicsVisualizer::getCaratRotation(const VisualizerView& view) {
  float rotation;
  switch (view_) {
    case VisualizerView::kLeft:
      rotation = juce::MathConstants<float>::pi;
      break;
    case VisualizerView::kRight:
      rotation = 0.f;
      break;
    case VisualizerView::kTop:
      rotation = -1.f * juce::MathConstants<float>::halfPi;
      break;
    case VisualizerView::kBottom:
      rotation = juce::MathConstants<float>::halfPi;
      break;
  }
  return rotation;
}

juce::String AmbisonicsVisualizer::getViewText(const VisualizerView& view) {
  juce::String text;
  switch (view) {
    case VisualizerView::kLeft:
      text = "Left";
      break;
    case VisualizerView::kRight:
      text = "Right";
      break;
    case VisualizerView::kFront:
      text = "Front";
      break;
    case VisualizerView::kRear:
      text = "Rear";
      break;
    case VisualizerView::kTop:
      text = "Top";
      break;
    case VisualizerView::kBottom:
      text = "Bottom";
      break;
  }
  return text;
}

std::vector<AmbisonicsVisualizer::CartesianPoint3D>
AmbisonicsVisualizer::getSpeakerPositions(AmbisonicsData* ambisonicsData) {
  std::vector<CartesianPoint3D> speakerPositions(
      ambisonicsData->speakerAzimuths.size(), CartesianPoint3D(0, 0));
  for (int i = 0; i < ambisonicsData->speakerAzimuths.size(); i++) {
    speakerPositions[i] =
        CartesianPoint3D(ambisonicsData->speakerAzimuths[i],
                         ambisonicsData->speakerElevations[i]);
  }
  return speakerPositions;
}

void AmbisonicsVisualizer::tesselateCircle(juce::Graphics& g,
                                           const juce::Rectangle<int>& bounds) {
  std::vector<float> loudnessValues(ambisonicsData_->speakerElevations.size());
  const bool valuesReturned =
      ambisonicsData_->speakerLoudnesses.read(loudnessValues);
  int colorIndex = 0;
  const int radius = bounds.getWidth() / 2;
  const int centreX = bounds.getCentreX();
  const int centreY = bounds.getCentreY();
  const int numPoints = 20;
  Eigen::VectorXf thetas = Eigen::VectorXf::LinSpaced(
      numPoints, 0, 2 * juce::MathConstants<float>::pi);
  const Eigen::VectorXf radii =
      Eigen::VectorXf::LinSpaced(numPoints, 0, radius);

  // create pie segments for the first two radii
  const float pieSegmentRadius = radii[1];
  const float avg_radius =
      (pieSegmentRadius / 2) / radius;  // ensure the value is normalized
  // keep an element index to access memoized elements
  int elementIndex = 0;
  for (int j = 1; j < numPoints; j++) {  // iterate through the thetas
    juce::Path tesselatedPatch;
    const float theta = thetas[j];
    const float thetaPrev = thetas[j - 1];
    juce::Point<float> arcStart{
        centreX + pieSegmentRadius * std::sin(thetaPrev),
        centreY - pieSegmentRadius * std::cos(thetaPrev)};
    tesselatedPatch.startNewSubPath(centreX, centreY);
    juce::Point<int> pointA = tesselatedPatch.getCurrentPosition().toInt();
    tesselatedPatch.lineTo(arcStart);
    juce::Point<int> pointB = tesselatedPatch.getCurrentPosition().toInt();
    tesselatedPatch.addCentredArc(centreX, centreY, pieSegmentRadius,
                                  pieSegmentRadius, 0, thetaPrev, theta);
    juce::Point<int> pointC = tesselatedPatch.getCurrentPosition().toInt();
    tesselatedPatch.lineTo(centreX, centreY);
    juce::Point<int> pointD = tesselatedPatch.getCurrentPosition().toInt();
    tesselatedPatch.closeSubPath();

    // add r & theta to the 3D points
    const float avg_theta = (theta + thetaPrev) / 2;
    // write to the Visualizer Elements
    writeVisualizerElements(tesselatedPatch,
                            CartesianPoint3D(avg_radius, avg_theta, view_));
    if (valuesReturned) {
      float loudness =
          gaussianFilter(*visualizerElements_.getLast(), loudnessValues);
      juce::Colour colour = ColourLegend::assignColour(loudness);

      visualizerElements_.getLast()->prevColour_ = colour;
    }
    g.setColour(visualizerElements_.getLast()->prevColour_);
    g.fillPath(tesselatedPatch);
    g.strokePath(tesselatedPatch, juce::PathStrokeType(1.f));
  }

  // create the rest of the circle
  int thetaCount = numPoints;            // store the previous theta count
  for (int i = 2; i < numPoints; i++) {  // iterate through the radii
    const float outerRadius = radii[i];
    const float innerRadius = radii[i - 1];
    const float avg_radius = ((outerRadius + innerRadius) / 2) /
                             radius;  // ensure radius is normalized
    // try to preserve radial resolution
    // scale the number of thetas based on the ratio of the radii
    const int newThetaCount =
        std::ceil(thetaCount * outerRadius /
                  innerRadius);  // round up to the next integer
    thetaCount = newThetaCount;  // store the previous theta count
    thetas = Eigen::VectorXf::LinSpaced(newThetaCount, 0,
                                        2 * juce::MathConstants<float>::pi);
    for (int j = 1; j < newThetaCount; j++) {  // iterate through the thetas
      juce::Path tesselatedPatch;
      // get the equivalent x and y coordinates of the starting point
      // theta is 0 from the circles top centre
      juce::Point<float> innerArcStart = {
          centreX + innerRadius * std::sin(thetas[j - 1]),
          centreY - innerRadius * std::cos(thetas[j - 1])};

      tesselatedPatch.startNewSubPath(innerArcStart);

      // inner arc is drawn clockwise
      tesselatedPatch.addCentredArc(centreX, centreY, innerRadius, innerRadius,
                                    0, thetas[j - 1], thetas[j], true);

      juce::Point<float> innerArcEnd = tesselatedPatch.getCurrentPosition();
      // theta is 0 from the circles top centre
      juce::Point<float> outterArcStart = {
          centreX + outerRadius * std::sin(thetas[j]),
          centreY - outerRadius * std::cos(thetas[j])};

      tesselatedPatch.lineTo(outterArcStart);
      //  outer arc is drawn counter clockwise
      tesselatedPatch.addCentredArc(centreX, centreY, outerRadius, outerRadius,
                                    0, thetas[j], thetas[j - 1]);

      juce::Point<float> outerArcEnd = tesselatedPatch.getCurrentPosition();

      tesselatedPatch.lineTo(innerArcStart);
      tesselatedPatch.closeSubPath();

      const float avg_theta = (thetas[j] + thetas[j - 1]) / 2;
      // write to the Visualizer Elements
      writeVisualizerElements(tesselatedPatch,
                              CartesianPoint3D(avg_radius, avg_theta, view_));
      if (valuesReturned) {
        float loudness =
            gaussianFilter(*visualizerElements_.getLast(), loudnessValues);
        juce::Colour colour = ColourLegend::assignColour(loudness);

        visualizerElements_.getLast()->prevColour_ = colour;
      }
      g.setColour(visualizerElements_.getLast()->prevColour_);
      g.fillPath(tesselatedPatch);
      g.strokePath(tesselatedPatch, juce::PathStrokeType(1.f));
    }
  }
}

void AmbisonicsVisualizer::timerCallback() { repaint(); }

void AmbisonicsVisualizer::repaintTesselatedCircle(juce::Graphics& g) {
  std::vector<float> loudnessValues(ambisonicsData_->speakerElevations.size());
  const bool readValues =
      ambisonicsData_->speakerLoudnesses.read(loudnessValues);
  // couldn't read the values
  // repaint the previous colours
  if (!readValues) {
    for (auto& element : visualizerElements_) {
      g.setColour(element->prevColour_);
      g.fillPath(element->tesselationPatch_);
      g.strokePath(element->tesselationPatch_, juce::PathStrokeType(1.f));
    }
  } else {      // able to update the loudness values, paint the new colours
    int i = 0;  // track element index for debugging
    for (auto& element : visualizerElements_) {
      float loudness = gaussianFilter(*element, loudnessValues);
      juce::Colour colour = ColourLegend::assignColour(loudness);
      g.setColour(colour);
      g.fillPath(element->tesselationPatch_);
      g.strokePath(element->tesselationPatch_, juce::PathStrokeType(1.f));
      element->prevColour_ = colour;
      i++;
    }
  }
}

float AmbisonicsVisualizer::gaussianFilter(
    const VisualizerElement& element,
    const std::vector<float>& loudnessValues) {
  float numerator = 0.f;
  float denominator = 0.f;
  // create a local copy to iterate over
  std::priority_queue<std::pair<float, int>> closestSpeakers =
      element.closestSpeakers_;
  int index = 0;
  // closestSpeakers will always have a max size of kNearestSpeakers_
  // element.gaussianFilterWeights_ will always have a size of kNearestSpeakers_
  while (!closestSpeakers.empty() && index < kNearestSpeakers_) {
    const auto speaker = closestSpeakers.top();
    const float distance = speaker.first;
    const float loudness = loudnessValues[speaker.second];
    const float weight = element.gaussianFilterWeights_[index];
    numerator += loudness * weight;
    index++;
    closestSpeakers.pop();
  }
  return {numerator / element.denominator_};
}

void AmbisonicsVisualizer::writeVisualizerElements(
    const juce::Path& path, const CartesianPoint3D& point) {
  // calculate the distance from point i to point j, keeping the k nearest
  // speakers
  std::priority_queue<std::pair<float, int>> closestSpeakers;
  for (int j = 0; j < ambisonicsData_->speakerAzimuths.size(); j++) {
    float geoDesicDistance =
        CartesianPoint3D::geoDesicDistance(point, speakerPositions_[j]);
    if (closestSpeakers.size() < kNearestSpeakers_) {
      closestSpeakers.push({geoDesicDistance, j});
    } else if (closestSpeakers.top().first > geoDesicDistance) {
      closestSpeakers.pop();
      closestSpeakers.push({geoDesicDistance, j});
    }
  }
  visualizerElements_.add(
      std::make_unique<VisualizerElement>(path, point, closestSpeakers));
}

AmbisonicsVisualizer::CartesianPoint3D::CartesianPoint3D(const float& azimuth,
                                                         const float& elevation)
    : x(cos(elevation) * cos(azimuth)),
      y(cos(elevation) * sin(azimuth)),
      z(sin(elevation)) {}

AmbisonicsVisualizer::CartesianPoint3D::CartesianPoint3D(
    const float& r, const float& theta, const VisualizerView& view) {
  calculateSurfacePosition(polarToCartesian(r, theta), view);
}

std::pair<float, float>
AmbisonicsVisualizer::CartesianPoint3D::polarToCartesian(const float& r,
                                                         const float& theta) {
  return {r * sin(theta), r * cos(theta)};
}

void AmbisonicsVisualizer::CartesianPoint3D::calculateSurfacePosition(
    const std::pair<float, float>& point, const VisualizerView& view) {
  switch (view) {
    // for the left & right view, we must solve for y
    // ensures the point is real
    case VisualizerView::kLeft:
      x = -1.f * point.first;
      z = point.second;
      y = unitSphere(x, z);
      assert(!std::isnan(y));
      break;
    case VisualizerView::kRight:
      x = point.first;
      z = point.second;
      y = -1.f * unitSphere(x, z);
      assert(!std::isnan(y));
      break;
    case VisualizerView::kFront:
      y = point.first;
      z = point.second;
      x = unitSphere(y, z);
      assert(!std::isnan(x));
      break;
    case VisualizerView::kRear:
      y = -1.f * point.first;
      z = point.second;
      x = -1.f * unitSphere(y, z);
      assert(!std::isnan(x));
      break;
    case VisualizerView::kTop:
      x = point.second;
      y = -1.f * point.first;
      z = unitSphere(x, y);
      assert(!std::isnan(z));
      break;
    case VisualizerView::kBottom:
      x = -1.f * point.second;
      y = -1.f * point.first;
      z = -1.f * unitSphere(x, y);
      assert(!std::isnan(z));
      break;
  }
}

float AmbisonicsVisualizer::CartesianPoint3D::unitSphere(const float& x,
                                                         const float& y) {
  return sqrt(1 - x * x - y * y);
}

float AmbisonicsVisualizer::CartesianPoint3D::geoDesicDistance(
    const CartesianPoint3D& vec1, const CartesianPoint3D& vec2) {
  return acos(dotProduct(vec1, vec2));
}

float AmbisonicsVisualizer::CartesianPoint3D::dotProduct(
    const CartesianPoint3D& vec1, const CartesianPoint3D& vec2) {
  return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}

AmbisonicsVisualizer::VisualizerElement::VisualizerElement(
    const juce::Path& tesselatedPatch, const CartesianPoint3D& position,
    const std::priority_queue<std::pair<float, int>>& closestSpeakers)
    : tesselationPatch_(tesselatedPatch),
      position_(position),
      closestSpeakers_(closestSpeakers),
      gaussianFilterWeights_(calculateWeights()),
      denominator_(calculateDenominator()) {}

std::vector<float> AmbisonicsVisualizer::VisualizerElement::calculateWeights() {
  std::vector<float> weights;
  std::priority_queue<std::pair<float, int>> speakerCopy =
      closestSpeakers_;  // create a local copy to iterate over
  // calculate the weights for the k nearest speakers
  while (!speakerCopy.empty()) {
    const auto speaker = speakerCopy.top();
    const float distance = speaker.first;
    const float weight =
        std::exp(-1.f * std::pow(distance, 2) / twoSigmaSquared_);
    weights.push_back(weight);
    speakerCopy.pop();
  }
  return weights;
}

float AmbisonicsVisualizer::VisualizerElement::calculateDenominator() {
  float denominator = 0.f;
  for (auto& weight : gaussianFilterWeights_) {
    denominator += weight;
  }
  return denominator;
}
