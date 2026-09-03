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

#include <cstddef>
#include <vector>

#include "Coordinates.h"

// Path assembly for the audio element panner's elevation surfaces.
//
// These live in a header of their own, rather than in an anonymous namespace
// inside PerspectiveRoomViews.cpp, for two reasons. They carry the index
// arithmetic that a wrong bound would silently turn into a truncated or
// self-intersecting surface, and a test can only reach them from here
// (ElevationSurfaces_test.cpp). And PerspectiveRoomViews.cpp is unity-built
// into components.cpp with twenty other sources, where an anonymous namespace
// is shared across the whole translation unit -- a named namespace cannot
// collide with a helper some other source adds later.
namespace ElevationSurfaces {

/**
 * @brief Close a run of 3D anchor vertices into a window-space path with
 * straight edges.
 *
 * Every elevation surface is built from anchors put through
 * Coordinates::toWindow, so resizing the plugin window keeps the surface
 * aligned to the drawn room instead of to a pixel constant.
 *
 * @param transformMat the view's projection
 * @param window the current window bounds
 * @param anchors the surface's corners, in draw order
 * @return juce::Path a closed path, empty when there are no anchors
 */
inline juce::Path anchorsToPath(
    const Coordinates::Mat4& transformMat,
    const Coordinates::WindowData& window,
    const std::vector<Coordinates::Point4D>& anchors) {
  juce::Path path;
  if (anchors.empty()) {
    return path;
  }
  const Coordinates::Point2D start =
      Coordinates::toWindow(transformMat, window, anchors.front());
  path.startNewSubPath(start.a[0], start.a[1]);
  for (size_t i = 1; i < anchors.size(); ++i) {
    const Coordinates::Point2D pt =
        Coordinates::toWindow(transformMat, window, anchors[i]);
    path.lineTo(pt.a[0], pt.a[1]);
  }
  path.closeSubPath();
  return path;
}

/**
 * @brief Stitch two sampled edges of one curved surface into a single filled
 * surface spanning left to right.
 *
 * The two edges are the left and right edges of the room, sampled at the same
 * front/back positions. The path runs the whole left edge, spans the room at
 * the far bound, runs the right edge back, and closes across the near bound.
 * quadraticTo is carried over from the arch the removed rear projection drew
 * (AudioElementPluginRearView::paintArchElevation, last present at 81dad4c^)
 * so a sampled edge stays smooth rather than showing its facets.
 *
 * @param transformMat the view's projection
 * @param window the current window bounds
 * @param leftEdge samples along the room's left edge, near bound first
 * @param rightEdge the same samples along the room's right edge
 * @return juce::Path a closed path, empty when the edges are shorter than
 * three samples or are not the same length
 */
inline juce::Path sampledEdgesToPath(
    const Coordinates::Mat4& transformMat,
    const Coordinates::WindowData& window,
    const std::vector<Coordinates::Point4D>& leftEdge,
    const std::vector<Coordinates::Point4D>& rightEdge) {
  juce::Path path;
  if (leftEdge.size() < 3 || leftEdge.size() != rightEdge.size()) {
    return path;
  }

  std::vector<Coordinates::Point2D> left, right;
  left.reserve(leftEdge.size());
  right.reserve(rightEdge.size());
  for (size_t i = 0; i < leftEdge.size(); ++i) {
    left.push_back(Coordinates::toWindow(transformMat, window, leftEdge[i]));
    right.push_back(Coordinates::toWindow(transformMat, window, rightEdge[i]));
  }

  // The guard above puts kLast at 2 or more, so kLast - 1 is at least 1 and
  // neither bound below can wrap around on an unsigned index.
  const size_t kLast = left.size() - 1;
  path.startNewSubPath(left[0].a[0], left[0].a[1]);
  for (size_t i = 1; i <= kLast - 1; ++i) {
    path.quadraticTo(left[i].a[0], left[i].a[1], left[i + 1].a[0],
                     left[i + 1].a[1]);
  }
  // Span the room at the far front/back bound, then walk the right edge back.
  path.lineTo(right[kLast].a[0], right[kLast].a[1]);
  for (size_t i = kLast - 1; i >= 1; --i) {
    path.quadraticTo(right[i].a[0], right[i].a[1], right[i - 1].a[0],
                     right[i - 1].a[1]);
  }
  // Closing the subpath spans the room at the near bound.
  path.closeSubPath();
  return path;
}

/**
 * @brief Index of the first sample at or behind the room's centre line.
 *
 * A surface that crests in the middle of the room has a front half facing the
 * light the tent's two shades imply and a back half facing away from it, so it
 * is drawn as two shaded surfaces meeting at the crest. This finds where to cut
 * a sampled edge: the front half is [0, index], the back half is [index, end),
 * and the two SHARE the returned sample so no seam opens between them.
 *
 * The crest is at the centre line because that is where the arch's parabola and
 * the dome's hemisphere both peak (Elevation.h). Searching for the coordinate
 * rather than assuming a sample lands exactly on zero keeps the split correct
 * if the sample count changes and no sample sits on the centre line.
 *
 * @param edge samples along one room edge, near bound first
 * @return size_t the first index whose front/back coordinate is at or behind
 * the centre line, or edge.size() when every sample is in front of it
 */
inline size_t frontBackSplitIndex(
    const std::vector<Coordinates::Point4D>& edge) {
  for (size_t i = 0; i < edge.size(); ++i) {
    if (edge[i].a[2] >= 0.f) {
      return i;
    }
  }
  return edge.size();
}

}  // namespace ElevationSurfaces
