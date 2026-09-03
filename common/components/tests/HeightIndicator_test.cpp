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

// The geometry behind the top-down panner's height indicator. Two classes of
// mistake here are silent in the plugin and loud in these tests: an anchor
// placed off the room bounds (the outline stops resting on the room's sides),
// and a connector endpoint that does not share the outline's height (it
// detaches from the outline as soon as the perspective divide scales the two
// differently).
//
// The perspective properties are asserted against the REAL top-view transform
// rather than an identity one, because they are properties OF that transform.
// Everything that only concerns anchor placement is asserted on the anchors
// directly, where the expected values are exact.

// Pull in the umbrella header first: several components/src headers include
// components.h themselves and only resolve correctly once the umbrella has
// fully loaded once. See the same note in Coordinates_test.cpp.
// clang-format off
#include <components/components.h>

#include "components/src/room_views/HeightIndicator.h"
// clang-format on

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace {
constexpr float kTolerance = 1e-4f;

// A 200x100 window, matching the shape paint() builds from the component
// bounds, and a second one of a different aspect for the resize checks.
const Coordinates::WindowData kWindow = {.leftCornerX = 0.f,
                                         .bottomCornerY = 100.f,
                                         .width = 200.f,
                                         .height = 100.f};
const Coordinates::WindowData kResizedWindow = {.leftCornerX = 0.f,
                                                .bottomCornerY = 640.f,
                                                .width = 480.f,
                                                .height = 640.f};

Coordinates::Mat4 topView() { return Coordinates::getTopViewTransform(); }

// The projected width of the cross-section outline, taken across its front
// edge -- the quantity the "expands toward the walls" behaviour is about.
float projectedOutlineWidth(const Coordinates::WindowData& window,
                            const float height) {
  const std::array<Coordinates::Point2D, 2> kFrontEdge =
      HeightIndicator::projectSegment(
          topView(), window, HeightIndicator::crossSectionOutline(height)[0]);
  return std::abs(kFrontEdge[1].a[0] - kFrontEdge[0].a[0]);
}

// The tent's surface height at one front/back position: the ridge at the centre
// line falling linearly to the floor at either bound, which is the shape
// paintTentElevation draws from its six anchors.
float tentRoofAt(const float, const float frontBack) {
  return 1.f - std::abs(frontBack) * 2.f;
}

// The dome's hemisphere: the one surface that falls away in left/right as well
// as front/back, and therefore the one under which BOTH connectors can cross.
// Outside the unit circle it meets the floor, as the real one does at its rim.
float domeRoofAt(const float leftRight, const float frontBack) {
  const float kRadiusSq = leftRight * leftRight + frontBack * frontBack;
  if (kRadiusSq >= 1.f) {
    return -1.f;
  }
  return 2.f * std::sqrt(1.f - kRadiusSq) - 1.f;
}

// A monotonic surface, standing in for the logarithmic curve. It crosses any
// one height ONCE, so it is what distinguishes a split that finds crossings
// from one that assumes the symmetric pair the tent and arch happen to have.
float monotonicRoofAt(const float, const float frontBack) { return frontBack; }

// A segment's length in room-view NDC, for the coverage invariant below.
float segmentLength(const HeightIndicator::Segment& segment) {
  const float kDx = segment.end.a[0] - segment.start.a[0];
  const float kDy = segment.end.a[1] - segment.start.a[1];
  const float kDz = segment.end.a[2] - segment.start.a[2];
  return std::sqrt(kDx * kDx + kDy * kDy + kDz * kDz);
}

// Whether any run ends on the room's right bound -- the right-edge connector's
// far end, and the one thing that tells it apart from the back-edge connector's
// runs, which all hold the source's left/right position.
bool anyRunReachesTheRightBound(
    const std::vector<HeightIndicator::Segment>& runs) {
  for (const HeightIndicator::Segment& run : runs) {
    if (std::abs(run.end.a[0] - 1.f) < kTolerance) {
      return true;
    }
  }
  return false;
}

// The total length of every run in a split. The four sides are each 2 long in
// NDC, so a split that loses or double-counts a run fails against 8.
float totalRunLength(const HeightIndicator::SplitOutline& split) {
  float total = 0.f;
  for (const HeightIndicator::Segment& run : split.below) {
    total += segmentLength(run);
  }
  for (const HeightIndicator::Segment& run : split.above) {
    total += segmentLength(run);
  }
  return total;
}
}  // namespace

// The outline's anchors rest on the room's sides at the requested height: every
// corner is at a room bound in left/right and front/back, and at that height.
TEST(HeightIndicatorTest, outlineAnchorsSitOnTheRoomBoundsAtTheGivenHeight) {
  const float kHeight = 0.4f;
  const std::array<HeightIndicator::Segment, 4> kOutline =
      HeightIndicator::crossSectionOutline(kHeight);

  for (const HeightIndicator::Segment& side : kOutline) {
    for (const Coordinates::Point4D& anchor : {side.start, side.end}) {
      EXPECT_NEAR(std::abs(anchor.a[0]), 1.f, kTolerance);
      EXPECT_NEAR(anchor.a[1], kHeight, kTolerance);
      EXPECT_NEAR(std::abs(anchor.a[2]), 1.f, kTolerance);
      EXPECT_NEAR(anchor.a[3], 1.f, kTolerance);
    }
  }
}

// The four sides form one closed loop, so drawing them puts a line along every
// side rather than leaving a gap or retracing an edge.
TEST(HeightIndicatorTest, outlineSidesFormAClosedLoop) {
  const std::array<HeightIndicator::Segment, 4> kOutline =
      HeightIndicator::crossSectionOutline(-0.2f);

  for (size_t i = 0; i < kOutline.size(); ++i) {
    const Coordinates::Point4D& kNextStart =
        kOutline[(i + 1) % kOutline.size()].start;
    EXPECT_NEAR(kOutline[i].end.a[0], kNextStart.a[0], kTolerance);
    EXPECT_NEAR(kOutline[i].end.a[2], kNextStart.a[2], kTolerance);
  }
}

// Height reaches the outline through the parameter-to-NDC mapping helper, so a
// z position parameter at its extent puts the outline at the room's ceiling --
// the check that would fail if the scale were open-coded and drifted.
TEST(HeightIndicatorTest, heightComesFromTheParameterMappingHelper) {
  const float kCeiling =
      Coordinates::toRoomNdc(0.f, 0.f, Coordinates::kPositionExtent).a[1];
  const float kFloor =
      Coordinates::toRoomNdc(0.f, 0.f, -Coordinates::kPositionExtent).a[1];

  EXPECT_NEAR(HeightIndicator::crossSectionOutline(kCeiling)[0].start.a[1], 1.f,
              kTolerance);
  EXPECT_NEAR(HeightIndicator::crossSectionOutline(kFloor)[0].start.a[1], -1.f,
              kTolerance);
}

// Both connectors start at the source and end on the outline: one on the back
// edge at the source's left/right position, one on the right edge at the
// source's front/back position.
TEST(HeightIndicatorTest, leaderLinesRunFromTheSourceToTheOutlineEdges) {
  const Coordinates::Point4D kSource = {-0.3f, 0.6f, 0.25f, 1.f};
  const std::array<HeightIndicator::Segment, 2> kLeaders =
      HeightIndicator::leaderLines(kSource);

  for (const HeightIndicator::Segment& leader : kLeaders) {
    EXPECT_NEAR(leader.start.a[0], kSource.a[0], kTolerance);
    EXPECT_NEAR(leader.start.a[1], kSource.a[1], kTolerance);
    EXPECT_NEAR(leader.start.a[2], kSource.a[2], kTolerance);
  }

  // The back edge is NDC z = +1, which this transform renders at the bottom of
  // the view; the connector meets it at the source's own left/right position.
  EXPECT_NEAR(kLeaders[0].end.a[0], kSource.a[0], kTolerance);
  EXPECT_NEAR(kLeaders[0].end.a[2], 1.f, kTolerance);

  // The right edge is NDC x = +1; the connector meets it at the source's own
  // front/back position.
  EXPECT_NEAR(kLeaders[1].end.a[0], 1.f, kTolerance);
  EXPECT_NEAR(kLeaders[1].end.a[2], kSource.a[2], kTolerance);
}

// Every connector endpoint shares the source's height, which is what keeps the
// connectors attached: an endpoint at any other height would take a different
// perspective divide from the outline and drift off it.
TEST(HeightIndicatorTest, leaderLineEndpointsShareTheSourceHeight) {
  for (const float height : {-1.f, -0.35f, 0.f, 0.5f, 1.f}) {
    const Coordinates::Point4D kSource = {0.42f, height, -0.7f, 1.f};
    for (const HeightIndicator::Segment& leader :
         HeightIndicator::leaderLines(kSource)) {
      EXPECT_NEAR(leader.start.a[1], height, kTolerance);
      EXPECT_NEAR(leader.end.a[1], height, kTolerance);
    }
  }
}

// Raising the source expands the projected outline toward the walls and
// lowering it contracts the outline, by exactly the ratio the transform's
// perspective divide implies. Nothing in the draw path scales it by hand.
TEST(HeightIndicatorTest,
     projectedOutlineExpandsWithHeightByTheTransformRatio) {
  const float kLow = -0.5f;
  const float kHigh = 0.75f;
  const float kLowWidth = projectedOutlineWidth(kWindow, kLow);
  const float kHighWidth = projectedOutlineWidth(kWindow, kHigh);

  EXPECT_GT(kHighWidth, kLowWidth);
  EXPECT_NEAR(kHighWidth / kLowWidth, (5.f - kLow) / (5.f - kHigh), kTolerance);
}

// The outline stays concentric with the room as it expands: its projected
// centre is the window centre at every height, because the transform puts the
// camera on the room's vertical axis.
TEST(HeightIndicatorTest, projectedOutlineStaysConcentricAtEveryHeight) {
  for (const float height : {-1.f, -0.25f, 0.f, 0.6f, 1.f}) {
    const std::array<Coordinates::Point2D, 2> kFrontEdge =
        HeightIndicator::projectSegment(
            topView(), kWindow,
            HeightIndicator::crossSectionOutline(height)[0]);
    EXPECT_NEAR((kFrontEdge[0].a[0] + kFrontEdge[1].a[0]) / 2.f,
                kWindow.leftCornerX + kWindow.width / 2.f, kTolerance);
  }
}

// A window resize keeps the outline on the room's sides and both connectors
// attached: each connector's far endpoint lands on its outline edge in the
// resized window exactly as it does in the original one.
TEST(HeightIndicatorTest, connectorsStayAttachedToTheOutlineAcrossWindowSizes) {
  const Coordinates::Point4D kSource = {-0.6f, 0.3f, 0.15f, 1.f};

  for (const Coordinates::WindowData& window : {kWindow, kResizedWindow}) {
    const std::array<HeightIndicator::Segment, 4> kOutline =
        HeightIndicator::crossSectionOutline(kSource.a[1]);
    const std::array<HeightIndicator::Segment, 2> kLeaders =
        HeightIndicator::leaderLines(kSource);

    // The back edge runs from (-1, h, 1) to (1, h, 1): the third side of the
    // outline. Its projected screen row is what the first connector must end
    // on.
    const std::array<Coordinates::Point2D, 2> kBackEdge =
        HeightIndicator::projectSegment(topView(), window, kOutline[2]);
    const std::array<Coordinates::Point2D, 2> kToBackEdge =
        HeightIndicator::projectSegment(topView(), window, kLeaders[0]);
    EXPECT_NEAR(kToBackEdge[1].a[1], kBackEdge[0].a[1], kTolerance);
    EXPECT_GE(kToBackEdge[1].a[0],
              std::min(kBackEdge[0].a[0], kBackEdge[1].a[0]));
    EXPECT_LE(kToBackEdge[1].a[0],
              std::max(kBackEdge[0].a[0], kBackEdge[1].a[0]));

    // The right edge runs from (1, h, -1) to (1, h, 1): the second side.
    const std::array<Coordinates::Point2D, 2> kRightEdge =
        HeightIndicator::projectSegment(topView(), window, kOutline[1]);
    const std::array<Coordinates::Point2D, 2> kToRightEdge =
        HeightIndicator::projectSegment(topView(), window, kLeaders[1]);
    EXPECT_NEAR(kToRightEdge[1].a[0], kRightEdge[0].a[0], kTolerance);
    EXPECT_GE(kToRightEdge[1].a[1],
              std::min(kRightEdge[0].a[1], kRightEdge[1].a[1]));
    EXPECT_LE(kToRightEdge[1].a[1],
              std::max(kRightEdge[0].a[1], kRightEdge[1].a[1]));
  }
}

// The split is what keeps the outline from drawing at full strength where it is
// actually behind a translucent elevation surface. The cases below are the ones
// that are silent in the plugin: a surface the outline never reaches, a surface
// it is wholly under, the two-crossing shape the tent and arch share, and the
// one-crossing shape the logarithmic curve has on its own.

// A surface everywhere below the outline leaves every run above it, so nothing
// is drawn before the fill -- the dome's floor-level disc and kNone both land
// here through the -1 sentinel.
TEST(HeightIndicatorTest, splitPutsEverythingAboveAFloorLevelSurface) {
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitAtElevation(
          0.f, [](const float, const float) { return -1.f; });

  EXPECT_TRUE(kSplit.below.empty());
  EXPECT_EQ(kSplit.above.size(), 4u);
  EXPECT_NEAR(totalRunLength(kSplit), 8.f, kTolerance);
}

// A surface everywhere above the outline puts every run below it, so the whole
// outline is drawn before the fill and the roof tints all of it.
TEST(HeightIndicatorTest, splitPutsEverythingBelowAHigherFlatSurface) {
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitAtElevation(
          -0.5f, [](const float, const float) { return 0.5f; });

  EXPECT_TRUE(kSplit.above.empty());
  EXPECT_EQ(kSplit.below.size(), 4u);
  EXPECT_NEAR(totalRunLength(kSplit), 8.f, kTolerance);
}

// A source resting exactly on the surface draws OVER it. ElevationListener
// clamps the source onto the surface for every pattern that has one, so this is
// the common case, and a strict comparison would flicker it between the lists.
TEST(HeightIndicatorTest, splitResolvesAnExactTieToAbove) {
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitAtElevation(
          0.25f, [](const float, const float) { return 0.25f; });

  EXPECT_TRUE(kSplit.below.empty());
  EXPECT_EQ(kSplit.above.size(), 4u);
}

// The tent crosses the outline twice along each side that runs front to back,
// and not at all along the two that sit at the room's front and back bounds --
// so the left and right sides split into three runs each and the other two stay
// whole. Six runs above, two below.
TEST(HeightIndicatorTest, splitCutsTheTentOnTheTwoFrontToBackSides) {
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitAtElevation(0.f, tentRoofAt);

  EXPECT_EQ(kSplit.above.size(), 6u);
  EXPECT_EQ(kSplit.below.size(), 2u);
  EXPECT_NEAR(totalRunLength(kSplit), 8.f, kTolerance);
}

// The crossing lands where the surface height equals the outline height, not at
// a sample boundary: the tent reaches 0 at front/back +/-0.5, so every run that
// changes list does so there. This is the assertion that would fail if the
// bisection were dropped for a nearest-sample split.
TEST(HeightIndicatorTest, splitCrossesWhereTheSurfaceMeetsTheOutlineHeight) {
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitAtElevation(0.f, tentRoofAt);

  // Each below-run spans the ridge, so both of its ends sit on a crossing.
  for (const HeightIndicator::Segment& run : kSplit.below) {
    EXPECT_NEAR(std::abs(run.start.a[2]), 0.5f, 1e-3f);
    EXPECT_NEAR(std::abs(run.end.a[2]), 0.5f, 1e-3f);
    EXPECT_NEAR(tentRoofAt(run.start.a[0], run.start.a[2]), 0.f, 1e-3f);
  }
}

// Raising the outline above the tent's ridge leaves nothing under it, and the
// runs collapse back to the four whole sides rather than to empty or duplicated
// ones.
TEST(HeightIndicatorTest, splitClearsTheTentOnceTheOutlineIsAboveTheRidge) {
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitAtElevation(1.f, tentRoofAt);

  EXPECT_TRUE(kSplit.below.empty());
  EXPECT_EQ(kSplit.above.size(), 4u);
  EXPECT_NEAR(totalRunLength(kSplit), 8.f, kTolerance);
}

// A monotonic surface crosses once per front-to-back side, so those sides split
// in two rather than three. The logarithmic curve is this shape, and a split
// hard-coded to the tent's symmetric pair would get it wrong.
TEST(HeightIndicatorTest, splitHandlesASurfaceThatCrossesOnlyOnce) {
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitAtElevation(0.f, monotonicRoofAt);

  // Front side (front/back -1) is above throughout, back side (+1) below;
  // each of the two front-to-back sides contributes one run to each list.
  EXPECT_EQ(kSplit.above.size(), 3u);
  EXPECT_EQ(kSplit.below.size(), 3u);
  EXPECT_NEAR(totalRunLength(kSplit), 8.f, kTolerance);

  for (const HeightIndicator::Segment& run : kSplit.above) {
    EXPECT_LE(std::min(run.start.a[2], run.end.a[2]), kTolerance);
  }
}

// Every run keeps the outline's height, so a split run still rests on the
// room's sides and still projects with the rest of the outline.
TEST(HeightIndicatorTest, splitRunsKeepTheOutlineHeight) {
  const float kHeight = -0.25f;
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitAtElevation(kHeight, tentRoofAt);

  for (const HeightIndicator::Segment& run : kSplit.below) {
    EXPECT_NEAR(run.start.a[1], kHeight, kTolerance);
    EXPECT_NEAR(run.end.a[1], kHeight, kTolerance);
  }
  for (const HeightIndicator::Segment& run : kSplit.above) {
    EXPECT_NEAR(run.start.a[1], kHeight, kTolerance);
    EXPECT_NEAR(run.end.a[1], kHeight, kTolerance);
  }
}

// The connectors go through the same split as the outline. These pin the cases
// that differ from the outline's: only one of the two can ever cross, and where
// the source rests on the surface the crossing sits on an endpoint.

// A surface everywhere below leaves both connectors whole and over it -- the
// dome and kNone case, and the check that the split adds no spurious cut.
TEST(HeightIndicatorTest, leaderSplitLeavesBothWholeAboveAFloorLevelSurface) {
  const Coordinates::Point4D kSource = {0.2f, 0.f, -0.5f, 1.f};
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitLeaderLinesAtElevation(
          kSource, [](const float, const float) { return -1.f; },
          /*splitRightEdge=*/false);

  EXPECT_TRUE(kSplit.below.empty());
  EXPECT_EQ(kSplit.above.size(), 2u);
}

// With the source on the tent's FRONT slope, the back-edge connector runs over
// the ridge and dips under it. The right-edge connector is coincident with the
// surface and is drawn on top unconditionally, so it contributes one whole run
// to `above`. One run below, two above.
TEST(HeightIndicatorTest, leaderSplitCutsOnlyTheBackEdgeConnector) {
  // The source rests ON the tent at front/back -0.5, where its height is 0.
  const Coordinates::Point4D kSource = {0.2f, 0.f, -0.5f, 1.f};
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitLeaderLinesAtElevation(kSource, tentRoofAt,
                                                   /*splitRightEdge=*/false);

  EXPECT_EQ(kSplit.below.size(), 1u);
  EXPECT_EQ(kSplit.above.size(), 2u);

  // The submerged run starts at the source and surfaces where the tent falls
  // back to the source's height, mirrored about the ridge at front/back +0.5.
  EXPECT_NEAR(kSplit.below[0].start.a[2], -0.5f, 1e-3f);
  EXPECT_NEAR(kSplit.below[0].end.a[2], 0.5f, 1e-3f);
}

// With the source on the tent's BACK slope the surface only falls away between
// it and the back edge, so nothing is submerged and both connectors stay whole.
// This is the half of the geometry the front-slope case cannot show.
TEST(HeightIndicatorTest, leaderSplitLeavesTheBackSlopeUncut) {
  const Coordinates::Point4D kSource = {0.2f, 0.f, 0.5f, 1.f};
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitLeaderLinesAtElevation(kSource, tentRoofAt,
                                                   /*splitRightEdge=*/false);

  EXPECT_TRUE(kSplit.below.empty());
  EXPECT_EQ(kSplit.above.size(), 2u);
}

// A connector starting exactly ON the surface -- which is where
// ElevationListener puts every clamped source -- must not emit a zero-length
// run at the source before dipping under. The epsilon in the emit guard is what
// this asserts, and its absence would put a degenerate segment into the list.
TEST(HeightIndicatorTest, leaderSplitEmitsNoZeroLengthRunAtTheSource) {
  const Coordinates::Point4D kSource = {0.2f, 0.f, -0.5f, 1.f};
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitLeaderLinesAtElevation(kSource, tentRoofAt,
                                                   /*splitRightEdge=*/false);

  for (const HeightIndicator::Segment& run : kSplit.below) {
    EXPECT_GT(segmentLength(run), 1e-4f);
  }
  for (const HeightIndicator::Segment& run : kSplit.above) {
    EXPECT_GT(segmentLength(run), 1e-4f);
  }
}

// Every connector run keeps the source's height and starts or ends where the
// whole connector did, so splitting never detaches one from the marker or from
// the outline edge it meets.
TEST(HeightIndicatorTest, leaderSplitRunsKeepTheSourceHeightAndSpan) {
  const Coordinates::Point4D kSource = {0.2f, 0.f, -0.5f, 1.f};
  const std::array<HeightIndicator::Segment, 2> kWhole =
      HeightIndicator::leaderLines(kSource);
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitLeaderLinesAtElevation(kSource, tentRoofAt,
                                                   /*splitRightEdge=*/false);

  float total = 0.f;
  for (const HeightIndicator::Segment& run : kSplit.below) {
    EXPECT_NEAR(run.start.a[1], kSource.a[1], kTolerance);
    EXPECT_NEAR(run.end.a[1], kSource.a[1], kTolerance);
    total += segmentLength(run);
  }
  for (const HeightIndicator::Segment& run : kSplit.above) {
    EXPECT_NEAR(run.start.a[1], kSource.a[1], kTolerance);
    EXPECT_NEAR(run.end.a[1], kSource.a[1], kTolerance);
    total += segmentLength(run);
  }

  // The runs cover both connectors exactly -- nothing lost, nothing counted
  // twice, whatever the split did in between.
  EXPECT_NEAR(total, segmentLength(kWhole[0]) + segmentLength(kWhole[1]),
              kTolerance);
}

// The right-edge connector is drawn on top even when the surface is entirely
// ABOVE the source, which is the case a comparison would classify as below.
// This is the assertion that fails if it is ever classified rather than placed.
TEST(HeightIndicatorTest, leaderSplitAlwaysDrawsTheRightEdgeConnectorOnTop) {
  const Coordinates::Point4D kSource = {0.2f, -0.5f, -0.5f, 1.f};
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitLeaderLinesAtElevation(
          kSource, [](const float, const float) { return 0.9f; },
          /*splitRightEdge=*/false);

  EXPECT_TRUE(anyRunReachesTheRightBound(kSplit.above));
  EXPECT_FALSE(anyRunReachesTheRightBound(kSplit.below));

  // The back-edge connector, by contrast, IS under a surface this high.
  EXPECT_EQ(kSplit.below.size(), 1u);
}

// The flicker itself: a quantised source height lands a fraction either side of
// the surface it rests on. Nudging the surface across that tie must not move
// the right-edge connector between the lists.
TEST(HeightIndicatorTest,
     leaderSplitKeepsTheRightEdgeConnectorStableAcrossATie) {
  const Coordinates::Point4D kSource = {0.2f, 0.f, -0.5f, 1.f};

  for (const float kNudge : {-1e-3f, 0.f, 1e-3f}) {
    const HeightIndicator::SplitOutline kSplit =
        HeightIndicator::splitLeaderLinesAtElevation(
            kSource,
            [kNudge](const float leftRight, const float frontBack) {
              return tentRoofAt(leftRight, frontBack) + kNudge;
            },
            /*splitRightEdge=*/false);

    EXPECT_TRUE(anyRunReachesTheRightBound(kSplit.above)) << "nudge " << kNudge;
    EXPECT_FALSE(anyRunReachesTheRightBound(kSplit.below))
        << "nudge " << kNudge;
  }
}

// It reaches the room's right bound at the source's own front/back position and
// keeps the source's height, so placing it rather than splitting it does not
// detach it from the marker or from the outline edge it meets.
TEST(HeightIndicatorTest, rightEdgeConnectorKeepsItsSpanWhenPlacedOnTop) {
  const Coordinates::Point4D kSource = {-0.4f, 0.2f, 0.3f, 1.f};
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitLeaderLinesAtElevation(kSource, tentRoofAt,
                                                   /*splitRightEdge=*/false);

  const HeightIndicator::Segment kWhole =
      HeightIndicator::leaderLines(kSource)[1];
  bool found = false;
  for (const HeightIndicator::Segment& run : kSplit.above) {
    if (std::abs(run.end.a[0] - 1.f) >= kTolerance) {
      continue;
    }
    found = true;
    EXPECT_NEAR(run.start.a[0], kWhole.start.a[0], kTolerance);
    EXPECT_NEAR(run.start.a[1], kSource.a[1], kTolerance);
    EXPECT_NEAR(run.start.a[2], kWhole.start.a[2], kTolerance);
    EXPECT_NEAR(run.end.a[1], kSource.a[1], kTolerance);
    EXPECT_NEAR(run.end.a[2], kSource.a[2], kTolerance);
    EXPECT_NEAR(segmentLength(run), segmentLength(kWhole), kTolerance);
  }
  EXPECT_TRUE(found);
}

// The dome is the one surface that falls away in left/right as well as
// front/back, so it is the one under which the RIGHT-EDGE connector genuinely
// crosses rather than resting coincident with the surface. These pin that
// asymmetry, which is why the split takes a flag rather than inferring it.

// From a source LEFT of the room's centre the right-edge connector runs over
// the dome's crest and dips under it -- the same shape the back-edge connector
// has from a source in front of centre, in the other axis.
TEST(HeightIndicatorTest,
     domeCutsTheRightEdgeConnectorFromASourceLeftOfCentre) {
  // On the dome at left/right -0.5, front/back 0: height 2*sqrt(0.75) - 1.
  const float kHeight = domeRoofAt(-0.5f, 0.f);
  const Coordinates::Point4D kSource = {-0.5f, kHeight, 0.f, 1.f};
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitLeaderLinesAtElevation(kSource, domeRoofAt,
                                                   /*splitRightEdge=*/true);

  // The submerged run reaches the room's right bound side of the source and
  // surfaces where the dome falls back to the source's height, mirrored about
  // the centre at left/right +0.5.
  ASSERT_EQ(kSplit.below.size(), 1u);
  EXPECT_NEAR(kSplit.below[0].start.a[0], -0.5f, 1e-3f);
  EXPECT_NEAR(kSplit.below[0].end.a[0], 0.5f, 1e-3f);
}

// From a source RIGHT of centre the dome only falls away toward the right
// bound, so the right-edge connector stays above it throughout -- the half the
// left-of-centre case cannot show.
TEST(HeightIndicatorTest, domeLeavesTheRightEdgeConnectorUncutRightOfCentre) {
  const float kHeight = domeRoofAt(0.5f, 0.f);
  const Coordinates::Point4D kSource = {0.5f, kHeight, 0.f, 1.f};
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitLeaderLinesAtElevation(kSource, domeRoofAt,
                                                   /*splitRightEdge=*/true);

  EXPECT_TRUE(kSplit.below.empty());
  EXPECT_EQ(kSplit.above.size(), 2u);
}

// Both connectors can be cut at once, which no other pattern can do: a source
// forward of and left of centre runs under the dome in both axes.
TEST(HeightIndicatorTest, domeCanCutBothConnectorsAtOnce) {
  const float kHeight = domeRoofAt(-0.4f, -0.4f);
  const Coordinates::Point4D kSource = {-0.4f, kHeight, -0.4f, 1.f};
  const HeightIndicator::SplitOutline kSplit =
      HeightIndicator::splitLeaderLinesAtElevation(kSource, domeRoofAt,
                                                   /*splitRightEdge=*/true);

  ASSERT_EQ(kSplit.below.size(), 2u);

  // One submerged run per connector, told apart by the axis it travels: the
  // right-edge connector's varies in left/right, the back-edge connector's in
  // front/back. Neither reaches a room bound -- both END at their crossing,
  // which is what distinguishes a cut connector from a whole one.
  bool cutAcrossLeftRight = false, cutAcrossFrontBack = false;
  for (const HeightIndicator::Segment& run : kSplit.below) {
    if (std::abs(run.end.a[0] - run.start.a[0]) > kTolerance) {
      cutAcrossLeftRight = true;
    }
    if (std::abs(run.end.a[2] - run.start.a[2]) > kTolerance) {
      cutAcrossFrontBack = true;
    }
  }
  EXPECT_TRUE(cutAcrossLeftRight);
  EXPECT_TRUE(cutAcrossFrontBack);
  EXPECT_FALSE(anyRunReachesTheRightBound(kSplit.below));
}

// The outline never crosses the dome, whatever the source height. It rests on
// the room's SIDES, and the dome meets the floor at its rim inside them, so
// there is nothing there to pass under -- this falls out of the geometry rather
// than being special-cased, and it is what keeps the dome looking unchanged.
TEST(HeightIndicatorTest, domeNeverCutsTheOutline) {
  for (const float kHeight : {-0.9f, -0.5f, 0.f, 0.5f, 0.9f}) {
    const HeightIndicator::SplitOutline kSplit =
        HeightIndicator::splitAtElevation(kHeight, domeRoofAt);

    EXPECT_TRUE(kSplit.below.empty()) << "height " << kHeight;
    EXPECT_EQ(kSplit.above.size(), 4u) << "height " << kHeight;
  }
}

// The surface is read at the point being tested, not at one coordinate of it.
// Under the dome two positions sharing a front/back sit at different heights,
// so a split that passed only front/back would classify them alike.
TEST(HeightIndicatorTest, domeHeightDependsOnBothCoordinates) {
  EXPECT_GT(domeRoofAt(0.f, 0.f), domeRoofAt(0.6f, 0.f));
  EXPECT_NEAR(domeRoofAt(0.6f, 0.f), domeRoofAt(-0.6f, 0.f), kTolerance);
  EXPECT_NEAR(domeRoofAt(1.f, 0.f), -1.f, kTolerance);
  EXPECT_NEAR(domeRoofAt(0.8f, 0.8f), -1.f, kTolerance);
}

// The ill-conditioning a quantised source height causes near the dome's nearly
// flat crown, and why the view reads the height off the SURFACE instead: a
// small height error there displaces the crossing a long way.

// A source right of the dome's crest admits no crossing: the surface only falls
// away toward the right bound. Taking its height from the surface is what makes
// that fall out, with no sign test anywhere.
TEST(HeightIndicatorTest, domeRightOfCrestHasNoCrossingAtTheExactHeight) {
  for (const float kLeftRight : {0.02f, 0.1f, 0.4f, 0.8f}) {
    const Coordinates::Point4D kSource = {
        kLeftRight, domeRoofAt(kLeftRight, 0.f), 0.f, 1.f};
    const HeightIndicator::SplitOutline kSplit =
        HeightIndicator::splitLeaderLinesAtElevation(kSource, domeRoofAt,
                                                     /*splitRightEdge=*/true);

    EXPECT_TRUE(kSplit.below.empty()) << "left/right " << kLeftRight;
  }
}

// Left of the crest it crosses at exactly the mirrored position, at every
// distance from the centre -- the property a quantised height destroys.
TEST(HeightIndicatorTest, domeLeftOfCrestCrossesAtTheMirroredPosition) {
  for (const float kLeftRight : {-0.04f, -0.2f, -0.6f}) {
    const Coordinates::Point4D kSource = {
        kLeftRight, domeRoofAt(kLeftRight, 0.f), 0.f, 1.f};
    const HeightIndicator::SplitOutline kSplit =
        HeightIndicator::splitLeaderLinesAtElevation(kSource, domeRoofAt,
                                                     /*splitRightEdge=*/true);

    ASSERT_EQ(kSplit.below.size(), 1u) << "left/right " << kLeftRight;
    EXPECT_NEAR(kSplit.below[0].end.a[0], -kLeftRight, 1e-3f);
  }
}

// One parameter step of height error near the crown puts the line under the
// dome for a long stretch, on the side that admits no crossing; reading the
// height off the surface removes the run entirely.
TEST(HeightIndicatorTest, domeQuantisedHeightManufacturesASpuriousCrossing) {
  // Parameter x = +1 of kPositionExtent = 50.
  const float kLeftRight = 1.f / 50.f;
  const float kExactHeight = domeRoofAt(kLeftRight, 0.f);
  // The same height after a round trip through one integer parameter step.
  const float kQuantisedHeight = std::floor(kExactHeight * 50.f) / 50.f;
  ASSERT_LT(kQuantisedHeight, kExactHeight);

  const HeightIndicator::SplitOutline kQuantised =
      HeightIndicator::splitLeaderLinesAtElevation(
          {kLeftRight, kQuantisedHeight, 0.f, 1.f}, domeRoofAt,
          /*splitRightEdge=*/true);
  const HeightIndicator::SplitOutline kExact =
      HeightIndicator::splitLeaderLinesAtElevation(
          {kLeftRight, kExactHeight, 0.f, 1.f}, domeRoofAt,
          /*splitRightEdge=*/true);

  // The quantised height sits BELOW the surface at the source, so both
  // connectors start submerged -- the error is not confined to the one that
  // travels the flattest part of the crown.
  EXPECT_EQ(kQuantised.below.size(), 2u);

  // The right-edge run is the one that travels in left/right. It resurfaces
  // where the dome falls back to the quantised height, far across the room from
  // a source barely off centre.
  bool sawTheRightEdgeRun = false;
  for (const HeightIndicator::Segment& run : kQuantised.below) {
    if (std::abs(run.end.a[0] - run.start.a[0]) <= kTolerance) {
      continue;
    }
    sawTheRightEdgeRun = true;
    EXPECT_GT(run.end.a[0], 0.1f);
  }
  EXPECT_TRUE(sawTheRightEdgeRun);

  // Reading the height off the surface removes every submerged run: this source
  // is right of the crest, where the geometry admits no crossing at all.
  EXPECT_TRUE(kExact.below.empty());
}
