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

#include "components/src/EclipsaColours.h"
#include "components/src/ambisonics_visualizers/ColourLegend.h"

AmbisonicsVisualizer::AmbisonicsVisualizer(AmbisonicsData* ambisonicsData,
                                           const VisualizerView& view)
    : ambisonicsData_(ambisonicsData),
      view_(view),
      speakerPositions_(getSpeakerPositions(ambisonicsData)),
      smoothedLoudnesses_(ambisonicsData->speakerAzimuths.size(), -100.0f) {
  label_.setText(getViewText(view), juce::dontSendNotification);
  label_.setJustificationType(juce::Justification::centred);
  label_.setColour(juce::Label::textColourId, EclipsaColours::headingGrey);
  label_.setColour(juce::Label::backgroundColourId,
                   juce::Colours::transparentBlack);
  addAndMakeVisible(label_);
  startTimerHz(kRefreshRate_);
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

  // Draw heatmap
  drawHeatmap(g, bounds);

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

void AmbisonicsVisualizer::timerCallback() { repaint(); }

void AmbisonicsVisualizer::drawHeatmap(juce::Graphics& g,
                                       const juce::Rectangle<int>& bounds) {
  std::vector<float> loudnessValues(ambisonicsData_->speakerElevations.size());
  ambisonicsData_->speakerLoudnesses.read(loudnessValues);

  // Update smoothed loudnesses with current values
  updateSmoothedLoudnesses(loudnessValues);

  const float kRadius = bounds.getWidth() / 2.0f;
  const float kCentreX = bounds.getCentreX();
  const float kCentreY = bounds.getCentreY();

  // Create a clipping region for the circle
  juce::Path clipPath;
  clipPath.addEllipse(bounds.toFloat());
  g.saveState();
  g.reduceClipRegion(clipPath);

  // Draw a radial gradient for each speaker
  for (int i = 0; i < ambisonicsData_->speakerAzimuths.size(); i++) {
    // Use smoothed loudness instead of current value
    const float kLoudness = smoothedLoudnesses_[i];

    // Skip silent speakers
    const float kSilenceThreshold = -60.0f;
    if (kLoudness < kSilenceThreshold) {
      continue;
    }

    // Get speaker position in 3D
    const CartesianPoint3D& kSpeakerPos = speakerPositions_[i];

    // Project speaker position onto the 2D circle view
    const auto kProjectedPoint =
        projectSpeakerToView(kSpeakerPos, view_, kCentreX, kCentreY, kRadius);

    // Skip if speaker is on the back hemisphere (not visible in this view)
    if (!kProjectedPoint.has_value()) continue;

    const auto [kSpeakerX, kSpeakerY] = kProjectedPoint.value();

    // Map loudness to color
    juce::Colour colour = ColourLegend::assignColour(kLoudness);

    // Create radial gradient from speaker position
    // Gradient radius based on loudness (louder = larger influence area)
    const float kGradientRadMultiplier = 0.8f;
    const float kMinGradientScale = 1.0f;
    const float kMaxGradientScale = 1.0f;
    const float kGradientRadius =
        kRadius * kGradientRadMultiplier *
        juce::jmap(kLoudness, kSilenceThreshold, 0.0f, kMinGradientScale,
                   kMaxGradientScale);

    const float kCenterAlpha = 0.8f;
    const float kEdgeAlpha = 0.0f;
    juce::ColourGradient gradient(colour.withAlpha(kCenterAlpha),  // center
                                  kSpeakerX, kSpeakerY,  // speaker position
                                  colour.withAlpha(kEdgeAlpha),  // edge
                                  kSpeakerX + kGradientRadius,
                                  kSpeakerY,  // edge of influence
                                  true);

    g.setGradientFill(gradient);

    // Draw a circle for this speaker's influence area
    juce::Rectangle<float> gradientBounds(
        kSpeakerX - kGradientRadius, kSpeakerY - kGradientRadius,
        kGradientRadius * 2, kGradientRadius * 2);

    g.fillEllipse(gradientBounds);
  }

  g.restoreState();
}

std::optional<std::pair<float, float>>
AmbisonicsVisualizer::projectSpeakerToView(const CartesianPoint3D& speaker3D,
                                           const VisualizerView& view,
                                           float centreX, float centreY,
                                           float radius) {
  float x2D, y2D;

  // Coordinate system: +X = front (azimuth 0°), +Y = left (azimuth 90°), +Z =
  // up (these come from SAF conventions)
  switch (view) {
    case VisualizerView::kFront:
      if (speaker3D.x < 0) return std::nullopt;  // speaker on far side (behind)
      x2D = speaker3D.y;
      y2D = -speaker3D.z;
      break;
    case VisualizerView::kRear:
      if (speaker3D.x > 0) return std::nullopt;  // speaker on far side (front)
      x2D = -speaker3D.y;
      y2D = -speaker3D.z;
      break;
    case VisualizerView::kLeft:
      if (speaker3D.y < 0) return std::nullopt;  // speaker on far side (right)
      x2D = speaker3D.x;
      y2D = -speaker3D.z;
      break;
    case VisualizerView::kRight:
      if (speaker3D.y > 0) return std::nullopt;  // speaker on far side (left)
      x2D = -speaker3D.x;
      y2D = -speaker3D.z;
      break;
    case VisualizerView::kTop:
      if (speaker3D.z < 0) return std::nullopt;  // speaker on far side (below)
      x2D = speaker3D.y;
      y2D = speaker3D.x;
      break;
    case VisualizerView::kBottom:
      if (speaker3D.z > 0) return std::nullopt;  // speaker on far side (above)
      x2D = speaker3D.y;
      y2D = -speaker3D.x;
      break;
  }

  // Convert from [-1, 1] normalized coordinates to pixel coordinates
  const float kPixelX = centreX + x2D * radius;
  const float kPixelY = centreY + y2D * radius;

  return std::make_pair(kPixelX, kPixelY);
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

void AmbisonicsVisualizer::updateSmoothedLoudnesses(
    const std::vector<float>& currentLoudnesses) {
  const float kWindowSeconds = 0.05f;

  const float dt = 1.0f / static_cast<float>(kRefreshRate_);
  const float alpha = 1.0f - std::exp(-dt / kWindowSeconds);
  for (int i = 0; i < currentLoudnesses.size(); i++) {
    const float kCurrLoudness =
        isnan(currentLoudnesses[i]) ? -100.0f : currentLoudnesses[i];
    const float delta = kCurrLoudness - smoothedLoudnesses_[i];
    smoothedLoudnesses_[i] += delta * alpha;
  }
  // Filter out possible NaN and inf values
  for (float& loudness : smoothedLoudnesses_) {
    if (std::isnan(loudness) || std::isinf(loudness)) {
      loudness = -100.0f;
    }
  }
}
