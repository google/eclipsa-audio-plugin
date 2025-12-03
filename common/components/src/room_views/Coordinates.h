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

constexpr Mat4 getRearViewTransform();
constexpr Mat4 getSideViewTransform();
constexpr Mat4 getTopViewTransform();
Mat4 getIsoViewTransform();
}  // namespace Coordinates