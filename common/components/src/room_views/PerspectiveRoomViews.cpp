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
AudioElementPluginTopView::AudioElementPluginTopView(
    const SpeakerMonitorData& monitorData)
    : PerspectiveRoomView(FaceLookup::getFaces(FaceLookup::kTop),
                          Coordinates::getTopViewTransform(),
                          {SpeakerLookup::kLFE}, {}, monitorData) {};

const float AudioElementPluginTopView::getTrackScaling(
    const Coordinates::Point4D pt) const {
  // Under a top-down projection the depth axis is NDC up, not NDC z.
  return 0.35 * pt.a[FaceLookup::kAxisY] + 1.35;
}

void AudioElementPluginTopView::drawTrack(const DrawableTrack& track,
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

void AudioElementPluginTopView::setElevationPattern(
    AudioElementSpatialLayout::Elevation elevation) {
  currentElevation_ = elevation;
}

void AudioElementPluginTopView::paint(juce::Graphics& g) {
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

// The five elevation painters are dispatched from paint above, but draw
// nothing yet: the rear view's shapes do not carry over to a plan projection
// and are re-derived for it separately.

void AudioElementPluginTopView::paintFlatElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // TODO(PAN-01.2): render the flat elevation under the top-down projection.
}

void AudioElementPluginTopView::paintTentElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // TODO(PAN-01.2): render the tent elevation under the top-down projection.
}

void AudioElementPluginTopView::paintArchElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // TODO(PAN-01.2): render the arch elevation under the top-down projection.
}

void AudioElementPluginTopView::paintDomeElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // TODO(PAN-01.2): render the dome elevation under the top-down projection.
}

void AudioElementPluginTopView::paintCurveElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // TODO(PAN-01.2): render the curve elevation under the top-down projection.
}
