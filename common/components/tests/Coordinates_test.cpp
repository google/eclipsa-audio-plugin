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
