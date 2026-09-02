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
#include <boost/qvm.hpp>

namespace Coordinates {
using Point2D = boost::qvm::vec<float, 2>;
using Point3D = boost::qvm::vec<float, 3>;
using Point4D = boost::qvm::vec<float, 4>;
using Mat4 = boost::qvm::mat<float, 4, 4>;

// Wrapper type for passing window data to the coordinate calculator.
struct WindowData {
  float leftCornerX, bottomCornerY, width, height;
};

/**
 * @brief Given a transform matrix and window data, convert a homogeneous 3D
 * coordinate to a 2D screen position.
 *
 * @param transformMat
 * @param windowData
 * @param point
 * @return Point2D
 */
Point2D toWindow(const Mat4& transformMat, const WindowData& windowData,
                 const Point4D point);

// Two coordinate conventions meet in the room views, and this is the only
// place the conversion between them is expressed.
//
//   Position-parameter space -- the automatable APVTS parameters reached
//   through AudioElementParameterTree: x = left/right, y = front/back,
//   z = height, integers over -kPositionExtent..kPositionExtent.
//
//   Room-view NDC -- the space room geometry and speakers are declared in
//   (see SpeakerLookup.h): x = left/right, y = up, z = front/back, floats
//   over -1..1.
//
// Converting between them is an axis swap (parameter y <-> NDC z, parameter
// z <-> NDC y, with front/back inverted in sign) plus a /kPositionExtent
// scale. Callers must never repeat that arithmetic themselves.
// fromRoomNdc recovers the original integer exactly for every parameter value
// in range. That exactness is a property of THIS extent, not of float round
// trips generally: 50 divides and re-multiplies without loss across
// -50..50 in IEEE-754 single precision. Re-verify it (Coordinates_test.cpp,
// roundTripIsExactOverTheFullRange) before changing this constant, and if the
// build ever enables -ffast-math or relaxes -ffp-contract.
constexpr float kPositionExtent = 50.f;

// A source position in position-parameter space. Integral because the
// parameters are: each axis has kPositionExtent * 2 + 1 discrete steps.
struct PositionParameters {
  int x;  // left/right
  int y;  // front/back
  int z;  // height
};

/**
 * @brief Convert a position in parameter space to a homogeneous room-view NDC
 * point.
 *
 * Takes floats rather than PositionParameters so the draw path can pass
 * already-float track data through without a lossy narrowing round trip.
 *
 * @param x left/right position parameter
 * @param y front/back position parameter
 * @param z height position parameter
 * @return Point4D the same position in room-view NDC, w = 1
 */
Point4D toRoomNdc(const float x, const float y, const float z);

/**
 * @brief Convert a homogeneous room-view NDC point back to position
 * parameters, quantized to the integer parameter domain.
 *
 * The inverse of toRoomNdc: round-tripping any valid parameter triple through
 * both returns the original integers.
 *
 * @param ndcPoint a room-view NDC point with w = 1
 * @return PositionParameters
 */
PositionParameters fromRoomNdc(const Point4D& ndcPoint);

constexpr Mat4 getRearViewTransform() {
  /**
   * Generated with the following code:
   * model = glm::scale(model, glm::vec3(1.2f, 0.9f, 2.5f));
   * view  = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));
   * projection = glm::perspective(glm::radians(45.0f), SCR_WIDTH / SCR_HEIGHT,
   *  0.1f, 100.0f);
   */
  return {{
      {2.19693f, 0.0f, 0.0f, 0.0f},
      {0.0f, 2.17279f, 0.0f, 0.0f},
      {0.0f, 0.0f, -2.505f, -2.5f},
      {0.0f, 0.0f, 4.80981f, 5.0f},
  }};
}

constexpr Mat4 getSideViewTransform() {
  /**
   * Generated with the following code:
   * model = glm::scale(model, glm::vec3(0.9f, 1.0f, 1.3f));
   * view  = glm::translate(view, glm::vec3(0.0f, 0.0f, -4.0f)) *
   *  glm::rotate(view, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
   * projection = glm::perspective(glm::radians(45.0f), SCR_WIDTH / SCR_HEIGHT,
   *  0.1f, 100.0f);
   */
  return {{
      {0.0f, 0.0f, 0.901802f, 0.9f},
      {0.0f, 2.41421f, 0.0f, 0.0f},
      {2.38001f, 0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 3.80781f, 4.0f},
  }};
}

// The panner's projection. Perspective, not orthographic: a point higher in the
// room projects further from the room's centre.
constexpr Mat4 getTopViewTransform() {
  /**
   * Generated with the following code:
   * model = glm::scale(model, glm::vec3(1.2f, 1.f, 1.4f));
   * view  = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f)) *
   *  glm::rotate(view, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
   * projection = glm::perspective(glm::radians(45.0f), SCR_WIDTH / SCR_HEIGHT,
   * 0.1f, 100.0f);
   */
  return {{
      {2.19693f, 0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, -1.002f, -1.0f},
      {0.0f, -3.3799f, 0.0f, 0.0f},
      {0.0f, 0.0f, 4.80981f, 5.0f},
  }};
}

Mat4 getIsoViewTransform();
}  // namespace Coordinates