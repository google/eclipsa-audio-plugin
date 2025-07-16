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

#include "PerspectiveRoomViews.h"

#include "data_structures/src/RepositoryCollection.h"

TopView::TopView(const SpeakerMonitorData& monitorData,
                 RepositoryCollection repos)
    : PerspectiveRoomView(
          FaceLookup::getFaces(FaceLookup::kTop),
          Coordinates::getTopViewTransform(), {SpeakerLookup::kLFE},
          IconStore::getInstance().getTopIcon(), monitorData, repos) {};

const float TopView::getTrackScaling(const Coordinates::Point4D pt) const {
  return 0.35 * pt.a[FaceLookup::kAxisY] + 1.35;
};

SideView::SideView(const SpeakerMonitorData& monitorData,
                   RepositoryCollection repos)
    : PerspectiveRoomView(
          FaceLookup::getFaces(FaceLookup::kSide),
          Coordinates::getSideViewTransform(),
          {SpeakerLookup::kLS, SpeakerLookup::kLSS, SpeakerLookup::kLRS,
           SpeakerLookup::kLTR, SpeakerLookup::kLFE, SpeakerLookup::kFL,
           SpeakerLookup::kSIL},
          IconStore::getInstance().getLeftIcon(), monitorData, repos) {};

const float SideView::getTrackScaling(const Coordinates::Point4D pt) const {
  return -0.35 * pt.a[FaceLookup::kAxisX] + 1.35;
};

RearView::RearView(const SpeakerMonitorData& monitorData,
                   RepositoryCollection repos)
    : PerspectiveRoomView(
          FaceLookup::getFaces(FaceLookup::kRear),
          Coordinates::getRearViewTransform(),
          {SpeakerLookup::kLTB, SpeakerLookup::kRTB, SpeakerLookup::kLFE,
           SpeakerLookup::kTPBL, SpeakerLookup::kTPBR, SpeakerLookup::kBL,
           SpeakerLookup::kBR},
          IconStore::getInstance().getBackIcon(), monitorData, repos) {};

const float RearView::getTrackScaling(const Coordinates::Point4D pt) const {
  return 0.35 * pt.a[FaceLookup::kAxisZ] + 1.35;
};

IsoView::IsoView(const SpeakerMonitorData& monitorData,
                 RepositoryCollection repos)
    : PerspectiveRoomView(
          FaceLookup::getFaces(FaceLookup::kIso),
          Coordinates::getIsoViewTransform(), {SpeakerLookup::kLFE},
          IconStore::getInstance().getIsoIcon(), monitorData, repos) {};

void IsoView::drawFace(const std::array<Coordinates::Point2D, 4>& faceVerts,
                       const juce::Colour& c, juce::Graphics& g) {
  // Only draw outlines for non-transparent faces.
  if (c.getAlpha() == 1.f) {
    g.setColour(EclipsaColours::backgroundOffBlack);
    drawLine(faceVerts[0], faceVerts[1], g);
    drawLine(faceVerts[1], faceVerts[2], g);
    drawLine(faceVerts[2], faceVerts[3], g);
    drawLine(faceVerts[3], faceVerts[0], g);
  } else {
    // Draw one line for transparent faces to indicate where they join.
    g.setColour(EclipsaColours::backgroundOffBlack);
    drawLine(faceVerts[1], faceVerts[2], g, 1.f);
  }

  // Fill the face.
  juce::Path facePath;
  facePath.startNewSubPath(faceVerts[0].a[0], faceVerts[0].a[1]);
  facePath.lineTo(faceVerts[1].a[0], faceVerts[1].a[1]);
  facePath.lineTo(faceVerts[2].a[0], faceVerts[2].a[1]);
  facePath.lineTo(faceVerts[3].a[0], faceVerts[3].a[1]);
  facePath.closeSubPath();
  g.setColour(c);
  g.fillPath(facePath);
}

const float IsoView::getTrackScaling(const Coordinates::Point4D pt) const {
  return 1.35f;
};

AudioElementPluginRearView::AudioElementPluginRearView(
    const SpeakerMonitorData& monitorData)
    : PerspectiveRoomView(
          FaceLookup::getFaces(FaceLookup::kRear),
          Coordinates::getRearViewTransform(),
          {SpeakerLookup::kLTB, SpeakerLookup::kRTB, SpeakerLookup::kLFE,
           SpeakerLookup::kTPBL, SpeakerLookup::kTPBR, SpeakerLookup::kBL,
           SpeakerLookup::kBR},
          {}, monitorData) {};

const float AudioElementPluginRearView::getTrackScaling(
    const Coordinates::Point4D pt) const {
  return 0.35 * pt.a[FaceLookup::kAxisZ] + 1.35;
}

void AudioElementPluginRearView::drawTrack(const DrawableTrack& track,
                                           juce::Graphics& g) {
  // Determine the size of the outer track volume indicator based on the
  // loudness. Only draw if the track is not silent.
  const juce::Colour kTrackColour = getLoudnessColour(track.trackLoudness);
  float width;
  if (kTrackColour != EclipsaColours::speakerSilentFill) {
    float sf2 = 6 * (1 - std::abs(track.trackLoudness) / 60.f);
    width = 14.f * track.sizeScale * sf2;
    g.setColour(kTrackColour.withAlpha(0.5f));
    g.fillEllipse(track.pos.a[0] - width / 2, track.pos.a[1] - width / 2, width,
                  width);
  }

  // The panned audio element track center is blue independent of loudness.
  g.setColour(EclipsaColours::controlBlue);
  width = 14.f * track.sizeScale;
  g.fillEllipse(track.pos.a[0] - width / 2, track.pos.a[1] - width / 2, width,
                width);
}

void AudioElementPluginRearView::setElevationPattern(
    AudioElementSpatialLayout::Elevation elevation) {
  currentElevation_ = elevation;
}

void AudioElementPluginRearView::paint(juce::Graphics& g) {
  Coordinates::WindowData wData = {
      .leftCornerX = 0.0f,
      .bottomCornerY = (float)getHeight(),
      .width = (float)getWidth(),
      .height = (float)getHeight(),
  };

  PerspectiveRoomView::paint(g);

  switch (currentElevation_) {
    case AudioElementSpatialLayout::Elevation::kFlat:
      paintFlatElevation(wData, g);
      break;
    case AudioElementSpatialLayout::Elevation::kTent:
      paintTentElevation(wData, g);
      break;
    case AudioElementSpatialLayout::Elevation::kArch:
      paintArchElevation(wData, g);
      break;
    case AudioElementSpatialLayout::Elevation::kDome:
      paintDomeElevation(wData, g);
      break;
    case AudioElementSpatialLayout::Elevation::kCurve:
      paintCurveElevation(wData, g);
      break;
    default:
      break;
  }

  if (!transformedTracks_.empty()) {
    drawTrack(transformedTracks_[0], g);
  }
}

void AudioElementPluginRearView::paintFlatElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // Set of (x,z) vertices needed to draw the shape. The height is given by the
  // current elevation set by the UI.
  std::array<Coordinates::Point4D, 4> flatElevationAnchorVertices = {
      Coordinates::Point4D{-1.f, currentFlatHeight_, -1.f, 1.f},
      Coordinates::Point4D{1.f, currentFlatHeight_, -1.f, 1.},
      Coordinates::Point4D{-1.f, currentFlatHeight_, 1.f, 1.},
      Coordinates::Point4D{1.f, currentFlatHeight_, 1.f, 1.},
  };

  std::array<Coordinates::Point2D, 4> flatElevationVertices;
  for (int i = 0; i < flatElevationAnchorVertices.size(); ++i) {
    flatElevationVertices[i] = Coordinates::toWindow(
        kTransformMat_, window, flatElevationAnchorVertices[i]);
  }

  // Draw the path.
  juce::Path flatElevationPath;
  flatElevationPath.startNewSubPath(flatElevationVertices[0].a[0],
                                    flatElevationVertices[0].a[1]);
  flatElevationPath.lineTo(flatElevationVertices[1].a[0],
                           flatElevationVertices[1].a[1]);
  flatElevationPath.lineTo(flatElevationVertices[3].a[0],
                           flatElevationVertices[3].a[1]);
  flatElevationPath.lineTo(flatElevationVertices[2].a[0],
                           flatElevationVertices[2].a[1]);
  flatElevationPath.closeSubPath();
  g.setColour(EclipsaColours::roomviewTranslucentWall.brighter());
  g.fillPath(flatElevationPath);
}

void AudioElementPluginRearView::paintTentElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // Define the vertices needed to draw the tent shape using multiple different
  // coloured paths.
  const std::array<Coordinates::Point4D, 6> tentElevationAnchorVertices = {
      Coordinates::Point4D{-1.f, -1.f, -1.f, 1.f},
      Coordinates::Point4D{1.f, -1.f, -1.f, 1.},
      Coordinates::Point4D{-1.f, 1.f, 0.f, 1.f},
      Coordinates::Point4D{1.f, 1.f, 0.f, 1.},
      Coordinates::Point4D{1.f, -1.f, 1.f, 1.},
      Coordinates::Point4D{-1.f, -1.f, 1.f, 1.},
  };

  std::array<Coordinates::Point2D, 6> tentElevationVertices;
  for (int i = 0; i < tentElevationAnchorVertices.size(); ++i) {
    tentElevationVertices[i] = Coordinates::toWindow(
        kTransformMat_, window, tentElevationAnchorVertices[i]);
  }

  // Draw dark-grey paths.
  g.setColour(EclipsaColours::roomviewTranslucentWall.brighter(0.2f));
  juce::Path leftTentPath, rightTentPath, bottomTentPath;
  leftTentPath.startNewSubPath(tentElevationVertices[0].a[0],
                               tentElevationVertices[0].a[1]);
  leftTentPath.lineTo(tentElevationVertices[2].a[0],
                      tentElevationVertices[2].a[1]);
  leftTentPath.lineTo(tentElevationVertices[5].a[0],
                      tentElevationVertices[5].a[1]);
  leftTentPath.closeSubPath();
  g.fillPath(leftTentPath);

  rightTentPath.startNewSubPath(tentElevationVertices[1].a[0],
                                tentElevationVertices[1].a[1]);
  rightTentPath.lineTo(tentElevationVertices[3].a[0],
                       tentElevationVertices[3].a[1]);
  rightTentPath.lineTo(tentElevationVertices[4].a[0],
                       tentElevationVertices[4].a[1]);
  rightTentPath.closeSubPath();
  g.fillPath(rightTentPath);

  bottomTentPath.startNewSubPath(tentElevationVertices[0].a[0],
                                 tentElevationVertices[0].a[1]);
  bottomTentPath.lineTo(tentElevationVertices[1].a[0],
                        tentElevationVertices[1].a[1]);
  bottomTentPath.lineTo(tentElevationVertices[4].a[0],
                        tentElevationVertices[4].a[1]);
  bottomTentPath.lineTo(tentElevationVertices[5].a[0],
                        tentElevationVertices[5].a[1]);
  bottomTentPath.closeSubPath();
  g.fillPath(bottomTentPath);

  // Draw light-grey paths.
  g.setColour(EclipsaColours::roomviewTranslucentWall.brighter());
  juce::Path topTentPath;
  topTentPath.startNewSubPath(tentElevationVertices[0].a[0],
                              tentElevationVertices[0].a[1]);
  topTentPath.lineTo(tentElevationVertices[1].a[0],
                     tentElevationVertices[1].a[1]);
  topTentPath.lineTo(tentElevationVertices[3].a[0],
                     tentElevationVertices[3].a[1]);
  topTentPath.lineTo(tentElevationVertices[2].a[0],
                     tentElevationVertices[2].a[1]);
  topTentPath.closeSubPath();
  g.fillPath(topTentPath);
}

void AudioElementPluginRearView::paintArchElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // Use the function in Elevation to calculate points along the parabolic edge.
  std::vector<Coordinates::Point4D> leftArchElevationAnchorVertices;
  for (int i = 0; i < 41; ++i) {
    float offset = i * 0.05f;
    leftArchElevationAnchorVertices.push_back(Coordinates::Point4D{
        -1.f,
        ElevationListener::getArchElevationPt({-1.f, 1.f - offset, 1.f}).a[1],
        -1 + offset, 1.f});
  }

  // Draw dark-grey paths.
  juce::Path leftArchPath, rightArchPath, bottomArchPath;

  // Convert the 3D points to window points for the left arch.
  std::vector<Coordinates::Point2D> leftArchVertices;
  for (int i = 0; i < leftArchElevationAnchorVertices.size(); ++i) {
    leftArchVertices.push_back(Coordinates::toWindow(
        kTransformMat_, window, leftArchElevationAnchorVertices[i]));
  }
  leftArchPath.startNewSubPath(leftArchVertices[0].a[0],
                               leftArchVertices[0].a[1]);
  for (int i = 1; i < leftArchVertices.size() - 1; ++i) {
    leftArchPath.quadraticTo(leftArchVertices[i].a[0], leftArchVertices[i].a[1],
                             leftArchVertices[i + 1].a[0],
                             leftArchVertices[i + 1].a[1]);
  }
  leftArchPath.closeSubPath();
  g.setColour(EclipsaColours::roomviewTranslucentWall.brighter(0.2f));
  g.fillPath(leftArchPath);

  // Convert the 3D points to window points for the right arch.
  std::vector<Coordinates::Point2D> rightArchVertices;
  for (int i = 0; i < leftArchElevationAnchorVertices.size(); ++i) {
    // Right arch points are left arch points mirrored along the z-axis.
    const Coordinates::Point4D rightArchPt = {
        -leftArchElevationAnchorVertices[i].a[0],
        leftArchElevationAnchorVertices[i].a[1],
        leftArchElevationAnchorVertices[i].a[2], 1.f};
    rightArchVertices.push_back(
        Coordinates::toWindow(kTransformMat_, window, rightArchPt));
  }
  // Paint the right arch.
  rightArchPath.startNewSubPath(rightArchVertices[0].a[0],
                                rightArchVertices[0].a[1]);
  for (int i = 1; i < rightArchVertices.size() - 1; ++i) {
    rightArchPath.quadraticTo(
        rightArchVertices[i].a[0], rightArchVertices[i].a[1],
        rightArchVertices[i + 1].a[0], rightArchVertices[i + 1].a[1]);
  }
  rightArchPath.closeSubPath();
  g.fillPath(rightArchPath);

  // Paint the floor.
  std::array<Coordinates::Point4D, 4> flatElevationAnchorVertices = {
      Coordinates::Point4D{-1.f, -1.f, -1.f, 1.f},
      Coordinates::Point4D{1.f, -1.f, -1.f, 1.},
      Coordinates::Point4D{1.f, -1.f, 1.f, 1.},
      Coordinates::Point4D{-1.f, -1.f, 1.f, 1.},
  };
  std::array<Coordinates::Point2D, 4> flatElevationVertices;
  for (int i = 0; i < flatElevationAnchorVertices.size(); ++i) {
    flatElevationVertices[i] = Coordinates::toWindow(
        kTransformMat_, window, flatElevationAnchorVertices[i]);
  }
  bottomArchPath.startNewSubPath(flatElevationVertices[0].a[0],
                                 flatElevationVertices[0].a[1]);
  for (int i = 1; i < flatElevationVertices.size(); ++i) {
    bottomArchPath.lineTo(flatElevationVertices[i].a[0],
                          flatElevationVertices[i].a[1]);
  }
  bottomArchPath.closeSubPath();
  g.setColour(EclipsaColours::roomviewTranslucentWall.brighter(0.2f));
  g.fillPath(bottomArchPath);

  // Draw light-grey path. The light grey path uses points of both the left and
  // right arches.
  juce::Path topArchPath;
  g.setColour(EclipsaColours::roomviewTranslucentWall.brighter());
  topArchPath.startNewSubPath(leftArchVertices[0].a[0],
                              leftArchVertices[0].a[1]);
  for (int i = 0; i < leftArchVertices.size() / 2 + 2; ++i) {
    topArchPath.quadraticTo(leftArchVertices[i].a[0], leftArchVertices[i].a[1],
                            leftArchVertices[i + 1].a[0],
                            leftArchVertices[i + 1].a[1]);
  }
  for (int i = rightArchVertices.size() / 2 + 3; i > 0; --i) {
    topArchPath.quadraticTo(
        rightArchVertices[i].a[0], rightArchVertices[i].a[1],
        rightArchVertices[i - 1].a[0], rightArchVertices[i - 1].a[1]);
    rightArchPath.createPathWithRoundedCorners(1.f);
  }
  topArchPath.closeSubPath();
  g.fillPath(topArchPath);
}

void AudioElementPluginRearView::paintDomeElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // Generate N linearly spaced points from 0 to 2Pi.
  const int N = 81;
  std::vector<float> theta(N);
  for (int i = 0; i < N; ++i) {
    theta[i] = i * juce::MathConstants<float>::twoPi / (N - 1);
  }

  // Generate the dome floor path points.
  std::vector<Coordinates::Point4D> domeFloorVertices3D;
  for (int i = 0; i < N; ++i) {
    float x = std::cos(theta[i]);
    float y = std::sin(theta[i]);
    auto floorPt =
        ElevationListener::getDomeElevationPtClamped({x, y, 0.f}, {});
    domeFloorVertices3D.push_back(
        {floorPt.a[0], floorPt.a[1], floorPt.a[2], 1.f});
  };
  // Generate the dome ceiling path points.
  std::vector<Coordinates::Point4D> domeRoofVertices3D;
  const float offset = 2.f / N;
  for (int i = 0; i < N + 1; ++i) {
    float x = 1 - i * offset;
    auto roofPt =
        ElevationListener::getDomeElevationPtClamped({x, 0.f, 0.f}, {});
    domeRoofVertices3D.push_back({roofPt.a[0], roofPt.a[1], roofPt.a[2], 1.f});
  }

  // Convert the 3D points to window points.
  std::vector<Coordinates::Point2D> domeFloorVertices;
  for (const auto& pt : domeFloorVertices3D) {
    domeFloorVertices.push_back(
        Coordinates::toWindow(kTransformMat_, window, pt));
  }
  std::vector<Coordinates::Point2D> domeRoofVertices;
  for (const auto& pt : domeRoofVertices3D) {
    domeRoofVertices.push_back(
        Coordinates::toWindow(kTransformMat_, window, pt));
  }

  // Draw the dome path using the front bottom vertices and the ceiling
  // vertices. We use a bit of offset here because if the top arch of the dome
  // connects to the circle exactly at its middle edges, it looks a bit weird.
  const int kOff = 4;
  juce::Path domePath;
  domePath.startNewSubPath(domeFloorVertices[kOff].a[0],
                           domeFloorVertices[kOff].a[1]);
  for (int i = kOff; i < domeRoofVertices.size() - kOff; ++i) {
    domePath.lineTo(domeRoofVertices[i].a[0], domeRoofVertices[i].a[1]);
  }
  domePath.lineTo(domeFloorVertices[domeFloorVertices.size() / 2 - kOff].a[0],
                  domeFloorVertices[domeFloorVertices.size() / 2 - kOff].a[1]);
  for (int i = domeFloorVertices.size() / 2 - kOff; i > kOff; --i) {
    domePath.lineTo(domeFloorVertices[i].a[0], domeFloorVertices[i].a[1]);
  }

  domePath.closeSubPath();
  domePath.createPathWithRoundedCorners(2.f);
  g.setColour(EclipsaColours::roomviewTranslucentWall.brighter(0.2f));
  g.fillPath(domePath);
}

void AudioElementPluginRearView::paintCurveElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // Generate N linearly spaced points from -1 to 1.
  const int N = 32;
  std::vector<float> y(N);
  for (int i = 0; i < N; ++i) {
    y[i] = -1.f + i * 2.f / (N - 1);
  }

  // Generate the curve path points.
  std::vector<Coordinates::Point4D> curveVertices3D(N);
  for (int i = 0; i < N; ++i) {
    auto curvePt = ElevationListener::getCurveElevationPt({-1.f, y[i], 0.f});
    curveVertices3D[i] = {curvePt.a[0], curvePt.a[1], y[i], 1.f};
  }

  // Convert the 3D points to window points.
  std::vector<Coordinates::Point2D> curveVertices;
  for (const auto& pt : curveVertices3D) {
    curveVertices.push_back(Coordinates::toWindow(kTransformMat_, window, pt));
  }
  // Mirror the points across the y-axis and convert them to window points.
  for (auto& pt : curveVertices3D) {
    pt.a[0] = -pt.a[0];
    curveVertices.push_back(Coordinates::toWindow(kTransformMat_, window, pt));
  }

  // Draw the curve path.
  juce::Path curvePath;
  curvePath.startNewSubPath(curveVertices[0].a[0], curveVertices[0].a[1]);
  for (int i = 1; i < curveVertices.size() / 2; ++i) {
    curvePath.lineTo(curveVertices[i].a[0], curveVertices[i].a[1]);
  }
  for (int i = curveVertices.size() - 1; i > curveVertices.size() / 2 - 1;
       --i) {
    curvePath.lineTo(curveVertices[i].a[0], curveVertices[i].a[1]);
  }
  curvePath.closeSubPath();
  g.setColour(EclipsaColours::roomviewTranslucentWall.brighter(0.2f));
  g.fillPath(curvePath);
}
