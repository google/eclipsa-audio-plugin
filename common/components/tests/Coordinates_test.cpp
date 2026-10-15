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

// Pull in the umbrella header first: several components/src headers include
// components.h themselves and only resolve correctly once the umbrella has
// fully loaded once. See the same note in ExportErrorBanner_test.cpp.
// clang-format off
#include <components/components.h>

#include "components/src/room_views/Coordinates.h"
// clang-format on

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
constexpr float kTolerance = 1e-5f;
}  // namespace

// The origin of parameter space is the origin of NDC, on every axis.
TEST(test_room_coordinates, originMapsToOrigin) {
  const Coordinates::Point4D ndc = Coordinates::toRoomNdc(0.f, 0.f, 0.f);
  EXPECT_NEAR(ndc.a[0], 0.f, kTolerance);
  EXPECT_NEAR(ndc.a[1], 0.f, kTolerance);
  EXPECT_NEAR(ndc.a[2], 0.f, kTolerance);
  EXPECT_NEAR(ndc.a[3], 1.f, kTolerance);
}

// Parameter x (left/right) maps to NDC x, scaled by 1/50 and not negated.
TEST(test_room_coordinates, leftRightMapsToNdcX) {
  EXPECT_NEAR(Coordinates::toRoomNdc(50.f, 0.f, 0.f).a[0], 1.f, kTolerance);
  EXPECT_NEAR(Coordinates::toRoomNdc(-50.f, 0.f, 0.f).a[0], -1.f, kTolerance);
  EXPECT_NEAR(Coordinates::toRoomNdc(25.f, 0.f, 0.f).a[0], 0.5f, kTolerance);
}

// Parameter z (height) maps to NDC y (up) -- the axis swap, not just a scale.
TEST(test_room_coordinates, heightMapsToNdcY) {
  EXPECT_NEAR(Coordinates::toRoomNdc(0.f, 0.f, 50.f).a[1], 1.f, kTolerance);
  EXPECT_NEAR(Coordinates::toRoomNdc(0.f, 0.f, -50.f).a[1], -1.f, kTolerance);
  EXPECT_NEAR(Coordinates::toRoomNdc(0.f, 0.f, 30.f).a[1], 0.6f, kTolerance);
}

// Parameter y (front/back) maps to NDC z with its sign inverted.
TEST(test_room_coordinates, frontBackMapsToNegatedNdcZ) {
  EXPECT_NEAR(Coordinates::toRoomNdc(0.f, 50.f, 0.f).a[2], -1.f, kTolerance);
  EXPECT_NEAR(Coordinates::toRoomNdc(0.f, -50.f, 0.f).a[2], 1.f, kTolerance);
}

// Each parameter axis moves exactly one NDC axis and leaves the others alone.
TEST(test_room_coordinates, axesAreIndependent) {
  const Coordinates::Point4D onlyX = Coordinates::toRoomNdc(50.f, 0.f, 0.f);
  EXPECT_NEAR(onlyX.a[1], 0.f, kTolerance);
  EXPECT_NEAR(onlyX.a[2], 0.f, kTolerance);

  const Coordinates::Point4D onlyY = Coordinates::toRoomNdc(0.f, 50.f, 0.f);
  EXPECT_NEAR(onlyY.a[0], 0.f, kTolerance);
  EXPECT_NEAR(onlyY.a[1], 0.f, kTolerance);

  const Coordinates::Point4D onlyZ = Coordinates::toRoomNdc(0.f, 0.f, 50.f);
  EXPECT_NEAR(onlyZ.a[0], 0.f, kTolerance);
  EXPECT_NEAR(onlyZ.a[2], 0.f, kTolerance);
}

// The inverse recovers the parameter triple, including the axis swap.
TEST(test_room_coordinates, inverseRecoversEachAxis) {
  EXPECT_EQ(Coordinates::fromRoomNdc({1.f, 0.f, 0.f, 1.f}).x, 50);
  EXPECT_EQ(Coordinates::fromRoomNdc({0.f, 0.f, -1.f, 1.f}).y, 50);
  EXPECT_EQ(Coordinates::fromRoomNdc({0.f, 1.f, 0.f, 1.f}).z, 50);
}

// Round-tripping every integer position on every axis returns the original.
TEST(test_room_coordinates, roundTripIsExactOverTheFullRange) {
  for (int v = -50; v <= 50; ++v) {
    const Coordinates::PositionParameters onX =
        Coordinates::fromRoomNdc(Coordinates::toRoomNdc((float)v, 0.f, 0.f));
    EXPECT_EQ(onX.x, v);
    EXPECT_EQ(onX.y, 0);
    EXPECT_EQ(onX.z, 0);

    const Coordinates::PositionParameters onY =
        Coordinates::fromRoomNdc(Coordinates::toRoomNdc(0.f, (float)v, 0.f));
    EXPECT_EQ(onY.x, 0);
    EXPECT_EQ(onY.y, v);
    EXPECT_EQ(onY.z, 0);

    const Coordinates::PositionParameters onZ =
        Coordinates::fromRoomNdc(Coordinates::toRoomNdc(0.f, 0.f, (float)v));
    EXPECT_EQ(onZ.x, 0);
    EXPECT_EQ(onZ.y, 0);
    EXPECT_EQ(onZ.z, v);
  }
}

// A round trip of all three axes at once is exact too -- the corners of the
// parameter cube are where an axis swap error would show up first.
TEST(test_room_coordinates, roundTripIsExactAtTheCorners) {
  for (int x : {-50, 0, 50}) {
    for (int y : {-50, 0, 50}) {
      for (int z : {-50, 0, 50}) {
        const Coordinates::PositionParameters back = Coordinates::fromRoomNdc(
            Coordinates::toRoomNdc((float)x, (float)y, (float)z));
        EXPECT_EQ(back.x, x);
        EXPECT_EQ(back.y, y);
        EXPECT_EQ(back.z, z);
      }
    }
  }
}

namespace {
// An odd-sized window, so a test cannot pass on symmetry alone.
const Coordinates::WindowData kTestWindow = {.leftCornerX = 0.f,
                                             .bottomCornerY = 431.f,
                                             .width = 613.f,
                                             .height = 431.f};

// Runs the whole drag conversion: parameters to window, then back at the same
// height.
Coordinates::PositionParameters projectAndInvert(const int x, const int y,
                                                 const int z) {
  const Coordinates::Point4D kNdc =
      Coordinates::toRoomNdc((float)x, (float)y, (float)z);
  const Coordinates::Point2D kWindow = Coordinates::toWindow(
      Coordinates::getTopViewTransform(), kTestWindow, kNdc);
  return Coordinates::fromRoomNdc(
      Coordinates::fromTopViewWindow(Coordinates::getTopViewTransform(),
                                     kTestWindow, kWindow, kNdc.a[1])
          .value());
}
}  // namespace

// The room's centre projects to the window's centre and inverts back to it.
TEST(test_room_coordinates, topViewCentreRoundTripsThroughTheWindow) {
  const Coordinates::PositionParameters back = projectAndInvert(0, 0, 0);
  EXPECT_EQ(back.x, 0);
  EXPECT_EQ(back.y, 0);
  EXPECT_EQ(back.z, 0);
}

// A round trip through the window returns the original position at every
// height.
TEST(test_room_coordinates, topViewWindowRoundTripsAcrossHeightsAndPositions) {
  for (int z : {-50, -30, -7, 0, 12, 30, 50}) {
    for (int x : {-50, -25, -1, 0, 1, 25, 50}) {
      for (int y : {-50, -25, -1, 0, 1, 25, 50}) {
        const Coordinates::PositionParameters back = projectAndInvert(x, y, z);
        EXPECT_EQ(back.x, x) << "at (" << x << ", " << y << ", " << z << ")";
        EXPECT_EQ(back.y, y) << "at (" << x << ", " << y << ", " << z << ")";
        EXPECT_EQ(back.z, z) << "at (" << x << ", " << y << ", " << z << ")";
      }
    }
  }
}

// One window point resolves to a different room position at each height.
TEST(test_room_coordinates, topViewWindowInverseIsHeightDependent) {
  // Off-centre in both axes so both room axes scale.
  const Coordinates::Point2D kOffCentre = {480.f, 120.f};
  const auto kAtHeight = [&kOffCentre](const int z) {
    const float kNdcUp = Coordinates::toRoomNdc(0.f, 0.f, (float)z).a[1];
    return Coordinates::fromRoomNdc(
        Coordinates::fromTopViewWindow(Coordinates::getTopViewTransform(),
                                       kTestWindow, kOffCentre, kNdcUp)
            .value());
  };

  const Coordinates::PositionParameters kLow = kAtHeight(-50);
  const Coordinates::PositionParameters kHigh = kAtHeight(50);
  EXPECT_NE(kLow.x, kHigh.x);
  EXPECT_NE(kLow.y, kHigh.y);
  EXPECT_EQ(kLow.z, -50);
  EXPECT_EQ(kHigh.z, 50);
  // A higher source projects further out, so a fixed window point resolves
  // nearer the centre.
  EXPECT_LT(std::abs(kHigh.x), std::abs(kLow.x));
}

// The inverse returns the height it was given.
TEST(test_room_coordinates, topViewWindowInversePreservesTheGivenHeight) {
  for (int z : {-50, -13, 0, 13, 50}) {
    const float kNdcUp = Coordinates::toRoomNdc(0.f, 0.f, (float)z).a[1];
    const Coordinates::Point4D kRoomNdc =
        Coordinates::fromTopViewWindow(Coordinates::getTopViewTransform(),
                                       kTestWindow, {300.f, 200.f}, kNdcUp)
            .value();
    EXPECT_NEAR(kRoomNdc.a[1], kNdcUp, kTolerance);
    EXPECT_NEAR(kRoomNdc.a[3], 1.f, kTolerance);
    EXPECT_EQ(Coordinates::fromRoomNdc(kRoomNdc).z, z);
  }
}

// A window with no width or no height names no room position.
TEST(test_room_coordinates, topViewWindowInverseRejectsAnEmptyWindow) {
  const Coordinates::Point2D kPointer = {0.f, 0.f};
  for (const Coordinates::WindowData& kEmpty :
       {Coordinates::WindowData{0.f, 400.f, 0.f, 400.f},
        Coordinates::WindowData{0.f, 0.f, 600.f, 0.f},
        Coordinates::WindowData{0.f, 0.f, 0.f, 0.f}}) {
    EXPECT_FALSE(Coordinates::fromTopViewWindow(
                     Coordinates::getTopViewTransform(), kEmpty, kPointer, 0.f)
                     .has_value())
        << "at " << kEmpty.width << " x " << kEmpty.height;
  }
}

namespace {
// The dome's surface, as ElevationListener derives it.
float domeHeightAt(const float planRadius) {
  return 2.f * std::sqrt(std::max(0.f, 1.f - planRadius * planRadius)) - 1.f;
}

// Window x of a point at a plan radius along the left/right axis, resting on
// the dome.
float domeWindowX(const float planRadius) {
  return Coordinates::toWindow(Coordinates::getTopViewTransform(), kTestWindow,
                               {planRadius, domeHeightAt(planRadius), 0.f, 1.f})
      .a[0];
}

// Window x of that same source AS DRAWN now: still resting on the dome, but
// put through the plan plane first. The height must not survive this.
float drawnMarkerX(const float planRadius) {
  return Coordinates::toWindow(
             Coordinates::getTopViewTransform(), kTestWindow,
             Coordinates::toPlanPlane(
                 {planRadius, domeHeightAt(planRadius), 0.f, 1.f}))
      .a[0];
}

// The footprint the dome painter fills: plan radius 1 on the plan plane.
// Every drawn marker has to stay inside it.
float footprintX() {
  return Coordinates::toWindow(Coordinates::getTopViewTransform(), kTestWindow,
                               {1.f, Coordinates::kPlanPlaneUp, 0.f, 1.f})
      .a[0];
}

// The room's centre, which projects to one window point at every height.
float centreX() {
  return Coordinates::toWindow(Coordinates::getTopViewTransform(), kTestWindow,
                               {0.f, Coordinates::kPlanPlaneUp, 0.f, 1.f})
      .a[0];
}

float centreY() {
  return Coordinates::toWindow(Coordinates::getTopViewTransform(), kTestWindow,
                               {0.f, Coordinates::kPlanPlaneUp, 0.f, 1.f})
      .a[1];
}
}  // namespace

// The plan plane keeps where a source is in the room and discards how high it
// is, which is the whole of what it does.
TEST(test_room_coordinates, planPlaneKeepsThePlanPositionAndDropsTheHeight) {
  const Coordinates::Point4D kOnPlane =
      Coordinates::toPlanPlane(Coordinates::toRoomNdc(-30.f, -35.f, -11.f));

  EXPECT_NEAR(kOnPlane.a[0], -0.6f, kTolerance);
  EXPECT_NEAR(kOnPlane.a[2], 0.7f, kTolerance);
  EXPECT_NEAR(kOnPlane.a[1], Coordinates::kPlanPlaneUp, kTolerance);
  EXPECT_NEAR(kOnPlane.a[3], 1.f, kTolerance);
}

// Why the dome is positioned in plan at all. A source resting on it projects
// out by r / (5 - height(r)), which peaks at r = 2*sqrt(2)/3 and comes back,
// so two plan radii share one screen position exactly: 0.8 and the rim. A
// pointer there names both, which is what no inverse can resolve.
TEST(test_room_coordinates, domeProjectionFoldsTwoPlanRadiiOntoOneWindowPoint) {
  EXPECT_NEAR(domeWindowX(0.8f), domeWindowX(1.f), kTolerance);
}

// On the plan plane that collision is gone: the two radii are as far apart on
// screen as they are in the room, and every pair is.
TEST(test_room_coordinates, planPlaneSeparatesThePlanRadiiTheDomeFolds) {
  EXPECT_GT(drawnMarkerX(1.f), drawnMarkerX(0.8f) + 1.f);

  // And no pair collides anywhere: the drawn position rises with plan radius
  // over the whole range, which the surviving height took away.
  float previous = drawnMarkerX(0.f);
  for (int i = 1; i <= 100; ++i) {
    const float kCurrent = drawnMarkerX(i / 100.f);
    EXPECT_GT(kCurrent, previous) << "at plan radius " << i / 100.f;
    previous = kCurrent;
  }
}

// The drag's conversion on the plan plane returns the position it was given,
// for every legal position and whatever height the dome then gives it. This is
// what makes one pointer position name one place: the inverse no longer reads
// a height the previous event produced.
TEST(test_room_coordinates, planPlaneDragConversionRoundTripsAtEveryHeight) {
  for (int x : {-50, -30, -1, 0, 1, 25, 50}) {
    for (int y : {-50, -35, -1, 0, 1, 25, 50}) {
      const Coordinates::Point4D kDrawn = Coordinates::toPlanPlane(
          Coordinates::toRoomNdc((float)x, (float)y,
                                 /*any height*/ -11.f));
      const Coordinates::Point2D kWindow = Coordinates::toWindow(
          Coordinates::getTopViewTransform(), kTestWindow, kDrawn);
      const Coordinates::PositionParameters kBack = Coordinates::fromRoomNdc(
          Coordinates::fromTopViewWindow(Coordinates::getTopViewTransform(),
                                         kTestWindow, kWindow,
                                         Coordinates::kPlanPlaneUp)
              .value());

      EXPECT_EQ(kBack.x, x) << "at (" << x << ", " << y << ")";
      EXPECT_EQ(kBack.y, y) << "at (" << x << ", " << y << ")";
    }
  }
}

// The loop the panner ran under the dome before it positioned in plan: every
// drag event un-projected the pointer at the height the LAST event left the
// source at, and the dome then moved that height. The map has gain above 1
// past the fold, so for one fixed pointer the outcome bifurcates -- below a
// threshold radius the source settles mid-dome, a few thousandths above it the
// source runs to the rim instead. Same pointer, two answers, chosen by where
// the source happened to be.
TEST(test_room_coordinates, aHeightTrackingDragBifurcatesUnderTheDome) {
  // A pointer on the left/right axis, inside the folded band.
  const Coordinates::Point2D kPointer = {domeWindowX(0.9f), centreY()};
  const auto kSettleFrom = [&kPointer](const float startRadius) {
    float radius = startRadius;
    for (int i = 0; i < 400; ++i) {
      const Coordinates::Point4D kNdc =
          Coordinates::fromTopViewWindow(Coordinates::getTopViewTransform(),
                                         kTestWindow, kPointer,
                                         domeHeightAt(radius))
              .value();
      radius = std::min(1.f, std::abs(kNdc.a[0]));
    }
    return radius;
  };

  EXPECT_NEAR(kSettleFrom(0.80f), 0.9f, 1e-3f);
  EXPECT_NEAR(kSettleFrom(0.973f), 0.9f, 1e-3f);
  // Just the other side of the threshold, the same pointer runs to the rim.
  EXPECT_NEAR(kSettleFrom(0.977f), 1.f, 1e-3f);
}

// On the plan plane there is no loop to run: the pointer un-projects to one
// position, and the source's height cannot reach the conversion at all.
TEST(test_room_coordinates, thePlanPlaneDragResolvesOnePositionFromAnyStart) {
  const Coordinates::Point2D kPointer = {domeWindowX(0.9f), centreY()};
  const auto kResolveFrom = [&kPointer](const float startRadius) {
    float radius = startRadius;
    for (int i = 0; i < 8; ++i) {
      const Coordinates::Point4D kNdc =
          Coordinates::fromTopViewWindow(Coordinates::getTopViewTransform(),
                                         kTestWindow, kPointer,
                                         Coordinates::kPlanPlaneUp)
              .value();
      radius = std::min(1.f, std::abs(kNdc.a[0]));
    }
    return radius;
  };

  EXPECT_NEAR(kResolveFrom(0.80f), kResolveFrom(0.977f), kTolerance);
  EXPECT_NEAR(kResolveFrom(0.10f), kResolveFrom(0.999f), kTolerance);
}

// The point manual testing reported, and every legal dome position: drawn on
// the plan plane, none reaches the footprint the painter fills at plan radius
// 1 on that same plane. The marker can no longer leave the drawn circle.
TEST(test_room_coordinates, everyLegalDomePositionDrawsInsideTheFootprint) {
  const float kFootprint = footprintX() - centreX();

  // The reported position: plan radius 46.1 of 50, at the height the dome
  // gives it. It used to draw 6 percent beyond the footprint.
  const Coordinates::Point2D kReported = Coordinates::toWindow(
      Coordinates::getTopViewTransform(), kTestWindow,
      Coordinates::toPlanPlane(Coordinates::toRoomNdc(-30.f, -35.f, -11.f)));
  EXPECT_LT(std::abs(kReported.a[0] - centreX()), kFootprint);

  for (int i = 0; i <= 100; ++i) {
    const float kPlanRadius = i / 100.f;
    EXPECT_LE(drawnMarkerX(kPlanRadius) - centreX(), kFootprint + kTolerance)
        << "at plan radius " << kPlanRadius;
  }
}

// Under the dome the height indicator is anchored where the marker is drawn,
// at the source's height: it projects onto the marker, so the leader lines
// start there, and it lies inside the room so they end on the outline.
TEST(test_room_coordinates, domeIndicatorAnchorSitsOnTheMarkerInsideTheRoom) {
  for (int x = -50; x <= 50; x += 5) {
    for (int y = -50; y <= 50; y += 5) {
      const Coordinates::Point4D kPlan =
          Coordinates::toRoomNdc((float)x, (float)y, 0.f);
      const float kPlanRadius = std::hypot(kPlan.a[0], kPlan.a[2]);
      if (kPlanRadius > 1.f) {
        continue;
      }
      const float kHeight = domeHeightAt(kPlanRadius);
      const Coordinates::Point2D kMarker =
          Coordinates::toWindow(Coordinates::getTopViewTransform(), kTestWindow,
                                Coordinates::toPlanPlane(kPlan));

      const Coordinates::Point4D kAnchor =
          Coordinates::fromTopViewWindow(Coordinates::getTopViewTransform(),
                                         kTestWindow, kMarker, kHeight)
              .value();
      const Coordinates::Point2D kAnchorOnScreen = Coordinates::toWindow(
          Coordinates::getTopViewTransform(), kTestWindow, kAnchor);

      EXPECT_NEAR(kAnchorOnScreen.a[0], kMarker.a[0], 1e-3f)
          << "at (" << x << ", " << y << ")";
      EXPECT_NEAR(kAnchorOnScreen.a[1], kMarker.a[1], 1e-3f)
          << "at (" << x << ", " << y << ")";
      EXPECT_LE(std::abs(kAnchor.a[0]), 1.f + kTolerance)
          << "at (" << x << ", " << y << ")";
      EXPECT_LE(std::abs(kAnchor.a[2]), 1.f + kTolerance)
          << "at (" << x << ", " << y << ")";
    }
  }
}
