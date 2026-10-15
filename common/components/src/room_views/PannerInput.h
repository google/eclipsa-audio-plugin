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
#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>
#include <limits>
#include <vector>

#include "Coordinates.h"
#include "data_structures/src/AudioElementSpatialLayout.h"

// Keyboard and scroll-wheel input mapping for the audio element panner.
//
// Held apart from AudioElementPluginTopView so it can be reached by tests:
// there is no JUCE UI test harness in this repo, so logic left inside the
// component is verifiable only by hand in a DAW.
//
// Placed in a header so it can be reached by tests.
namespace PannerInput {

// Which position parameter an input moves. kNone means the input is not a
// panner gesture and belongs to whatever else wants it.
enum class Axis { kNone, kLeftRight, kFrontBack, kHeight };

// One discrete input: the axis it moves and the signed number of parameter
// steps it moves it by.
struct Nudge {
  Axis axis = Axis::kNone;
  int steps = 0;
};

// A wheel notch arrives as one event carrying a fraction of a full scroll; a
// trackpad sends a stream of smaller values instead. Accumulating to this
// threshold gives both devices one step per notch-worth of travel.
constexpr float kWheelNotchDelta = 0.05f;

/**
 * @brief Map an arrow key to the position step it nudges.
 *
 * Front/back is signed against the SCREEN, not the parameter name. The top
 * view puts the front of the room at the top (the front speakers sit at
 * negative room-view NDC z) and Coordinates::toRoomNdc negates front/back on
 * the way in, so the front of the room is a POSITIVE front/back parameter and
 * the up arrow adds a step. Verify that against getTopViewTransform rather
 * than reading it off the axis name.
 *
 * @param keyCode a juce::KeyPress key code
 * @return Nudge the axis and signed steps, or Axis::kNone for any other key
 */
// Not a switch: JUCE's key codes are static const ints rather than constant
// expressions, so they cannot be case labels.
inline Nudge nudgeForKeyCode(const int keyCode) {
  if (keyCode == juce::KeyPress::leftKey) {
    return {Axis::kLeftRight, -1};
  }
  if (keyCode == juce::KeyPress::rightKey) {
    return {Axis::kLeftRight, 1};
  }
  if (keyCode == juce::KeyPress::upKey) {
    return {Axis::kFrontBack, 1};
  }
  if (keyCode == juce::KeyPress::downKey) {
    return {Axis::kFrontBack, -1};
  }
  return {};
}

/**
 * @brief Convert one wheel event into whole height steps.
 *
 * Carries the sub-notch remainder in the accumulator so a trackpad's stream of
 * small deltas adds up instead of being discarded, and resets on every step so
 * a long scroll cannot bank travel and fire a burst later.
 *
 * @param delta the event's deltaY
 * @param accumulator carried between events; reset when a step is emitted
 * @return int the signed step count, 0 while below a notch
 */
inline int stepsForWheelDelta(const float delta, float& accumulator) {
  accumulator += delta;
  if (std::abs(accumulator) < kWheelNotchDelta) {
    return 0;
  }
  const int kSteps = accumulator > 0.f ? 1 : -1;
  accumulator = 0.f;
  return kSteps;
}

/**
 * @brief Whether the elevation pattern derives height from position.
 *
 * ElevationListener recomputes z from x/y under Tent, Arch, Dome and Curve and
 * writes it back, so a wheel write in those modes is overwritten before it is
 * seen. It leaves z alone under kNone and kFlat -- the same two patterns for
 * which PositionSelectionScreen keeps the height dial enabled.
 *
 * @param elevation the active elevation pattern
 * @return bool true when the pattern owns height
 */
inline bool elevationOwnsHeight(
    const AudioElementSpatialLayout::Elevation elevation) {
  switch (elevation) {
    case AudioElementSpatialLayout::Elevation::kTent:
    case AudioElementSpatialLayout::Elevation::kArch:
    case AudioElementSpatialLayout::Elevation::kDome:
    case AudioElementSpatialLayout::Elevation::kCurve:
      return true;
    default:
      return false;
  }
}

// The speaker rectangle drawSpeaker fills: 10x15, anchored half a width left
// and half a height above the speaker's window position.
constexpr float kSpeakerWidth = 10.f;
constexpr float kSpeakerHeight = 15.f;
// Widens the target past the drawn edge, as the source marker's grab margin
// does.
constexpr float kSpeakerHitMargin = 2.f;

// One speaker as the hit test sees it: where it was drawn, and whether it was
// drawn at all. An undrawn speaker is never a snap target -- the source must
// not jump to something the user cannot see.
struct SpeakerTarget {
  juce::Point<float> pos;
  bool drawn = true;
};

constexpr int kNoSpeaker = -1;

/**
 * @brief Index of the drawn speaker a press lands on, or kNoSpeaker.
 *
 * Returns an INDEX because a tag does not identify a speaker: several layouts
 * repeat one (k3Point1Point2 lists kLowFreqEffects twice, and kLU045/kRU045
 * reuse kLTF/kRTF). The caller's window-position and room-position lists are
 * built one-to-one in the same order, so the index is what carries identity
 * between them.
 *
 * Nearest centre wins where two rectangles overlap, so the speaker the user
 * aimed at moves the source rather than whichever came first in the layout.
 *
 * @param targets the drawn speakers, in layout order
 * @param windowPoint the press position in window coordinates
 * @return int an index into targets, or kNoSpeaker
 */
inline int speakerIndexAt(const std::vector<SpeakerTarget>& targets,
                          const juce::Point<float>& windowPoint) {
  constexpr float kHalfWidth = kSpeakerWidth / 2.f + kSpeakerHitMargin;
  constexpr float kHalfHeight = kSpeakerHeight / 2.f + kSpeakerHitMargin;

  int nearest = kNoSpeaker;
  float nearestDistance = std::numeric_limits<float>::max();
  for (int i = 0; i < (int)targets.size(); ++i) {
    if (!targets[i].drawn) {
      continue;
    }
    const float kDx = windowPoint.x - targets[i].pos.x;
    const float kDy = windowPoint.y - targets[i].pos.y;
    if (std::abs(kDx) > kHalfWidth || std::abs(kDy) > kHalfHeight) {
      continue;
    }
    const float kDistance = kDx * kDx + kDy * kDy;
    if (kDistance < nearestDistance) {
      nearest = i;
      nearestDistance = kDistance;
    }
  }
  return nearest;
}

/**
 * @brief Whether a speaker click sets height as well as left/right and
 * front/back.
 *
 * kNone is the only pattern that leaves height entirely to the user, so it is
 * the only one where a click can place the source in all three dimensions.
 * Under kFlat the source stays on the plane the pattern represents, and under
 * the derived patterns ElevationListener recomputes height from the new
 * position -- writing height in either case would fight the pattern.
 *
 * @param elevation the active elevation pattern
 * @return bool true when a click writes the height parameter
 */
inline bool clickSetsHeight(
    const AudioElementSpatialLayout::Elevation elevation) {
  return elevation == AudioElementSpatialLayout::Elevation::kNone;
}

/**
 * @brief Clamp a pointer target onto the floor plan the elevation pattern
 *        allows.
 *
 * Call this before writing, not after. ElevationListener bounds the dome too,
 * but only once the position is the parameter's value, so a correction there
 * has already been published to the host. It also gives on whichever axis did
 * not just change, which suits a single dial edit; a pointer moves both at
 * once, so its nearest legal point is the radial one.
 *
 * Picks the integer position nearest the rim that is still inside the circle,
 * so the result leaves nothing to correct and falls in the dome's rim band,
 * where ElevationListener holds the height at the floor.
 *
 * @param elevation the active elevation pattern
 * @param target a position in parameter space
 * @return Coordinates::PositionParameters target unchanged, or the nearest
 *         position the pattern allows
 */
inline Coordinates::PositionParameters clampToElevationPlan(
    const AudioElementSpatialLayout::Elevation elevation,
    const Coordinates::PositionParameters target) {
  // Only the dome bounds the floor plan; the others are height fields over the
  // whole room.
  if (elevation != AudioElementSpatialLayout::Elevation::kDome) {
    return target;
  }
  const float kRadius = std::hypot((float)target.x, (float)target.y);
  if (kRadius <= Coordinates::kPositionExtent) {
    return target;
  }
  const float kScale = Coordinates::kPositionExtent / kRadius;
  const float kRimX = target.x * kScale;
  const float kRimY = target.y * kScale;

  // The integer neighbour of the rim point farthest out that is still inside
  // the circle, so the source lands in the dome's rim band.
  Coordinates::PositionParameters best = {0, 0, target.z};
  int bestRadiusSq = -1;
  for (const float kX : {std::floor(kRimX), std::ceil(kRimX)}) {
    for (const float kY : {std::floor(kRimY), std::ceil(kRimY)}) {
      const int kRadiusSq = (int)(kX * kX + kY * kY);
      const int kExtentSq =
          (int)(Coordinates::kPositionExtent * Coordinates::kPositionExtent);
      if (kRadiusSq <= kExtentSq && kRadiusSq > bestRadiusSq) {
        best = {(int)kX, (int)kY, target.z};
        bestRadiusSq = kRadiusSq;
      }
    }
  }
  return best;
}

}  // namespace PannerInput
