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
// fully loaded once. See the same note in Coordinates_test.cpp.
// clang-format off
#include <components/components.h>

#include "components/src/room_views/FaceLookup.h"
// clang-format on

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

using FaceLookup::Face;
using FaceLookup::FaceTag;

// Every screen corner lies on the front wall's plane.
TEST(PannerScreenFace, CornersLieOnTheFrontWall) {
  for (const Coordinates::Point4D& corner :
       FaceLookup::kFrontScreenFace.cornerVertices) {
    EXPECT_FLOAT_EQ(corner.a[FaceLookup::kAxisZ], -1.f);
  }
}

// Every screen corner sits three quarters of the way out in width and height.
TEST(PannerScreenFace, CornersSpanThreeQuartersOfTheWall) {
  for (const Coordinates::Point4D& corner :
       FaceLookup::kFrontScreenFace.cornerVertices) {
    EXPECT_FLOAT_EQ(std::abs(corner.a[FaceLookup::kAxisX]), 0.75f);
    EXPECT_FLOAT_EQ(std::abs(corner.a[FaceLookup::kAxisY]), 0.75f);
  }
}

// The panner's face set is the top view's with the screen drawn last.
TEST(PannerScreenFace, PannerTopEndsWithTheScreen) {
  const std::vector<Face> kPannerFaces =
      FaceLookup::getFaces(FaceLookup::kPannerTop);
  const std::vector<Face> kTopFaces = FaceLookup::getFaces(FaceLookup::kTop);
  ASSERT_EQ(kPannerFaces.size(), kTopFaces.size() + 1);
  for (size_t i = 0; i < kTopFaces.size(); ++i) {
    EXPECT_EQ(kPannerFaces[i].tag, kTopFaces[i].tag);
  }
  EXPECT_EQ(kPannerFaces.back().tag, FaceTag::kScreen);
}

// The renderer's top view keeps its faces and never shows the screen.
TEST(PannerScreenFace, TopHasNoScreen) {
  const std::vector<Face> kTopFaces = FaceLookup::getFaces(FaceLookup::kTop);
  EXPECT_TRUE(std::none_of(
      kTopFaces.begin(), kTopFaces.end(),
      [](const Face& face) { return face.tag == FaceTag::kScreen; }));
}

}  // namespace
