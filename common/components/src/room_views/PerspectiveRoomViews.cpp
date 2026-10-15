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

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

#include "components/src/room_views/ElevationSurfaces.h"
#include "components/src/room_views/HeightIndicator.h"
#include "data_structures/src/RepositoryCollection.h"

namespace {

// The two shades of the room-view translucent grey the rear projection
// established. A pattern that presents more than one surface uses both, so
// adjacent surfaces read as separate planes instead of merging into one panel.
//
// Named for the elevation surfaces on purpose: this file is unity-built into
// components.cpp with twenty other sources, and an anonymous namespace is
// shared across that whole translation unit, so a bare `recedingSurface` here
// would be a name another source could collide with.
juce::Colour elevationRecedingSurface() {
  return EclipsaColours::roomviewTranslucentWall.brighter(0.2f);
}
juce::Colour elevationFacingSurface() {
  return EclipsaColours::roomviewTranslucentWall.brighter();
}

// The height indicator's line weights.
constexpr float kOutlineThickness = 2.f;
constexpr float kHeightIndicatorConnectorThickness = 2.f;

// The marker's drawn diameter before depth scaling, shared by the draw and the
// hit test.
constexpr float kAudioSourceMarkerDiameter = 14.f;
// Widens the grab target past the drawn edge.
constexpr float kAudioSourceGrabMargin = 4.f;

// setValueNotifyingHost takes a normalized value. Passing a raw position would
// peg the parameter to its maximum.
//
// Emits no gesture of its own: the drag brackets the whole gesture from
// mouseDown to mouseUp, and writeSteppedPositionParameter brackets a single
// step.
void writePositionParameter(juce::RangedAudioParameter* parameter,
                            const int value) {
  const int kClamped =
      std::clamp(value, AutoParamMetaData::positionRange_.first,
                 AutoParamMetaData::positionRange_.second);
  parameter->setValueNotifyingHost(parameter->convertTo0to1((float)kClamped));
}

// A nudge and a wheel notch are each one discrete change rather than a
// continuous gesture, so each carries its own bracket and none is left open
// between events.
void writeSteppedPositionParameter(juce::RangedAudioParameter* parameter,
                                   const int value) {
  parameter->beginChangeGesture();
  writePositionParameter(parameter, value);
  parameter->endChangeGesture();
}

}  // namespace

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
    : PerspectiveRoomView(FaceLookup::getFaces(FaceLookup::kPannerTop),
                          Coordinates::getTopViewTransform(),
                          {SpeakerLookup::kLFE}, {}, monitorData) {
  // Arrow keys reach keyPressed only once this view can hold focus. It takes
  // focus on mouseDown rather than on construction, so opening the plugin does
  // not steal it from whatever the host had focused.
  setWantsKeyboardFocus(true);
};

const float AudioElementPluginTopView::getTrackScaling(
    const Coordinates::Point4D pt) const {
  // Under a top-down projection the depth axis is NDC up, not NDC z.
  return 0.35 * pt.a[FaceLookup::kAxisY] + 1.35;
}

void AudioElementPluginTopView::drawFace(
    const std::array<Coordinates::Point2D, 4>& faceVerts, const juce::Colour& c,
    juce::Graphics& g) {
  if (c != EclipsaColours::roomviewScreen) {
    PerspectiveRoomView::drawFace(faceVerts, c, g);
    return;
  }
  // The screen is filled without the wall outline, so it reads as an object on
  // the wall rather than a panel of it.
  juce::Path screenPath;
  screenPath.startNewSubPath(faceVerts[0].a[0], faceVerts[0].a[1]);
  for (size_t i = 1; i < faceVerts.size(); ++i) {
    screenPath.lineTo(faceVerts[i].a[0], faceVerts[i].a[1]);
  }
  screenPath.closeSubPath();
  g.setColour(c);
  g.fillPath(screenPath);
}

void AudioElementPluginTopView::drawTrack(const DrawableTrack& track,
                                          juce::Graphics& g) {
  // Determine the size of the outer track volume indicator based on the
  // loudness. Only draw if the track is not silent.
  const juce::Colour kTrackColour = getLoudnessColour(track.trackLoudness);
  float width;
  if (kTrackColour != EclipsaColours::speakerSilentFill) {
    float sf2 = 6 * (1 - std::abs(track.trackLoudness) / 60.f);
    width = kAudioSourceMarkerDiameter * track.sizeScale * sf2;
    g.setColour(kTrackColour.withAlpha(0.5f));
    g.fillEllipse(track.pos.a[0] - width / 2, track.pos.a[1] - width / 2, width,
                  width);
  }

  // The panned audio element track center is blue independent of loudness.
  g.setColour(EclipsaColours::controlBlue);
  width = kAudioSourceMarkerDiameter * track.sizeScale;
  g.fillEllipse(track.pos.a[0] - width / 2, track.pos.a[1] - width / 2, width,
                width);
}

void AudioElementPluginTopView::setElevationPattern(
    AudioElementSpatialLayout::Elevation elevation) {
  currentElevation_ = elevation;
}

juce::RangedAudioParameter* AudioElementPluginTopView::positionParameter(
    const juce::String& parameterName) const {
  if (parameterTree_ == nullptr) {
    return nullptr;
  }
  return parameterTree_->getParameter(
      AutoParamMetaData::getParameterIDFromName(parameterName).getParamID());
}

bool AudioElementPluginTopView::sourceMarkerContains(
    const juce::Point<float>& windowPoint) const {
  const DrawableTrack& kSource = transformedTracks_[0];
  const float kRadius = kAudioSourceMarkerDiameter * kSource.sizeScale / 2.f +
                        kAudioSourceGrabMargin;
  return windowPoint.getDistanceFrom({kSource.pos.a[0], kSource.pos.a[1]}) <=
         kRadius;
}

int AudioElementPluginTopView::speakerIndexAt(
    const juce::Point<float>& windowPoint) const {
  // setSpeakers only flags the vertices for recalculation, so the two lists
  // are briefly out of step between a layout change and the next paint. An
  // index resolved against a stale window list would name a different speaker.
  if (speakers_.size() != transformedSpeakers_.size()) {
    return PannerInput::kNoSpeaker;
  }

  std::vector<PannerInput::SpeakerTarget> targets;
  targets.reserve(transformedSpeakers_.size());
  for (const DrawableSpeaker& spkr : transformedSpeakers_) {
    targets.push_back({{spkr.pos.a[0], spkr.pos.a[1]}, speakerIsDrawn(spkr)});
  }
  return PannerInput::speakerIndexAt(targets, windowPoint);
}

void AudioElementPluginTopView::snapToSpeaker(const int speakerIndex) {
  const Coordinates::PositionParameters kTarget =
      Coordinates::fromRoomNdc(speakers_[speakerIndex].pos);
  juce::RangedAudioParameter* xParameter =
      positionParameter(AutoParamMetaData::xPosition);
  juce::RangedAudioParameter* yParameter =
      positionParameter(AutoParamMetaData::yPosition);
  // Unbracketed: the snap sits inside the drag gesture mouseDown has opened.
  if (xParameter != nullptr) {
    writePositionParameter(xParameter, kTarget.x);
  }
  if (yParameter != nullptr) {
    writePositionParameter(yParameter, kTarget.y);
  }
  // Under every other pattern height belongs to the pattern: ElevationListener
  // derives it from the position just written, so writing height here would
  // fight it. Unlike a drag, no plan clamp is needed: every speaker sits
  // inside the dome's footprint.
  if (PannerInput::clickSetsHeight(currentElevation_)) {
    juce::RangedAudioParameter* zParameter =
        positionParameter(AutoParamMetaData::zPosition);
    if (zParameter != nullptr) {
      writeSteppedPositionParameter(zParameter, kTarget.z);
    }
  }
}

void AudioElementPluginTopView::writeDragPosition(
    const juce::Point<float>& windowPoint) {
  if (parameterTree_ == nullptr) {
    return;
  }
  // The plane the pointer is un-projected onto. Under a pattern that positions
  // in plan it is FIXED, so the pointer names one position however the height
  // then moves. Reading it from the source's live height instead closes a
  // feedback loop through ElevationListener: each event un-projects at the
  // height the last one produced, and where the projection folds that loop has
  // gain above 1, so the source runs away from the pointer and lands somewhere
  // that depends on where it started.
  const float kNdcUp =
      elevationPositionsInPlan()
          ? Coordinates::kPlanPlaneUp
          : Coordinates::toRoomNdc(0.f, 0.f,
                                   (float)parameterTree_->getZPosition())
                .a[1];
  const Coordinates::Point2D kWindowPoint = {windowPoint.x, windowPoint.y};
  const std::optional<Coordinates::Point4D> kRoomNdc =
      Coordinates::fromTopViewWindow(kTransformMat_, currentWindow(),
                                     kWindowPoint, kNdcUp);
  // The source keeps its last position while the panner has no area.
  if (!kRoomNdc) {
    return;
  }
  // Clamped before the write: a correction made after it has already been
  // published to the host.
  const Coordinates::PositionParameters kTarget =
      PannerInput::clampToElevationPlan(currentElevation_,
                                        Coordinates::fromRoomNdc(*kRoomNdc));

  // Each event converts the pointer afresh, so no drift accumulates.
  juce::RangedAudioParameter* xParameter =
      positionParameter(AutoParamMetaData::xPosition);
  juce::RangedAudioParameter* yParameter =
      positionParameter(AutoParamMetaData::yPosition);
  if (xParameter != nullptr) {
    writePositionParameter(xParameter, kTarget.x);
  }
  if (yParameter != nullptr) {
    writePositionParameter(yParameter, kTarget.y);
  }
}

void AudioElementPluginTopView::mouseDown(const juce::MouseEvent& event) {
  draggingSource_ = false;
  // Any press in the panner takes focus, not just one that grabs the source:
  // nudge is meant to work after the user has interacted with the panner at
  // all, and a press that missed the marker is still that interaction.
  grabKeyboardFocus();
  if (parameterTree_ == nullptr) {
    return;
  }
  // Tested before the source, so a press on a speaker snaps even where the
  // source is sitting on top of one. Either press starts a drag.
  const int kSpeakerIndex = speakerIndexAt(event.position);
  const bool kOnSpeaker = kSpeakerIndex != PannerInput::kNoSpeaker;
  if (!kOnSpeaker &&
      (transformedTracks_.empty() || !sourceMarkerContains(event.position))) {
    return;
  }
  draggingSource_ = true;
  holdingSnap_ = kOnSpeaker;

  // Bracket the drag so the host records one continuous gesture, snap included.
  juce::RangedAudioParameter* xParameter =
      positionParameter(AutoParamMetaData::xPosition);
  juce::RangedAudioParameter* yParameter =
      positionParameter(AutoParamMetaData::yPosition);
  if (xParameter != nullptr) {
    xParameter->beginChangeGesture();
  }
  if (yParameter != nullptr) {
    yParameter->beginChangeGesture();
  }
  if (kOnSpeaker) {
    snapToSpeaker(kSpeakerIndex);
  }
}

void AudioElementPluginTopView::mouseDrag(const juce::MouseEvent& event) {
  if (!draggingSource_) {
    return;
  }
  // A click jitters by a pixel or two; moving the source on that would pull it
  // off the speaker it just snapped to.
  if (holdingSnap_ && !event.mouseWasDraggedSinceMouseDown()) {
    return;
  }
  holdingSnap_ = false;
  writeDragPosition(event.position);
}

void AudioElementPluginTopView::mouseUp(const juce::MouseEvent& event) {
  if (!draggingSource_) {
    return;
  }
  draggingSource_ = false;
  juce::RangedAudioParameter* xParameter =
      positionParameter(AutoParamMetaData::xPosition);
  juce::RangedAudioParameter* yParameter =
      positionParameter(AutoParamMetaData::yPosition);
  if (xParameter != nullptr) {
    xParameter->endChangeGesture();
  }
  if (yParameter != nullptr) {
    yParameter->endChangeGesture();
  }
}

void AudioElementPluginTopView::nudgeAxis(const PannerInput::Axis axis,
                                          const int steps) {
  if (parameterTree_ == nullptr || steps == 0) {
    return;
  }
  juce::String parameterName;
  int current = 0;
  switch (axis) {
    case PannerInput::Axis::kLeftRight:
      parameterName = AutoParamMetaData::xPosition;
      current = parameterTree_->getXPosition();
      break;
    case PannerInput::Axis::kFrontBack:
      parameterName = AutoParamMetaData::yPosition;
      current = parameterTree_->getYPosition();
      break;
    case PannerInput::Axis::kHeight:
      parameterName = AutoParamMetaData::zPosition;
      current = parameterTree_->getZPosition();
      break;
    case PannerInput::Axis::kNone:
      return;
  }

  juce::RangedAudioParameter* parameter = positionParameter(parameterName);
  if (parameter == nullptr) {
    return;
  }
  // Stepped from the parameter's own current value, so the source is rendered
  // from the parameter rather than from an accumulated position and
  // ElevationListener's side effects stay visible.
  writeSteppedPositionParameter(parameter, current + steps);
}

void AudioElementPluginTopView::mouseWheelMove(
    const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
  if (parameterTree_ == nullptr ||
      PannerInput::elevationOwnsHeight(currentElevation_)) {
    // Under a derived pattern ElevationListener recomputes height from x/y and
    // would overwrite the notch before it was seen, so pass the event on
    // rather than write a value that will not stick.
    PerspectiveRoomView::mouseWheelMove(event, wheel);
    return;
  }
  const int kSteps =
      PannerInput::stepsForWheelDelta(wheel.deltaY, wheelAccumulator_);
  nudgeAxis(PannerInput::Axis::kHeight, kSteps);
}

bool AudioElementPluginTopView::keyPressed(const juce::KeyPress& key) {
  const PannerInput::Nudge kNudge =
      PannerInput::nudgeForKeyCode(key.getKeyCode());
  if (kNudge.axis == PannerInput::Axis::kNone) {
    // Not a nudge -- let the host and the rest of the UI see the key.
    return false;
  }
  nudgeAxis(kNudge.axis, kNudge.steps);
  return true;
}

Coordinates::WindowData AudioElementPluginTopView::currentWindow() const {
  return {
      .leftCornerX = 0.0f,
      .bottomCornerY = (float)getHeight(),
      .width = (float)getWidth(),
      .height = (float)getHeight(),
  };
}

void AudioElementPluginTopView::paint(juce::Graphics& g) {
  const Coordinates::WindowData wData = currentWindow();

  PerspectiveRoomView::paint(g);

  const bool kHeadUnderSurface =
      ListenerHead::drawnBeforeElevation(currentElevation_, currentFlatHeight_);
  if (kHeadUnderSurface) {
    paintListenerHead(wData, g);
  }

  // First split the indicator against the elevation surface: the outline and
  // the back-edge connector pass under it, so each is drawn either side of the
  // fill. The right-edge connector is coincident with the surface rather than
  // either side of it and stays on top -- see splitLeaderLinesAtElevation.
  HeightIndicator::SplitOutline outline, connectors;
  if (!transformedTracks_.empty()) {
    // One height function for both splits, so the outline and a connector
    // agree about the point where they meet.
    const auto kRoofAt = [this](const float leftRight, const float frontBack) {
      return elevationHeightAt(leftRight, frontBack);
    };
    const Coordinates::Point4D kSourcePos =
        sourceHeightPosition(transformedTracks_[0].ndcPos);
    const Coordinates::Point4D kIndicatorPos =
        indicatorPosition(transformedTracks_[0], wData);
    outline = HeightIndicator::splitAtElevation(kSourcePos.a[1], kRoofAt);
    // Split against the surface where the source is, not where it is drawn.
    connectors = HeightIndicator::splitLeaderLinesDrawnAt(
        kSourcePos, kIndicatorPos, kRoofAt, elevationVariesAcrossLeftRight());
  }

  // Then the runs that pass under the surface, so the fill tints them.
  paintIndicatorRuns(wData, outline.below, kOutlineThickness, g);
  paintIndicatorRuns(wData, connectors.below,
                     kHeightIndicatorConnectorThickness, g);

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

  if (!kHeadUnderSurface) {
    paintListenerHead(wData, g);
  }

  // Then the runs over it, at full strength.
  paintIndicatorRuns(wData, outline.above, kOutlineThickness, g);
  paintIndicatorRuns(wData, connectors.above,
                     kHeightIndicatorConnectorThickness, g);

  if (!transformedTracks_.empty()) {
    // The audio source marker is drawn last so it is always on top.
    drawTrack(transformedTracks_[0], g);
  }
}

float AudioElementPluginTopView::elevationHeightAt(
    const float leftRight, const float frontBack) const {
  switch (currentElevation_) {
    case AudioElementSpatialLayout::Elevation::kFlat:
      return currentFlatHeight_;
    case AudioElementSpatialLayout::Elevation::kTent:
      return ElevationListener::getTentElevationPt({-1.f, frontBack, 0.f}).a[1];
    case AudioElementSpatialLayout::Elevation::kArch:
      return ElevationListener::getArchElevationPt({-1.f, frontBack, 0.f}).a[1];
    case AudioElementSpatialLayout::Elevation::kCurve:
      // Sampled un-negated, on paintCurveElevation's sign convention -- the
      // value the listener passes IS the NDC front/back coordinate.
      return ElevationListener::getCurveElevationPt({-1.f, frontBack, 0.f})
          .a[1];
    case AudioElementSpatialLayout::Elevation::kDome: {
      // The one pattern needing BOTH coordinates: it falls away from the
      // room's centre in left/right as well as front/back.
      //
      // Beyond the dome's rim the surface is at the floor. Checked here rather
      // than left to the static, which would clamp such a point onto the rim
      // instead and can take the square root of a negative.
      const float kRadiusSq = leftRight * leftRight + frontBack * frontBack;
      if (kRadiusSq >= 1.f) {
        return -1.f;
      }
      return ElevationListener::getDomeElevationPtClamped(
                 {leftRight, frontBack, 0.f}, {})
          .a[1];
    }
    default:
      // kNone draws nothing, so every run comes back in `above`.
      return -1.f;
  }
}

Coordinates::Point4D AudioElementPluginTopView::indicatorPosition(
    const DrawableTrack& source, const Coordinates::WindowData& window) const {
  const Coordinates::Point4D kHeightAnchored =
      sourceHeightPosition(source.ndcPos);
  if (!elevationPositionsInPlan()) {
    return kHeightAnchored;
  }
  // The marker is drawn on the plan plane, so anchor the indicator at the point
  // on the source's height plane that projects onto the marker. The leader
  // lines then start at the marker and still end on the height's outline.
  const Coordinates::Point2D kMarker = {source.pos.a[0], source.pos.a[1]};
  return Coordinates::fromTopViewWindow(kTransformMat_, window, kMarker,
                                        kHeightAnchored.a[1])
      .value_or(kHeightAnchored);
}

Coordinates::Point4D AudioElementPluginTopView::sourceHeightPosition(
    const Coordinates::Point4D& sourceNdc) const {
  // Where a pattern clamps the source, read the height back off the surface
  // rather than off the position parameter. The parameter is quantised, and
  // comparing a quantised height against the exact surface is ill conditioned
  // near a crest -- on the dome that dimmed a long run of the indicator beside
  // a source visibly resting on the surface. Reading the surface on both sides
  // of the comparison takes the quantisation out of it, and moves the drawn
  // height by at most half a parameter step.
  if (!elevationClampsTheSource()) {
    return sourceNdc;
  }
  Coordinates::Point4D snapped = sourceNdc;
  snapped.a[1] = elevationHeightAt(sourceNdc.a[0], sourceNdc.a[2]);
  return snapped;
}

bool AudioElementPluginTopView::elevationClampsTheSource() const {
  // The four patterns ElevationListener recomputes the height for. kFlat and
  // kNone leave the z dial live, so under those only the parameter knows the
  // source's height.
  switch (currentElevation_) {
    case AudioElementSpatialLayout::Elevation::kTent:
    case AudioElementSpatialLayout::Elevation::kArch:
    case AudioElementSpatialLayout::Elevation::kDome:
    case AudioElementSpatialLayout::Elevation::kCurve:
      return true;
    default:
      return false;
  }
}

bool AudioElementPluginTopView::elevationPositionsInPlan() const {
  // Only the dome. Its height falls away in left/right as well as front/back,
  // so a source resting on it projects out by r / (5 - height(r)), which peaks
  // at r = 2*sqrt(2)/3 and comes back: one screen position names two plan
  // radii, and neither the drag nor the hit test can tell them apart. The
  // other patterns vary in front/back alone and stay monotonic, so they are
  // positioned in perspective, with the marker resting on the drawn surface.
  return currentElevation_ == AudioElementSpatialLayout::Elevation::kDome;
}

Coordinates::Point4D AudioElementPluginTopView::trackDrawPosition(
    const Coordinates::Point4D& ndcPos) const {
  if (!elevationPositionsInPlan()) {
    return ndcPos;
  }
  return Coordinates::toPlanPlane(ndcPos);
}

bool AudioElementPluginTopView::elevationVariesAcrossLeftRight() const {
  // Only the dome. The others are height fields in front/back alone, so the
  // right-edge connector lies at a single surface height under them and is
  // placed rather than split -- see
  // HeightIndicator::splitLeaderLinesAtElevation.
  return currentElevation_ == AudioElementSpatialLayout::Elevation::kDome;
}

void AudioElementPluginTopView::paintListenerHead(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // Painted here rather than through imageComponent_: a child component paints
  // over all of this view's paint output, so it could not sit under a surface.
  const juce::Image kHead = IconStore::getInstance().getTopIcon();
  // drawImage draws at the context's current opacity, which the elevation
  // painters leave at the surface's alpha.
  g.setOpacity(1.f);
  const Coordinates::Point2D kCentre =
      Coordinates::toWindow(kTransformMat_, window, {0.f, 0.f, 0.f, 1.f});
  g.drawImage(kHead, kHead.getBounds().toFloat().withCentre(
                         {kCentre.a[0], kCentre.a[1]}));
}

void AudioElementPluginTopView::paintIndicatorRuns(
    const Coordinates::WindowData& window,
    const std::vector<HeightIndicator::Segment>& runs, const float thickness,
    juce::Graphics& g) {
  g.setColour(EclipsaColours::controlBlue);
  for (const HeightIndicator::Segment& run : runs) {
    const std::array<Coordinates::Point2D, 2> kEnds =
        HeightIndicator::projectSegment(kTransformMat_, window, run);
    drawLine(kEnds[0], kEnds[1], g, thickness);
  }
}

// The five elevation painters below are dispatched from paint above.
//
// The top view is not an orthographic plan view: getTopViewTransform() is a
// 45-degree perspective with the camera rotated 90 degrees about X, so its
// w = 5 - y. Height therefore scales the projection, and an elevation pattern
// reads as a 3D surface seen from above rather than as a flat outline -- which
// is why only Flat is a single panel here, and why surfaces the rear
// projection could omit as occluded are visible and must be drawn.
//
// Heights come from the statics in Elevation.h; no elevation curve is
// re-derived here. Those helpers take the front/back coordinate in a[1] and
// return the height in a[1] (see the axis note in Elevation.h).

void AudioElementPluginTopView::paintFlatElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // The one pattern that is genuinely a single plane: a panel over the floor at
  // the height set through setFlatHeight. Height is indicated by the uniform
  // tint alone.
  const std::vector<Coordinates::Point4D> kFlatAnchors = {
      {-1.f, currentFlatHeight_, -1.f, 1.f},
      {1.f, currentFlatHeight_, -1.f, 1.f},
      {1.f, currentFlatHeight_, 1.f, 1.f},
      {-1.f, currentFlatHeight_, 1.f, 1.f},
  };

  g.setColour(elevationFacingSurface());
  g.fillPath(
      ElevationSurfaces::anchorsToPath(kTransformMat_, window, kFlatAnchors));
}

void AudioElementPluginTopView::paintTentElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // Two sloping planes meeting at the ridge, off the rear projection's six
  // anchors: floor corners at (+/-1, -1, -/+1) and ridge points at
  // (+/-1, 1, 0). The rear projection drew only the front slope because the
  // back slope was occluded; from above both are visible.
  const Coordinates::Point4D kFrontFloorLeft = {-1.f, -1.f, -1.f, 1.f};
  const Coordinates::Point4D kFrontFloorRight = {1.f, -1.f, -1.f, 1.f};
  const Coordinates::Point4D kRidgeLeft = {-1.f, 1.f, 0.f, 1.f};
  const Coordinates::Point4D kRidgeRight = {1.f, 1.f, 0.f, 1.f};
  const Coordinates::Point4D kBackFloorRight = {1.f, -1.f, 1.f, 1.f};
  const Coordinates::Point4D kBackFloorLeft = {-1.f, -1.f, 1.f, 1.f};

  // Shade the two planes differently so the shared ridge reads as an edge
  // rather than the pair merging into one panel.
  g.setColour(elevationRecedingSurface());
  g.fillPath(ElevationSurfaces::anchorsToPath(
      kTransformMat_, window,
      {kBackFloorLeft, kBackFloorRight, kRidgeRight, kRidgeLeft}));

  g.setColour(elevationFacingSurface());
  g.fillPath(ElevationSurfaces::anchorsToPath(
      kTransformMat_, window,
      {kFrontFloorLeft, kFrontFloorRight, kRidgeRight, kRidgeLeft}));
}

void AudioElementPluginTopView::paintArchElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // A curved surface, not a silhouette edge: sample the parabola along the
  // left and right room edges at the same front/back positions, then stitch
  // the two sampled edges into one filled surface spanning left to right.
  const int kNumSamples = 41;
  std::vector<Coordinates::Point4D> leftEdge, rightEdge;
  leftEdge.reserve(kNumSamples);
  rightEdge.reserve(kNumSamples);
  for (int i = 0; i < kNumSamples; ++i) {
    const float offset = i * 0.05f;
    const float frontBack = -1.f + offset;
    const float height =
        ElevationListener::getArchElevationPt({-1.f, frontBack, 0.f}).a[1];
    leftEdge.push_back({-1.f, height, frontBack, 1.f});
    rightEdge.push_back({1.f, height, frontBack, 1.f});
  }

  // The parabola crests at the centre line, so the surface has a front half
  // facing the room's front and a back half falling away from it -- the same
  // shape as the tent, sampled instead of faceted. Shade it the same way: the
  // back half recedes, the front half faces. Both halves keep the crest sample,
  // so they meet exactly at the ridge with no seam between them.
  const size_t kCrest = ElevationSurfaces::frontBackSplitIndex(leftEdge);
  const size_t kFrontEnd = std::min(kCrest + 1, leftEdge.size());

  g.setColour(elevationRecedingSurface());
  g.fillPath(ElevationSurfaces::sampledEdgesToPath(
      kTransformMat_, window, {leftEdge.begin() + kCrest, leftEdge.end()},
      {rightEdge.begin() + kCrest, rightEdge.end()}));

  g.setColour(elevationFacingSurface());
  g.fillPath(ElevationSurfaces::sampledEdgesToPath(
      kTransformMat_, window, {leftEdge.begin(), leftEdge.begin() + kFrontEnd},
      {rightEdge.begin(), rightEdge.begin() + kFrontEnd}));
}

void AudioElementPluginTopView::paintDomeElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // A filled circle marking the dome's outer bound -- the region
  // getDomeElevationPtClamped clamps x and y into -- so the circle shows where
  // the source can actually sit. Sampling the boundary through that same
  // function is what ties the radius to the clamp rather than to a constant:
  // on the unit circle it returns the sampled (x, y) unclamped at floor
  // height.
  // The hemisphere crests at the centre of the room, so like the tent and the
  // arch it has a half facing the room's front and a half falling away from it.
  // Drawn as two half-discs in the same two shades, split across the centre
  // line. Sampling each half over its own angular range rather than
  // partitioning one full sweep is what makes the shared edge the exact
  // diameter: cos(0) and cos(pi) are exactly +/-1, so both halves close on the
  // same chord and no seam opens along it. The step is pi/40 per half, so the
  // union is the same 81 boundary positions the full sweep visited and the
  // radius is unchanged.
  const int kSamplesPerHalf = 41;
  const auto boundAnchorAt = [](const float theta) {
    const Coordinates::Point3D boundPt =
        ElevationListener::getDomeElevationPtClamped(
            {std::cos(theta), std::sin(theta), 0.f}, {});
    return Coordinates::Point4D{boundPt.a[0], boundPt.a[1], boundPt.a[2], 1.f};
  };

  std::vector<Coordinates::Point4D> backHalf, frontHalf;
  backHalf.reserve(kSamplesPerHalf);
  frontHalf.reserve(kSamplesPerHalf);
  for (int i = 0; i < kSamplesPerHalf; ++i) {
    // sin(theta) is the front/back coordinate, so [0, pi) is the back half and
    // [pi, 2pi) the front half.
    const float theta =
        i * juce::MathConstants<float>::pi / (kSamplesPerHalf - 1);
    backHalf.push_back(boundAnchorAt(theta));
    frontHalf.push_back(boundAnchorAt(theta + juce::MathConstants<float>::pi));
  }

  g.setColour(elevationRecedingSurface());
  g.fillPath(
      ElevationSurfaces::anchorsToPath(kTransformMat_, window, backHalf));

  g.setColour(elevationFacingSurface());
  g.fillPath(
      ElevationSurfaces::anchorsToPath(kTransformMat_, window, frontHalf));
}

void AudioElementPluginTopView::paintCurveElevation(
    const Coordinates::WindowData& window, juce::Graphics& g) {
  // The same treatment as Arch, against the logarithmic curve, at the existing
  // sample count.
  //
  // Sign convention: ElevationListener negates the front/back POSITION
  // PARAMETER before calling getCurveElevationPt, and room-view NDC z is that
  // same parameter negated (Coordinates::toRoomNdc). The value the listener
  // passes is therefore the NDC front/back coordinate itself, so sampling at
  // frontBack un-negated is what makes the drawn surface agree with the height
  // the listener derives. The curve is not symmetric about 0, so this sign
  // matters here where it does not for Tent, Arch, or Dome.
  const int kNumSamples = 32;
  std::vector<Coordinates::Point4D> leftEdge, rightEdge;
  leftEdge.reserve(kNumSamples);
  rightEdge.reserve(kNumSamples);
  for (int i = 0; i < kNumSamples; ++i) {
    const float frontBack = -1.f + i * 2.f / (kNumSamples - 1);
    const float height =
        ElevationListener::getCurveElevationPt({-1.f, frontBack, 0.f}).a[1];
    leftEdge.push_back({-1.f, height, frontBack, 1.f});
    rightEdge.push_back({1.f, height, frontBack, 1.f});
  }

  g.setColour(elevationFacingSurface());
  g.fillPath(ElevationSurfaces::sampledEdgesToPath(kTransformMat_, window,
                                                   leftEdge, rightEdge));
}
