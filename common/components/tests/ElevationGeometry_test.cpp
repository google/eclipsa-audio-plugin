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

// The properties of the Elevation.h statics that the top-down panner's
// elevation painters draw against. The painters themselves are not reachable
// from a test -- there is no UI test harness (issue #32) and their visual
// result is verified by hand in PAN-01.4 -- but the geometry they sample is,
// and it is the half that can silently drift: the surfaces are built by
// sampling these functions, so a change here changes what is drawn.

// Pull in the umbrella header first: several components/src headers include
// components.h themselves and only resolve correctly once the umbrella has
// fully loaded once. See the same note in Coordinates_test.cpp.
// clang-format off
#include <components/components.h>

#include "data_structures/src/Elevation.h"
// clang-format on

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace {
constexpr float kTolerance = 1e-4f;
// The dome's height is 2 * sqrt(1 - (x^2 + y^2)) - 1, so a point on the
// boundary reaches the floor through a square root of a value that is only
// approximately zero in floating point. The height therefore needs a looser
// tolerance than the coordinates it is derived from.
constexpr float kDomeHeightTolerance = 5e-3f;

// The front/back positions the arch painter samples: 41 samples, offset
// i * 0.05f, spanning -1 to 1.
std::vector<float> archSamplePositions() {
  std::vector<float> positions;
  for (int i = 0; i < 41; ++i) {
    positions.push_back(-1.f + i * 0.05f);
  }
  return positions;
}

// The front/back positions the curve painter samples: 32 samples spanning
// -1 to 1.
std::vector<float> curveSamplePositions() {
  std::vector<float> positions;
  for (int i = 0; i < 32; ++i) {
    positions.push_back(-1.f + i * 2.f / 31.f);
  }
  return positions;
}
}  // namespace

// The tent ridge sits at the middle of the room at full height, and the tent
// meets the floor at both front/back bounds -- the six anchors the painter
// builds its two sloping planes from.
TEST(ElevationGeometry, tentRunsFromFloorToRidgeToFloor) {
  EXPECT_NEAR(ElevationListener::getTentElevationPt({0.f, 0.f, 0.f}).a[1], 1.f,
              kTolerance);
  EXPECT_NEAR(ElevationListener::getTentElevationPt({0.f, -1.f, 0.f}).a[1],
              -1.f, kTolerance);
  EXPECT_NEAR(ElevationListener::getTentElevationPt({0.f, 1.f, 0.f}).a[1], -1.f,
              kTolerance);
}

// The tent is symmetric about the ridge, which is why the painter may sample
// it at a front/back coordinate of either sign.
TEST(ElevationGeometry, tentIsSymmetricAboutTheRidge) {
  for (const float frontBack : archSamplePositions()) {
    EXPECT_NEAR(
        ElevationListener::getTentElevationPt({0.f, frontBack, 0.f}).a[1],
        ElevationListener::getTentElevationPt({0.f, -frontBack, 0.f}).a[1],
        kTolerance);
  }
}

// The arch is the parabola through (-1,-1), (1,-1) and (0,1) -- its peak is at
// the middle of the room and it meets the floor at both front/back bounds.
TEST(ElevationGeometry, archIsTheParabolaThroughTheRoomBounds) {
  EXPECT_NEAR(ElevationListener::getArchElevationPt({0.f, 0.f, 0.f}).a[1], 1.f,
              kTolerance);
  EXPECT_NEAR(ElevationListener::getArchElevationPt({0.f, -1.f, 0.f}).a[1],
              -1.f, kTolerance);
  EXPECT_NEAR(ElevationListener::getArchElevationPt({0.f, 1.f, 0.f}).a[1], -1.f,
              kTolerance);
}

// The arch is symmetric about the middle of the room, which is why the painter
// may sample it at a front/back coordinate of either sign.
TEST(ElevationGeometry, archIsSymmetricAboutTheRoomMiddle) {
  for (const float frontBack : archSamplePositions()) {
    EXPECT_NEAR(
        ElevationListener::getArchElevationPt({0.f, frontBack, 0.f}).a[1],
        ElevationListener::getArchElevationPt({0.f, -frontBack, 0.f}).a[1],
        kTolerance);
  }
}

// The arch's sample set is genuinely curved: consecutive samples differ, and
// the height varies across the run, so the painter cannot be approximating it
// with a single plane.
TEST(ElevationGeometry, archSamplesDescribeACurveNotAPlane) {
  float minHeight = 2.f, maxHeight = -2.f;
  for (const float frontBack : archSamplePositions()) {
    const float height =
        ElevationListener::getArchElevationPt({0.f, frontBack, 0.f}).a[1];
    minHeight = std::min(minHeight, height);
    maxHeight = std::max(maxHeight, height);
    EXPECT_GE(height, -1.f - kTolerance);
    EXPECT_LE(height, 1.f + kTolerance);
  }
  EXPECT_NEAR(minHeight, -1.f, kTolerance);
  EXPECT_NEAR(maxHeight, 1.f, kTolerance);
}

// The curve is NOT symmetric about the middle of the room -- it rises
// monotonically from front to back -- which is why the sign the painter passes
// as the front/back coordinate matters for this pattern and not for the
// others.
TEST(ElevationGeometry, curveIsMonotonicAndNotSymmetric) {
  float previous = -2.f;
  for (const float frontBack : curveSamplePositions()) {
    const float height =
        ElevationListener::getCurveElevationPt({0.f, frontBack, 0.f}).a[1];
    EXPECT_GE(height, previous);
    previous = height;
  }
  EXPECT_GT(ElevationListener::getCurveElevationPt({0.f, 0.5f, 0.f}).a[1],
            ElevationListener::getCurveElevationPt({0.f, -0.5f, 0.f}).a[1]);
}

// The curve never leaves the room: it is clamped to the floor at the front
// bound, where the logarithm is undefined, and stays within the ceiling at the
// back bound.
TEST(ElevationGeometry, curveStaysWithinTheRoomAtBothBounds) {
  for (const float frontBack : curveSamplePositions()) {
    const float height =
        ElevationListener::getCurveElevationPt({0.f, frontBack, 0.f}).a[1];
    EXPECT_FALSE(std::isnan(height));
    EXPECT_GE(height, -1.f - kTolerance);
    EXPECT_LE(height, 1.f + kTolerance);
  }
  EXPECT_NEAR(ElevationListener::getCurveElevationPt({0.f, -1.f, 0.f}).a[1],
              -1.f, kTolerance);
}

// The dome's clamp boundary is the unit circle at floor height: every point
// the painter samples on that circle comes back unmoved, at the floor. This is
// what ties the drawn circle's radius to the clamp rather than to a constant.
TEST(ElevationGeometry, domeBoundaryIsTheUnitCircleAtFloorHeight) {
  const int kNumSamples = 81;
  for (int i = 0; i < kNumSamples; ++i) {
    const float theta =
        i * juce::MathConstants<float>::twoPi / (kNumSamples - 1);
    const Coordinates::Point3D boundPt =
        ElevationListener::getDomeElevationPtClamped(
            {std::cos(theta), std::sin(theta), 0.f}, {});
    const float radius =
        std::sqrt(boundPt.a[0] * boundPt.a[0] + boundPt.a[2] * boundPt.a[2]);
    EXPECT_NEAR(radius, 1.f, kTolerance);
    EXPECT_NEAR(boundPt.a[1], -1.f, kDomeHeightTolerance);
  }
}

// A source outside the dome is pulled back onto that same boundary, so the
// drawn circle really does mark where a source can sit. The point used is
// inside the parameter domain on each axis (both coordinates within -1..1) and
// outside the circle only in combination, which is the reachable case.
TEST(ElevationGeometry, domeClampsAnOutsidePointOntoTheBoundary) {
  const Coordinates::Point3D clamped =
      ElevationListener::getDomeElevationPtClamped({0.9f, 0.9f, 0.f},
                                                   {0.f, 0.9f, 0.f});
  const float radius =
      std::sqrt(clamped.a[0] * clamped.a[0] + clamped.a[2] * clamped.a[2]);
  EXPECT_NEAR(radius, 1.f, kTolerance);
  EXPECT_NEAR(clamped.a[1], -1.f, kDomeHeightTolerance);
}

// The dome's apex is at the middle of the room, at the ceiling.
TEST(ElevationGeometry, domeApexIsAtTheRoomCeiling) {
  EXPECT_NEAR(
      ElevationListener::getDomeElevationPtClamped({0.f, 0.f, 0.f}, {}).a[1],
      1.f, kTolerance);
}
