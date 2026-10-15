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

// The panner's keyboard and scroll-wheel input mapping.
//
// The front/back nudge sign is the reason this file exists. It is signed
// against the SCREEN, and the chain that decides it runs through three places
// (the front speakers' room-view NDC z, toRoomNdc's negation of front/back,
// and getTopViewTransform's own sign), so it is not readable off the axis
// name. windowX and windowY below assert it against the real transform rather
// than against a restatement of any one of those coefficients -- get it wrong
// and the up arrow drives the source toward the back of the room, which is
// silent in the plugin and only shows up in a DAW.

// Pull in the umbrella header first: several components/src headers include
// components.h themselves and only resolve correctly once the umbrella has
// fully loaded once. See the same note in Coordinates_test.cpp.
// clang-format off
#include <components/components.h>

#include "components/src/room_views/Coordinates.h"
#include "components/src/room_views/PannerInput.h"
#include "components/src/room_views/SpeakerLookup.h"
#include "data_structures/src/Elevation.h"
// clang-format on

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace {

using Elevation = AudioElementSpatialLayout::Elevation;

// Matches the shape paint() builds from the component bounds.
const Coordinates::WindowData kWindow = {.leftCornerX = 0.f,
                                         .bottomCornerY = 400.f,
                                         .width = 400.f,
                                         .height = 400.f};

// The window y a position parameter triple projects to. Window y grows
// downward, so a SMALLER value is higher on screen.
float windowY(const int leftRight, const int frontBack, const int height) {
  return Coordinates::toWindow(
             Coordinates::getTopViewTransform(), kWindow,
             Coordinates::toRoomNdc((float)leftRight, (float)frontBack,
                                    (float)height))
      .a[1];
}

// The window x a position parameter triple projects to.
float windowX(const int leftRight, const int frontBack, const int height) {
  return Coordinates::toWindow(
             Coordinates::getTopViewTransform(), kWindow,
             Coordinates::toRoomNdc((float)leftRight, (float)frontBack,
                                    (float)height))
      .a[0];
}

// Where a speaker declared in room-view NDC is drawn, under the real top-view
// transform. Speaker hit-testing has to agree with the draw path, so the tests
// project through it rather than restating window positions.
juce::Point<float> speakerWindowPos(const Coordinates::Point4D& ndcPos) {
  const Coordinates::Point2D kPos = Coordinates::toWindow(
      Coordinates::getTopViewTransform(), kWindow, ndcPos);
  return {kPos.a[0], kPos.a[1]};
}

// The top view's hidden set: LFE sits at the room origin and is not a
// positional target, so AudioElementPluginTopView never draws it.
bool drawnInTopView(const SpeakerLookup::RoomViewSpeaker& speaker) {
  return speaker.tag != SpeakerLookup::kLFE;
}

// The speaker targets a layout presents to the panner, in layout order.
std::vector<PannerInput::SpeakerTarget> targetsForLayout(
    const Speakers::AudioElementSpeakerLayout layout) {
  std::vector<PannerInput::SpeakerTarget> targets;
  for (const SpeakerLookup::RoomViewSpeaker& kSpeaker :
       SpeakerLookup::getRoomViewSpeakers(layout)) {
    targets.push_back(
        {speakerWindowPos(kSpeaker.pos), drawnInTopView(kSpeaker)});
  }
  return targets;
}

}  // namespace

// The left and right arrows move the source one step along left/right.
TEST(PannerInputTest, arrowKeysNudgeLeftRightOneStep) {
  const PannerInput::Nudge kLeft =
      PannerInput::nudgeForKeyCode(juce::KeyPress::leftKey);
  const PannerInput::Nudge kRight =
      PannerInput::nudgeForKeyCode(juce::KeyPress::rightKey);

  EXPECT_EQ(kLeft.axis, PannerInput::Axis::kLeftRight);
  EXPECT_EQ(kLeft.steps, -1);
  EXPECT_EQ(kRight.axis, PannerInput::Axis::kLeftRight);
  EXPECT_EQ(kRight.steps, 1);
}

// The up and down arrows move the source one step along front/back.
TEST(PannerInputTest, arrowKeysNudgeFrontBackOneStep) {
  const PannerInput::Nudge kUp =
      PannerInput::nudgeForKeyCode(juce::KeyPress::upKey);
  const PannerInput::Nudge kDown =
      PannerInput::nudgeForKeyCode(juce::KeyPress::downKey);

  EXPECT_EQ(kUp.axis, PannerInput::Axis::kFrontBack);
  EXPECT_EQ(kDown.axis, PannerInput::Axis::kFrontBack);
  EXPECT_EQ(kUp.steps, -kDown.steps);
}

// The up arrow moves the source toward the top of the view and the down arrow
// toward the bottom, under the real top-view transform.
TEST(PannerInputTest, upArrowMovesTheSourceTowardTheTopOfTheView) {
  const int kUpSteps =
      PannerInput::nudgeForKeyCode(juce::KeyPress::upKey).steps;
  const int kDownSteps =
      PannerInput::nudgeForKeyCode(juce::KeyPress::downKey).steps;

  EXPECT_LT(windowY(0, kUpSteps, 0), windowY(0, 0, 0));
  EXPECT_GT(windowY(0, kDownSteps, 0), windowY(0, 0, 0));
}

// The top of the view is the front of the room, so the up arrow agrees with
// where the front speakers are drawn.
TEST(PannerInputTest, theTopOfTheViewIsTheFrontOfTheRoom) {
  // The centre speaker is the room's front, at room-view NDC z -1.
  const float kCentreSpeakerWindowY =
      Coordinates::toWindow(Coordinates::getTopViewTransform(), kWindow,
                            Coordinates::Point4D{0.f, 0.f, -1.f, 1.f})
          .a[1];
  const int kUpSteps =
      PannerInput::nudgeForKeyCode(juce::KeyPress::upKey).steps;

  EXPECT_LT(kCentreSpeakerWindowY, windowY(0, 0, 0));
  EXPECT_LT(windowY(0, kUpSteps, 0), windowY(0, 0, 0));
}

// The right arrow moves the source toward the right of the view.
TEST(PannerInputTest, rightArrowMovesTheSourceTowardTheRightOfTheView) {
  const int kRightSteps =
      PannerInput::nudgeForKeyCode(juce::KeyPress::rightKey).steps;

  EXPECT_GT(windowX(kRightSteps, 0, 0), windowX(0, 0, 0));
}

// A key that is not an arrow is not a panner gesture and is left alone.
TEST(PannerInputTest, otherKeysAreNotNudges) {
  for (const int kKeyCode :
       {(int)juce::KeyPress::spaceKey, (int)juce::KeyPress::returnKey,
        (int)juce::KeyPress::tabKey, (int)'a'}) {
    const PannerInput::Nudge kNudge = PannerInput::nudgeForKeyCode(kKeyCode);
    EXPECT_EQ(kNudge.axis, PannerInput::Axis::kNone);
    EXPECT_EQ(kNudge.steps, 0);
  }
}

// A wheel notch emits exactly one height step.
TEST(PannerInputTest, oneWheelNotchIsOneStep) {
  float accumulator = 0.f;

  EXPECT_EQ(PannerInput::stepsForWheelDelta(0.1f, accumulator), 1);
  EXPECT_EQ(PannerInput::stepsForWheelDelta(-0.1f, accumulator), -1);
}

// Successive notches each emit one step rather than banking travel into a
// later burst.
TEST(PannerInputTest, successiveNotchesEachEmitOneStep) {
  float accumulator = 0.f;

  for (int notch = 0; notch < 5; ++notch) {
    EXPECT_EQ(PannerInput::stepsForWheelDelta(0.15f, accumulator), 1);
  }
}

// A trackpad's sub-notch deltas accumulate to one step instead of being
// discarded.
TEST(PannerInputTest, subNotchDeltasAccumulateToOneStep) {
  float accumulator = 0.f;
  const float kSmall = PannerInput::kWheelNotchDelta / 4.f;

  EXPECT_EQ(PannerInput::stepsForWheelDelta(kSmall, accumulator), 0);
  EXPECT_EQ(PannerInput::stepsForWheelDelta(kSmall, accumulator), 0);
  EXPECT_EQ(PannerInput::stepsForWheelDelta(kSmall, accumulator), 0);
  EXPECT_EQ(PannerInput::stepsForWheelDelta(kSmall, accumulator), 1);
}

// Reversing direction below a notch cancels rather than emitting a step.
TEST(PannerInputTest, reversedSubNotchTravelCancels) {
  float accumulator = 0.f;
  const float kSmall = PannerInput::kWheelNotchDelta / 2.f;

  EXPECT_EQ(PannerInput::stepsForWheelDelta(kSmall, accumulator), 0);
  EXPECT_EQ(PannerInput::stepsForWheelDelta(-kSmall, accumulator), 0);
  EXPECT_FLOAT_EQ(accumulator, 0.f);
}

// A step resets the accumulator, so no remainder carries into the next event.
TEST(PannerInputTest, emittingAStepResetsTheAccumulator) {
  float accumulator = 0.f;

  EXPECT_EQ(PannerInput::stepsForWheelDelta(1.f, accumulator), 1);
  EXPECT_FLOAT_EQ(accumulator, 0.f);
}

// The four derived elevation patterns own height, so the wheel stays out of
// them.
TEST(PannerInputTest, derivedElevationPatternsOwnHeight) {
  EXPECT_TRUE(PannerInput::elevationOwnsHeight(Elevation::kTent));
  EXPECT_TRUE(PannerInput::elevationOwnsHeight(Elevation::kArch));
  EXPECT_TRUE(PannerInput::elevationOwnsHeight(Elevation::kDome));
  EXPECT_TRUE(PannerInput::elevationOwnsHeight(Elevation::kCurve));
}

// kNone and kFlat leave height to the user, the same two patterns for which
// PositionSelectionScreen keeps the height dial enabled.
TEST(PannerInputTest, flatAndNoPatternLeaveHeightToTheUser) {
  EXPECT_FALSE(PannerInput::elevationOwnsHeight(Elevation::kNone));
  EXPECT_FALSE(PannerInput::elevationOwnsHeight(Elevation::kFlat));
}

// A press inside a speaker's drawn rectangle selects it.
TEST(PannerInputTest, aPressOnASpeakerSelectsIt) {
  const std::vector<PannerInput::SpeakerTarget> kTargets = {
      {{100.f, 100.f}, true}};

  EXPECT_EQ(PannerInput::speakerIndexAt(kTargets, {100.f, 100.f}), 0);
  EXPECT_EQ(PannerInput::speakerIndexAt(
                kTargets, {100.f + PannerInput::kSpeakerWidth / 2.f, 100.f}),
            0);
  EXPECT_EQ(PannerInput::speakerIndexAt(
                kTargets, {100.f, 100.f - PannerInput::kSpeakerHeight / 2.f}),
            0);
}

// A press outside the rectangle and its margin selects nothing, so a press in
// open floor still reaches the drag path.
TEST(PannerInputTest, aPressAwayFromEverySpeakerSelectsNothing) {
  const std::vector<PannerInput::SpeakerTarget> kTargets = {
      {{100.f, 100.f}, true}};
  const float kOutsideX = 100.f + PannerInput::kSpeakerWidth / 2.f +
                          PannerInput::kSpeakerHitMargin + 1.f;
  const float kOutsideY = 100.f + PannerInput::kSpeakerHeight / 2.f +
                          PannerInput::kSpeakerHitMargin + 1.f;

  EXPECT_EQ(PannerInput::speakerIndexAt(kTargets, {kOutsideX, 100.f}),
            PannerInput::kNoSpeaker);
  EXPECT_EQ(PannerInput::speakerIndexAt(kTargets, {100.f, kOutsideY}),
            PannerInput::kNoSpeaker);
}

// An undrawn speaker is never a snap target, whatever it overlaps.
TEST(PannerInputTest, undrawnSpeakersAreNotSelectable) {
  const std::vector<PannerInput::SpeakerTarget> kTargets = {
      {{100.f, 100.f}, false}};

  EXPECT_EQ(PannerInput::speakerIndexAt(kTargets, {100.f, 100.f}),
            PannerInput::kNoSpeaker);
}

// Where two speakers overlap, the one whose centre is nearer wins.
TEST(PannerInputTest, theNearerSpeakerWinsAnOverlap) {
  const std::vector<PannerInput::SpeakerTarget> kTargets = {
      {{100.f, 100.f}, true}, {{104.f, 100.f}, true}};

  EXPECT_EQ(PannerInput::speakerIndexAt(kTargets, {101.f, 100.f}), 0);
  EXPECT_EQ(PannerInput::speakerIndexAt(kTargets, {103.f, 100.f}), 1);
}

// A layout that repeats a tag still resolves each drawn speaker to its own
// entry: k3Point1Point2 lists kLowFreqEffects twice, so a tag cannot identify
// one and only the index can.
TEST(PannerInputTest, aRepeatedTagStillSelectsTheRightSpeaker) {
  const std::vector<SpeakerLookup::RoomViewSpeaker> kSpeakers =
      SpeakerLookup::getRoomViewSpeakers(Speakers::k3Point1Point2);
  const std::vector<PannerInput::SpeakerTarget> kTargets =
      targetsForLayout(Speakers::k3Point1Point2);

  int repeatedTags = 0;
  for (size_t i = 0; i < kSpeakers.size(); ++i) {
    for (size_t j = i + 1; j < kSpeakers.size(); ++j) {
      repeatedTags += kSpeakers[i].tag == kSpeakers[j].tag ? 1 : 0;
    }
  }
  ASSERT_GT(repeatedTags, 0);

  for (size_t i = 0; i < kSpeakers.size(); ++i) {
    if (!kTargets[i].drawn) {
      continue;
    }
    EXPECT_EQ(PannerInput::speakerIndexAt(kTargets, kTargets[i].pos), (int)i);
  }
}

// LFE is repeated in that layout and hidden in both places, so neither entry
// can pull the source to the room origin.
TEST(PannerInputTest, aHiddenSpeakerIsSkippedAtEveryIndexItAppears) {
  const std::vector<SpeakerLookup::RoomViewSpeaker> kSpeakers =
      SpeakerLookup::getRoomViewSpeakers(Speakers::k3Point1Point2);
  const std::vector<PannerInput::SpeakerTarget> kTargets =
      targetsForLayout(Speakers::k3Point1Point2);

  int hidden = 0;
  for (size_t i = 0; i < kSpeakers.size(); ++i) {
    if (kSpeakers[i].tag != SpeakerLookup::kLFE) {
      continue;
    }
    ++hidden;
    EXPECT_NE(PannerInput::speakerIndexAt(kTargets, kTargets[i].pos), (int)i);
  }
  EXPECT_EQ(hidden, 2);
}

// The index the hit test returns reaches the speaker's own room position, so
// the click writes where that speaker actually is.
TEST(PannerInputTest, theSelectedIndexNamesThatSpeakersPosition) {
  const std::vector<SpeakerLookup::RoomViewSpeaker> kSpeakers =
      SpeakerLookup::getRoomViewSpeakers(Speakers::k5Point1Point4);
  const std::vector<PannerInput::SpeakerTarget> kTargets =
      targetsForLayout(Speakers::k5Point1Point4);

  for (size_t i = 0; i < kSpeakers.size(); ++i) {
    if (!kTargets[i].drawn) {
      continue;
    }
    const int kIndex = PannerInput::speakerIndexAt(kTargets, kTargets[i].pos);
    ASSERT_NE(kIndex, PannerInput::kNoSpeaker);
    const Coordinates::PositionParameters kSelected =
        Coordinates::fromRoomNdc(kSpeakers[kIndex].pos);
    const Coordinates::PositionParameters kExpected =
        Coordinates::fromRoomNdc(kSpeakers[i].pos);
    EXPECT_EQ(kSelected.x, kExpected.x);
    EXPECT_EQ(kSelected.y, kExpected.y);
    EXPECT_EQ(kSelected.z, kExpected.z);
  }
}

// A top-layer speaker carries a non-zero height, which is what a click under
// no elevation pattern writes to the height parameter.
TEST(PannerInputTest, aTopLayerSpeakerCarriesANonZeroHeight) {
  const Coordinates::PositionParameters kTopFront =
      Coordinates::fromRoomNdc(SpeakerLookup::kLeftTopFront.pos);

  EXPECT_EQ(kTopFront.z, 25);
}

// Only kNone leaves height to the click. Flat holds the source on its plane,
// and the derived patterns compute height from the position instead.
TEST(PannerInputTest, onlyNoPatternLetsAClickSetHeight) {
  EXPECT_TRUE(PannerInput::clickSetsHeight(Elevation::kNone));
  EXPECT_FALSE(PannerInput::clickSetsHeight(Elevation::kFlat));
  EXPECT_FALSE(PannerInput::clickSetsHeight(Elevation::kTent));
  EXPECT_FALSE(PannerInput::clickSetsHeight(Elevation::kArch));
  EXPECT_FALSE(PannerInput::clickSetsHeight(Elevation::kDome));
  EXPECT_FALSE(PannerInput::clickSetsHeight(Elevation::kCurve));
}

// The dome is the only pattern with a bounded floor plan; the rest are height
// fields over the whole room and must not move a legal pointer target.
TEST(PannerInputTest, onlyTheDomeConstrainsTheFloorPlan) {
  const Coordinates::PositionParameters kCorner = {50, 50, 10};

  for (const Elevation kElevation :
       {Elevation::kNone, Elevation::kFlat, Elevation::kTent, Elevation::kArch,
        Elevation::kCurve}) {
    const Coordinates::PositionParameters kClamped =
        PannerInput::clampToElevationPlan(kElevation, kCorner);
    EXPECT_EQ(kClamped.x, kCorner.x);
    EXPECT_EQ(kClamped.y, kCorner.y);
    EXPECT_EQ(kClamped.z, kCorner.z);
  }
}

// A target already inside the dome is returned untouched, so an ordinary drag
// keeps the exact integer fromRoomNdc produced.
TEST(PannerInputTest, domeLeavesAnInsideTargetAlone) {
  const Coordinates::PositionParameters kInside = {20, -30, 7};
  const Coordinates::PositionParameters kClamped =
      PannerInput::clampToElevationPlan(Elevation::kDome, kInside);

  EXPECT_EQ(kClamped.x, kInside.x);
  EXPECT_EQ(kClamped.y, kInside.y);
  EXPECT_EQ(kClamped.z, kInside.z);
}

// No target, anywhere in or beyond the room, comes back outside the circle.
TEST(PannerInputTest, domeNeverReturnsAPointOutsideTheCircle) {
  for (int x = -80; x <= 80; ++x) {
    for (int y = -80; y <= 80; ++y) {
      const Coordinates::PositionParameters kClamped =
          PannerInput::clampToElevationPlan(Elevation::kDome, {x, y, 0});
      const float kRadius = std::hypot((float)kClamped.x, (float)kClamped.y);
      EXPECT_LE(kRadius, Coordinates::kPositionExtent)
          << "target (" << x << ", " << y << ") clamped to (" << kClamped.x
          << ", " << kClamped.y << ")";
    }
  }
}

// The clamp is radial, so the source stays in the direction the pointer is in
// rather than sliding along the rim. A 45-degree target keeps both axes equal.
TEST(PannerInputTest, domeClampHoldsTheDirectionOfThePointer) {
  const Coordinates::PositionParameters kClamped =
      PannerInput::clampToElevationPlan(Elevation::kDome, {70, 70, 0});

  EXPECT_EQ(kClamped.x, kClamped.y);
  // 50 / sqrt(2) = 35.355...; 36 would leave the circle.
  EXPECT_EQ(kClamped.x, 35);
}

// The clamp lands in the dome's rim band: no target outside the circle comes
// back one parameter step or more inside it.
TEST(PannerInputTest, domeClampReachesTheBoundary) {
  for (int x = -80; x <= 80; ++x) {
    for (int y = -80; y <= 80; ++y) {
      if (std::hypot((float)x, (float)y) <= Coordinates::kPositionExtent) {
        continue;
      }
      const Coordinates::PositionParameters kClamped =
          PannerInput::clampToElevationPlan(Elevation::kDome, {x, y, 0});
      const float kRadius = std::hypot((float)kClamped.x, (float)kClamped.y);
      EXPECT_GT(kRadius, Coordinates::kPositionExtent - 1.f)
          << "target (" << x << ", " << y << ") clamped to (" << kClamped.x
          << ", " << kClamped.y << ")";
    }
  }
}

// Height is the pattern's to derive, so the clamp passes it through unchanged
// even when it moves the horizontal axes.
TEST(PannerInputTest, domeClampLeavesHeightToThePattern) {
  const Coordinates::PositionParameters kClamped =
      PannerInput::clampToElevationPlan(Elevation::kDome, {70, 70, -25});

  EXPECT_EQ(kClamped.z, -25);
}

// A drag pinned at the rim reads floor height, whichever direction it left
// the circle in.
TEST(PannerInputTest, domeClampedDragSitsAtTheFloor) {
  for (int x = -80; x <= 80; ++x) {
    for (int y = -80; y <= 80; ++y) {
      if (std::hypot((float)x, (float)y) <= Coordinates::kPositionExtent) {
        continue;
      }
      const Coordinates::PositionParameters kClamped =
          PannerInput::clampToElevationPlan(Elevation::kDome, {x, y, 0});
      const Coordinates::Point3D kOnDome =
          ElevationListener::getDomeElevationPtClamped(
              {kClamped.x / Coordinates::kPositionExtent,
               kClamped.y / Coordinates::kPositionExtent, 0.f},
              {});
      EXPECT_EQ(kOnDome.a[1] * Coordinates::kPositionExtent, -50.f)
          << "target (" << x << ", " << y << ") clamped to (" << kClamped.x
          << ", " << kClamped.y << ")";
    }
  }
}
