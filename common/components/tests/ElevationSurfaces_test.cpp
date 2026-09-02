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

// The path assembly behind the top-down panner's elevation surfaces. This is
// the half of the drawing code that can be wrong silently: an off-by-one in
// the reverse index walk of sampledEdgesToPath truncates or self-intersects
// the surface, and nothing about the plugin would report it. The tests build
// paths against an identity transform, where toWindow reduces to a plain
// NDC-to-window map, so every expected coordinate is exact.

// Pull in the umbrella header first: several components/src headers include
// components.h themselves and only resolve correctly once the umbrella has
// fully loaded once. See the same note in Coordinates_test.cpp.
// clang-format off
#include <components/components.h>

#include "components/src/room_views/ElevationSurfaces.h"
// clang-format on

#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

namespace {
constexpr float kTolerance = 1e-4f;

// A 200x100 window, matching the shape paint() builds from the component
// bounds.
const Coordinates::WindowData kWindow = {.leftCornerX = 0.f,
                                         .bottomCornerY = 100.f,
                                         .width = 200.f,
                                         .height = 100.f};

// The identity transform leaves the perspective divide at w = 1, so toWindow
// maps NDC x to 100 * (x + 1) and NDC y (up) to 50 * (1 - y). NDC z does not
// reach the screen under it, which is what makes the expectations below exact.
Coordinates::Mat4 identityTransform() {
  return boost::qvm::identity_mat<float, 4>();
}

// Count the path's elements by kind, so a test can assert the shape of the
// path and not merely its extent.
struct PathElementCounts {
  int subPathStarts = 0;
  int lines = 0;
  int quadratics = 0;
  int closes = 0;
};

PathElementCounts countElements(const juce::Path& path) {
  PathElementCounts counts;
  juce::Path::Iterator it(path);
  while (it.next()) {
    switch (it.elementType) {
      case juce::Path::Iterator::startNewSubPath:
        ++counts.subPathStarts;
        break;
      case juce::Path::Iterator::lineTo:
        ++counts.lines;
        break;
      case juce::Path::Iterator::quadraticTo:
        ++counts.quadratics;
        break;
      case juce::Path::Iterator::closePath:
        ++counts.closes;
        break;
      default:
        break;
    }
  }
  return counts;
}

// The two edges of a room-spanning surface, sampled at `count` heights running
// from the floor to the ceiling. Front/back is left at 0 because it does not
// reach the screen under the identity transform.
void makeEdges(int count, std::vector<Coordinates::Point4D>& leftEdge,
               std::vector<Coordinates::Point4D>& rightEdge) {
  leftEdge.clear();
  rightEdge.clear();
  for (int i = 0; i < count; ++i) {
    const float height = -1.f + i * 2.f / (count - 1);
    leftEdge.push_back({-1.f, height, 0.f, 1.f});
    rightEdge.push_back({1.f, height, 0.f, 1.f});
  }
}
}  // namespace

// A four-anchor surface becomes one closed quad whose corners land where the
// projection puts them -- the Flat pattern's whole shape.
TEST(ElevationSurfaces, anchorsToPathClosesAQuadAtTheProjectedCorners) {
  const std::vector<Coordinates::Point4D> kAnchors = {{-1.f, -1.f, 0.f, 1.f},
                                                      {1.f, -1.f, 0.f, 1.f},
                                                      {1.f, 1.f, 0.f, 1.f},
                                                      {-1.f, 1.f, 0.f, 1.f}};

  const juce::Path path =
      ElevationSurfaces::anchorsToPath(identityTransform(), kWindow, kAnchors);

  EXPECT_FALSE(path.isEmpty());
  const juce::Rectangle<float> bounds = path.getBounds();
  EXPECT_NEAR(bounds.getX(), 0.f, kTolerance);
  EXPECT_NEAR(bounds.getY(), 0.f, kTolerance);
  EXPECT_NEAR(bounds.getWidth(), 200.f, kTolerance);
  EXPECT_NEAR(bounds.getHeight(), 100.f, kTolerance);

  const PathElementCounts counts = countElements(path);
  EXPECT_EQ(counts.subPathStarts, 1);
  EXPECT_EQ(counts.lines, 3);  // one per anchor after the first
  EXPECT_EQ(counts.closes, 1);
  EXPECT_EQ(counts.quadratics, 0);
}

// No anchors is not a degenerate path, it is no path -- the caller fills
// nothing rather than a stray shape at the window origin.
TEST(ElevationSurfaces, anchorsToPathReturnsAnEmptyPathForNoAnchors) {
  EXPECT_TRUE(ElevationSurfaces::anchorsToPath(identityTransform(), kWindow, {})
                  .isEmpty());
}

// The stitched surface spans the whole room in both directions: a walk that
// stopped early on either edge would shrink these bounds.
TEST(ElevationSurfaces, sampledEdgesToPathSpansBothRoomEdges) {
  std::vector<Coordinates::Point4D> leftEdge, rightEdge;
  makeEdges(3, leftEdge, rightEdge);

  const juce::Path path = ElevationSurfaces::sampledEdgesToPath(
      identityTransform(), kWindow, leftEdge, rightEdge);

  EXPECT_FALSE(path.isEmpty());
  const juce::Rectangle<float> bounds = path.getBounds();
  EXPECT_NEAR(bounds.getX(), 0.f, kTolerance);
  EXPECT_NEAR(bounds.getRight(), 200.f, kTolerance);
  EXPECT_NEAR(bounds.getY(), 0.f, kTolerance);
  EXPECT_NEAR(bounds.getBottom(), 100.f, kTolerance);
}

// Every sample on both edges is consumed exactly once: N samples per edge
// yield 2 * (N - 2) quadratic segments, one span across the far bound, and one
// closing span across the near bound. This is the assertion an off-by-one in
// either index walk fails.
TEST(ElevationSurfaces, sampledEdgesToPathConsumesEverySampleOnce) {
  for (const int sampleCount : {3, 32, 41, 81}) {
    std::vector<Coordinates::Point4D> leftEdge, rightEdge;
    makeEdges(sampleCount, leftEdge, rightEdge);

    const juce::Path path = ElevationSurfaces::sampledEdgesToPath(
        identityTransform(), kWindow, leftEdge, rightEdge);
    const PathElementCounts counts = countElements(path);

    EXPECT_EQ(counts.subPathStarts, 1) << "sampleCount = " << sampleCount;
    EXPECT_EQ(counts.quadratics, 2 * (sampleCount - 2))
        << "sampleCount = " << sampleCount;
    EXPECT_EQ(counts.lines, 1) << "sampleCount = " << sampleCount;
    EXPECT_EQ(counts.closes, 1) << "sampleCount = " << sampleCount;
  }
}

// Fewer than three samples cannot describe a curved edge, and the index walk
// has no valid bounds there, so the surface is refused rather than built from
// a wrapped-around index.
TEST(ElevationSurfaces, sampledEdgesToPathRefusesTooFewSamples) {
  for (const int sampleCount : {0, 1, 2}) {
    std::vector<Coordinates::Point4D> leftEdge, rightEdge;
    for (int i = 0; i < sampleCount; ++i) {
      leftEdge.push_back({-1.f, 0.f, 0.f, 1.f});
      rightEdge.push_back({1.f, 0.f, 0.f, 1.f});
    }
    EXPECT_TRUE(ElevationSurfaces::sampledEdgesToPath(
                    identityTransform(), kWindow, leftEdge, rightEdge)
                    .isEmpty())
        << "sampleCount = " << sampleCount;
  }
}

// Mismatched edges would pair a sample with the wrong opposite sample, so the
// surface is refused rather than built from an out-of-range read.
TEST(ElevationSurfaces, sampledEdgesToPathRefusesMismatchedEdges) {
  std::vector<Coordinates::Point4D> leftEdge, rightEdge;
  makeEdges(8, leftEdge, rightEdge);
  rightEdge.pop_back();

  EXPECT_TRUE(ElevationSurfaces::sampledEdgesToPath(
                  identityTransform(), kWindow, leftEdge, rightEdge)
                  .isEmpty());
}

// The arch and the dome crest at the room's centre line, so each is drawn as a
// front half and a back half in different shades. The split lands on the sample
// AT the centre line, which both halves keep -- that shared sample is what
// makes them meet at the crest instead of leaving a seam.
TEST(ElevationSurfaces, frontBackSplitIndexFindsTheCentreLineSample) {
  // The arch's own sampling: 41 samples, front/back from -1 to 1, so sample 20
  // sits exactly on the centre line.
  std::vector<Coordinates::Point4D> edge;
  for (int i = 0; i < 41; ++i) {
    const float frontBack = -1.f + i * 0.05f;
    edge.push_back({-1.f, 0.f, frontBack, 1.f});
  }

  const size_t split = ElevationSurfaces::frontBackSplitIndex(edge);

  EXPECT_EQ(split, 20u);
  EXPECT_NEAR(edge[split].a[2], 0.f, kTolerance);
  // Both halves keep sample 20: the front half is [0, 20] and the back half is
  // [20, 41), so each has 21 samples and neither is degenerate.
  EXPECT_EQ(split + 1, 21u);
  EXPECT_EQ(edge.size() - split, 21u);
}

// No sample need land exactly on the centre line -- an even sample count
// straddles it. The split then takes the first sample behind it, so the two
// halves still tile the edge with no gap and no overlap beyond the shared
// bound.
TEST(ElevationSurfaces, frontBackSplitIndexStraddlesTheCentreLine) {
  // Four samples at -0.75, -0.25, 0.25, 0.75: none is zero.
  std::vector<Coordinates::Point4D> edge;
  for (int i = 0; i < 4; ++i) {
    edge.push_back({-1.f, 0.f, -0.75f + i * 0.5f, 1.f});
  }

  const size_t split = ElevationSurfaces::frontBackSplitIndex(edge);

  EXPECT_EQ(split, 2u);
  EXPECT_GT(edge[split].a[2], 0.f);
  EXPECT_LT(edge[split - 1].a[2], 0.f);
}

// An edge wholly in front of the centre line has no back half. The index then
// addresses one past the last sample, which is a valid empty range rather than
// an out-of-range read -- the case that would otherwise walk off the end.
TEST(ElevationSurfaces, frontBackSplitIndexReturnsTheSizeWhenNoSampleIsBehind) {
  std::vector<Coordinates::Point4D> edge;
  for (int i = 0; i < 5; ++i) {
    edge.push_back({-1.f, 0.f, -1.f + i * 0.1f, 1.f});
  }

  EXPECT_EQ(ElevationSurfaces::frontBackSplitIndex(edge), edge.size());
}

// The mirror case: an edge wholly at or behind the centre line has no front
// half, so the split is at the very first sample.
TEST(ElevationSurfaces, frontBackSplitIndexReturnsZeroWhenEverySampleIsBehind) {
  std::vector<Coordinates::Point4D> edge;
  for (int i = 0; i < 5; ++i) {
    edge.push_back({-1.f, 0.f, i * 0.25f, 1.f});
  }

  EXPECT_EQ(ElevationSurfaces::frontBackSplitIndex(edge), 0u);
}

// An empty edge has nothing to split; the size is the only answer that keeps
// both derived ranges empty rather than inverted.
TEST(ElevationSurfaces, frontBackSplitIndexHandlesAnEmptyEdge) {
  EXPECT_EQ(ElevationSurfaces::frontBackSplitIndex({}), 0u);
}

// Each half of a split arch is still a well-formed surface: 21 samples yields
// the same element counts sampledEdgesToPath produces for any 21-sample edge,
// so neither half is truncated by the slicing.
TEST(ElevationSurfaces, splitArchHalvesAreBothWellFormedSurfaces) {
  std::vector<Coordinates::Point4D> leftEdge, rightEdge;
  makeEdges(41, leftEdge, rightEdge);
  const size_t split = 20;

  const juce::Path frontHalf = ElevationSurfaces::sampledEdgesToPath(
      identityTransform(), kWindow,
      {leftEdge.begin(), leftEdge.begin() + split + 1},
      {rightEdge.begin(), rightEdge.begin() + split + 1});
  const juce::Path backHalf = ElevationSurfaces::sampledEdgesToPath(
      identityTransform(), kWindow, {leftEdge.begin() + split, leftEdge.end()},
      {rightEdge.begin() + split, rightEdge.end()});

  const auto expectWellFormed = [](const juce::Path& half, const char* name) {
    const PathElementCounts counts = countElements(half);
    EXPECT_EQ(counts.subPathStarts, 1) << name;
    EXPECT_EQ(counts.quadratics, 2 * (21 - 2)) << name;
    EXPECT_EQ(counts.lines, 1) << name;
    EXPECT_EQ(counts.closes, 1) << name;
  };
  expectWellFormed(frontHalf, "front");
  expectWellFormed(backHalf, "back");
}

// The window drives the projection: the same surface drawn into a window twice
// the size scales with it rather than staying at a pixel constant.
TEST(ElevationSurfaces, surfacesTrackTheWindowSize) {
  const std::vector<Coordinates::Point4D> kAnchors = {{-1.f, -1.f, 0.f, 1.f},
                                                      {1.f, -1.f, 0.f, 1.f},
                                                      {1.f, 1.f, 0.f, 1.f},
                                                      {-1.f, 1.f, 0.f, 1.f}};
  const Coordinates::WindowData kDoubled = {.leftCornerX = 0.f,
                                            .bottomCornerY = 200.f,
                                            .width = 400.f,
                                            .height = 200.f};

  const juce::Rectangle<float> bounds =
      ElevationSurfaces::anchorsToPath(identityTransform(), kDoubled, kAnchors)
          .getBounds();

  EXPECT_NEAR(bounds.getWidth(), 400.f, kTolerance);
  EXPECT_NEAR(bounds.getHeight(), 200.f, kTolerance);
}
