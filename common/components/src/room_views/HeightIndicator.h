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
#include <algorithm>
#include <array>
#include <vector>

#include "Coordinates.h"

// Geometry for the audio element panner's height indicator: the room's
// horizontal cross-section at the source's height, plus the two leader lines
// tying the source to it.
//
// The panner's vertical screen axis carries front/back, so height has no axis
// of its own. The cross-section supplies it: it rests on the room's sides at
// the source's height, and the top view's perspective projection expands it
// toward the walls as the source rises.
//
// Placed in a header so it can be reached by tests.
namespace HeightIndicator {

// A straight edge, held as its two 3D anchors so the projection is applied to
// the endpoints rather than to an already-flattened screen line.
struct Segment {
  Coordinates::Point4D start;
  Coordinates::Point4D end;
};

/**
 * @brief The room's horizontal cross-section at one height, as four segments.
 *
 * The anchors are the room bounds at that height and the segments run around
 * them in order, so drawing all four puts a line along every side. Outline
 * only -- the elevation surfaces are the filled shapes, and this sits over
 * them.
 *
 * @param height the source's height in room-view NDC (-1..1)
 * @return std::array<Segment, 4> the four sides, in draw order
 */
inline std::array<Segment, 4> crossSectionOutline(const float height) {
  const Coordinates::Point4D kFrontLeft = {-1.f, height, -1.f, 1.f};
  const Coordinates::Point4D kFrontRight = {1.f, height, -1.f, 1.f};
  const Coordinates::Point4D kBackRight = {1.f, height, 1.f, 1.f};
  const Coordinates::Point4D kBackLeft = {-1.f, height, 1.f, 1.f};

  return {Segment{kFrontLeft, kFrontRight}, Segment{kFrontRight, kBackRight},
          Segment{kBackRight, kBackLeft}, Segment{kBackLeft, kFrontLeft}};
}

// Runs of the indicator, split by which side of the elevation surface each one
// lies on. A painter's algorithm has no depth buffer, so the two lists ARE the
// draw order: `below` is drawn before the surface is filled and is tinted by
// it, `above` after and stays full strength. Carries the outline's runs and the
// connectors' alike -- the same walk splits both.
struct SplitOutline {
  std::vector<Segment> below;
  std::vector<Segment> above;
};

/**
 * @brief A point partway along a segment, at parameter t in [0, 1].
 *
 * All three components are interpolated, so a segment whose ends sit at
 * different heights is handled too.
 *
 * @param segment the segment to walk
 * @param t the parameter, 0 at start and 1 at end
 * @return Coordinates::Point4D the interpolated anchor, w = 1
 */
inline Coordinates::Point4D pointAlong(const Segment& segment, const float t) {
  const auto kLerp = [t](const float from, const float to) {
    return from + (to - from) * t;
  };
  return {kLerp(segment.start.a[0], segment.end.a[0]),
          kLerp(segment.start.a[1], segment.end.a[1]),
          kLerp(segment.start.a[2], segment.end.a[2]), 1.f};
}

/**
 * @brief Split one segment where it crosses an elevation surface, appending its
 * runs to a split.
 *
 * Used for every part of the indicator, so a point where the outline and a
 * connector meet is always classified the same way.
 *
 * The crossing is found by sampling and bisection rather than by solving each
 * pattern: it then matches the faceted surface actually drawn, and handles a
 * curve that crosses once as readily as one that crosses twice.
 *
 * @param segment the run to split
 * @param roofHeightAt the elevation surface's height at one (left/right,
 * front/back) position; returns the floor where the pattern has no surface.
 * Both coordinates are passed because the dome needs both
 * @param samples positions tested along the segment, at least 2
 * @param into the split to append to
 */
template <typename RoofHeightFn>
inline void splitSegmentInto(const Segment& segment,
                             RoofHeightFn&& roofHeightAt, const int samples,
                             SplitOutline& into) {
  const int kSamples = std::max(2, samples);

  // A point is ABOVE when it is at or over the surface. Ties resolve to above,
  // so a source resting exactly on the surface draws over it rather than
  // flickering between the two lists.
  const auto kIsAbove = [&](const float t) {
    const Coordinates::Point4D kAt = pointAlong(segment, t);
    return kAt.a[1] >= roofHeightAt(kAt.a[0], kAt.a[2]);
  };
  const auto kEmit = [&](const float from, const float to, const bool above) {
    // Skip the empty run a crossing landing on an end would emit.
    if (to - from <= 1e-6f) {
      return;
    }
    (above ? into.above : into.below)
        .push_back(Segment{pointAlong(segment, from), pointAlong(segment, to)});
  };

  float runStart = 0.f;
  bool runAbove = kIsAbove(0.f);
  for (int i = 1; i < kSamples; ++i) {
    const float kPrev = static_cast<float>(i - 1) / (kSamples - 1);
    const float kHere = static_cast<float>(i) / (kSamples - 1);
    if (kIsAbove(kHere) == runAbove) {
      continue;
    }
    // Bisect the bracketing interval rather than interpolating the height
    // difference: the tent's ridge is a crease, so that difference is not
    // linear across a sample step containing it.
    float lo = kPrev, hi = kHere;
    for (int step = 0; step < 20; ++step) {
      const float kMid = 0.5f * (lo + hi);
      if (kIsAbove(kMid) == runAbove) {
        lo = kMid;
      } else {
        hi = kMid;
      }
    }
    const float kCrossing = 0.5f * (lo + hi);
    kEmit(runStart, kCrossing, runAbove);
    runStart = kCrossing;
    runAbove = !runAbove;
  }
  kEmit(runStart, 1.f, runAbove);
}

/**
 * @brief Split the cross-section outline where it crosses an elevation surface.
 *
 * All four sides go through the same walk; none is special-cased.
 *
 * @param height the source's height in room-view NDC (-1..1)
 * @param roofHeightAt the elevation surface's height at one front/back position
 * @param samplesPerSide positions tested along each side, at least 2
 * @return SplitOutline the runs under the surface and the runs over it
 */
template <typename RoofHeightFn>
inline SplitOutline splitAtElevation(const float height,
                                     RoofHeightFn&& roofHeightAt,
                                     const int samplesPerSide = 41) {
  SplitOutline split;
  for (const Segment& side : crossSectionOutline(height)) {
    splitSegmentInto(side, roofHeightAt, samplesPerSide, split);
  }

  return split;
}

/**
 * @brief The two leader lines from a source to its cross-section outline.
 *
 * One runs to the outline's back edge at the source's own left/right position,
 * the other to its right edge at the source's own front/back position. Both
 * endpoints are built in 3D at the source's height, so they stay attached to
 * the outline under the perspective divide and on a window resize.
 *
 * @param source the source position in room-view NDC, w = 1
 * @return std::array<Segment, 2> the back-edge connector, then the right-edge
 * connector
 */
inline std::array<Segment, 2> leaderLines(const Coordinates::Point4D& source) {
  const float kHeight = source.a[1];
  const Coordinates::Point4D kToBackEdge = {source.a[0], kHeight, 1.f, 1.f};
  const Coordinates::Point4D kToRightEdge = {1.f, kHeight, source.a[2], 1.f};

  return {Segment{source, kToBackEdge}, Segment{source, kToRightEdge}};
}

/**
 * @brief Split the connectors against an elevation surface.
 *
 * The BACK-EDGE connector varies in front/back, so the surface beneath it
 * changes along its length under every pattern and it always goes through the
 * same walk the outline does.
 *
 * The RIGHT-EDGE connector holds the source's front/back position, so it only
 * crosses the surface where that surface varies with left/right too -- the
 * dome. Under the other patterns it is COINCIDENT with the surface rather than
 * either side of it, and a coincident line classified by comparison flickers,
 * so the caller states the distinction instead of it being inferred here.
 *
 * @param source the source position in room-view NDC, w = 1
 * @param roofHeightAt the surface height at one (left/right, front/back)
 * position
 * @param splitRightEdge whether the surface varies with left/right, so the
 * right-edge connector must be split rather than placed on top
 * @param samplesPerLine positions tested along each split connector, min 2
 * @return SplitOutline the runs under the surface and the runs over it
 */
template <typename RoofHeightFn>
inline SplitOutline splitLeaderLinesAtElevation(
    const Coordinates::Point4D& source, RoofHeightFn&& roofHeightAt,
    const bool splitRightEdge, const int samplesPerLine = 41) {
  SplitOutline split;
  const std::array<Segment, 2> kLeaders = leaderLines(source);

  splitSegmentInto(kLeaders[0], roofHeightAt, samplesPerLine, split);

  if (splitRightEdge) {
    splitSegmentInto(kLeaders[1], roofHeightAt, samplesPerLine, split);
  } else {
    split.above.push_back(kLeaders[1]);
  }

  return split;
}

/**
 * @brief Project one segment's anchors into window coordinates.
 *
 * @param transformMat the view's projection
 * @param window the current window bounds
 * @param segment the segment to project
 * @return std::array<Coordinates::Point2D, 2> its start and end on screen
 */
inline std::array<Coordinates::Point2D, 2> projectSegment(
    const Coordinates::Mat4& transformMat,
    const Coordinates::WindowData& window, const Segment& segment) {
  return {Coordinates::toWindow(transformMat, window, segment.start),
          Coordinates::toWindow(transformMat, window, segment.end)};
}

}  // namespace HeightIndicator
