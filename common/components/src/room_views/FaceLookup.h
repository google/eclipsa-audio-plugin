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

#include <array>
#include <vector>

#include "../EclipsaColours.h"
#include "Coordinates.h"

/**
 * @brief Raw face data from which to construct a room to be displayed. Face
 * data includes corner and grid vertices (in NDC), as well as the colour of the
 * face and colour of the gridlines.
 *
 */
namespace FaceLookup {
enum PerspectiveView {
  kTop,
  kSide,
  kRear,
  kIso,
  // The top view with the audio element panner's front-wall screen added.
  kPannerTop,
};
enum class FaceTag {
  kFront,
  kBack,
  kLeft,
  kRight,
  kTop,
  kBottom,
  kIsoLeft,
  kIsoBack,
  kScreen,
};
enum NormalAxis {
  kAxisX,
  kAxisY,
  kAxisZ,
};

static const int kNumGridLines = 3;

struct Face {
  constexpr Face(const std::array<Coordinates::Point4D, 4>& cornerVertices,
                 const juce::Colour& colour, const juce::Colour& gridColour,
                 const FaceTag tag)
      : cornerVertices(cornerVertices),
        faceColour(colour),
        gridColour(gridColour),
        tag(tag) {
    // For each corner vertex, generate kNumGridLines gridline anchor vertices
    // between the current vertex and the next.
    std::array<Coordinates::Point4D, kNumGridLines * 4> gridVertices;
    for (int i = 0, gridIdx = 0; i < cornerVertices.size(); ++i) {
      const auto& start = cornerVertices[i];
      const auto& end = cornerVertices[(i + 1) % cornerVertices.size()];
      for (int j = 1; j <= kNumGridLines; ++j) {
        float t = (float)j / (kNumGridLines + 1);
        gridVertices[gridIdx++] = Coordinates::Point4D{
            start.a[0] + t * (end.a[0] - start.a[0]),
            start.a[1] + t * (end.a[1] - start.a[1]),
            start.a[2] + t * (end.a[2] - start.a[2]), 1.0f};
      }
    }
    // Store gridline vertices of parallel edges in pairs for easy drawing.
    // Only need to iterate over half the total gridline vertices to pair all.
    for (int i = 0; i < kNumGridLines * 2; ++i) {
      int paraEdgeStartIdx =
          kNumGridLines * 2 + (i / kNumGridLines) * kNumGridLines;
      gridlineVertices[i] = {gridVertices[i],
                             gridVertices[paraEdgeStartIdx + kNumGridLines -
                                          i % kNumGridLines - 1]};
    }
  }

  std::array<Coordinates::Point4D, 4> cornerVertices;
  std::array<std::pair<Coordinates::Point4D, Coordinates::Point4D>,
             kNumGridLines * 2>
      gridlineVertices;
  juce::Colour faceColour, gridColour;
  FaceTag tag;
};

// Room corner vertices.
const std::vector<Coordinates::Point4D> kRoomVerts = {
    {-1.0f, 1.0f, -1.0f, 1.0f},   // Top-left-front
    {1.0f, 1.0f, -1.0f, 1.0f},    // Top-right-front
    {1.0f, -1.0f, -1.0f, 1.0f},   // Bottom-right-front
    {-1.0f, -1.0f, -1.0f, 1.0f},  // Bottom-left-front
    {-1.0f, 1.0f, 1.0f, 1.0f},    // Top-left-back
    {1.0f, 1.0f, 1.0f, 1.0f},     // Top-right-back
    {1.0f, -1.0f, 1.0f, 1.0f},    // Bottom-right-back
    {-1.0f, -1.0f, 1.0f, 1.0f},   // Bottom-left-back
};

// Create faces as subsets of the room vertices.
const Face kFrontFace = {
    {kRoomVerts[0], kRoomVerts[1], kRoomVerts[2], kRoomVerts[3]},
    EclipsaColours::roomviewLightWall,
    EclipsaColours::roomviewLightGrid,
    FaceTag::kFront};
const Face kBackFace = {
    {kRoomVerts[4], kRoomVerts[5], kRoomVerts[6], kRoomVerts[7]},
    EclipsaColours::roomviewDarkWall,
    EclipsaColours::roomviewDarkGrid,
    FaceTag::kBack};
const Face kLeftFace = {
    {kRoomVerts[0], kRoomVerts[4], kRoomVerts[7], kRoomVerts[3]},
    EclipsaColours::roomviewDarkWall,
    EclipsaColours::roomviewDarkGrid,
    FaceTag::kLeft};
const Face kRightFace = {
    {kRoomVerts[1], kRoomVerts[5], kRoomVerts[6], kRoomVerts[2]},
    EclipsaColours::roomviewDarkWall,
    EclipsaColours::roomviewDarkGrid,
    FaceTag::kRight};
const Face kTopFace = {
    {kRoomVerts[0], kRoomVerts[1], kRoomVerts[5], kRoomVerts[4]},
    EclipsaColours::roomviewDarkWall,
    EclipsaColours::roomviewDarkGrid,
    FaceTag::kTop};
const Face kBottomFace = {
    {kRoomVerts[3], kRoomVerts[2], kRoomVerts[6], kRoomVerts[7]},
    EclipsaColours::roomviewDarkWall,
    EclipsaColours::roomviewDarkGrid,
    FaceTag::kBottom};
const Face kIsoLeftFace = {
    {kRoomVerts[0], kRoomVerts[4], kRoomVerts[7], kRoomVerts[3]},
    EclipsaColours::roomviewIsoTransparentWall,
    juce::Colours::transparentBlack,
    FaceTag::kIsoLeft};
const Face kIsoBackFace = {
    {kRoomVerts[4], kRoomVerts[5], kRoomVerts[6], kRoomVerts[7]},
    EclipsaColours::roomviewIsoTransparentWall,
    juce::Colours::transparentBlack,
    FaceTag::kIsoBack};

// A screen centred on the front wall, spanning three quarters of its width and
// height. Wound like kFrontFace.
const Face kFrontScreenFace = {
    {Coordinates::Point4D{-0.75f, 0.75f, -1.f, 1.f},
     Coordinates::Point4D{0.75f, 0.75f, -1.f, 1.f},
     Coordinates::Point4D{0.75f, -0.75f, -1.f, 1.f},
     Coordinates::Point4D{-0.75f, -0.75f, -1.f, 1.f}},
    EclipsaColours::roomviewScreen,
    juce::Colours::transparentBlack,
    FaceTag::kScreen};

// Make face sets queryable by tag.
inline std::vector<Face> getFaces(const PerspectiveView view) {
  switch (view) {
    case PerspectiveView::kTop:
      return {kFrontFace, kLeftFace, kRightFace, kBackFace, kBottomFace};
    case PerspectiveView::kSide:
      return {kFrontFace, kRightFace, kTopFace, kBottomFace, kBackFace};
    case PerspectiveView::kRear:
      return {kFrontFace, kLeftFace, kRightFace, kTopFace, kBottomFace};
    case PerspectiveView::kIso:
      return {kFrontFace, kBottomFace, kRightFace, kIsoBackFace, kIsoLeftFace};
    case PerspectiveView::kPannerTop: {
      // Last, so it draws over the front wall it sits on.
      std::vector<Face> faces = getFaces(PerspectiveView::kTop);
      faces.push_back(kFrontScreenFace);
      return faces;
    }
    default:
      return {kFrontFace, kLeftFace, kRightFace, kBottomFace, kBackFace};
  }
}
}  // namespace FaceLookup