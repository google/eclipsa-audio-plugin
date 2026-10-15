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

#include "Coordinates.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <cmath>

namespace Coordinates {

Point4D toRoomNdc(const float x, const float y, const float z) {
  // Axis swap plus scale: parameter height becomes NDC up, and parameter
  // front/back becomes NDC z with its sign inverted.
  return {x / kPositionExtent, z / kPositionExtent, -y / kPositionExtent, 1.f};
}

PositionParameters fromRoomNdc(const Point4D& ndcPoint) {
  // Round rather than truncate: the parameter domain is integral, and the
  // /kPositionExtent scale is not exactly invertible in binary floating point.
  return {
      static_cast<int>(std::lround(ndcPoint.a[0] * kPositionExtent)),
      static_cast<int>(std::lround(-ndcPoint.a[2] * kPositionExtent)),
      static_cast<int>(std::lround(ndcPoint.a[1] * kPositionExtent)),
  };
}

Point4D toPlanPlane(const Point4D& ndcPoint) {
  return {ndcPoint.a[0], kPlanPlaneUp, ndcPoint.a[2], 1.f};
}

Point2D toWindow(const Mat4& transformMat, const WindowData& windowData,
                 const Point4D point) {
  // Apply transform.
  Point4D nPoint = point * transformMat;

  // Apply Perspective Division.
  Point3D ndcPoint = {nPoint.a[0] / nPoint.a[3], nPoint.a[1] / nPoint.a[3],
                      nPoint.a[2] / nPoint.a[3]};

  // Apply NDC to Window Coordinates transformation.
  const float w2 = windowData.width / 2;
  const float h2 = windowData.height / 2;
  return {w2 * ndcPoint.a[0] + windowData.leftCornerX + w2,
          -h2 * ndcPoint.a[1] + windowData.bottomCornerY - h2};
}

std::optional<Point4D> fromTopViewWindow(const Mat4& transformMat,
                                         const WindowData& windowData,
                                         const Point2D& windowPoint,
                                         const float ndcUp) {
  // A host can collapse the editor mid-drag, and the division below would hand
  // fromRoomNdc a non-finite value, which std::lround does not define.
  if (windowData.width <= 0.f || windowData.height <= 0.f) {
    return std::nullopt;
  }
  // Recover the NDC coordinates toWindow mapped into the window.
  const float w2 = windowData.width / 2;
  const float h2 = windowData.height / 2;
  const float ndcX = (windowPoint.a[0] - windowData.leftCornerX - w2) / w2;
  const float ndcY = -(windowPoint.a[1] - windowData.bottomCornerY + h2) / h2;

  // Read the scale factors through the same product toWindow applies.
  const Point4D kAtCentre = Point4D{0.f, ndcUp, 0.f, 1.f} * transformMat;
  const Point4D kAtUnitLeftRight = Point4D{1.f, ndcUp, 0.f, 1.f} * transformMat;
  const Point4D kAtUnitFrontBack = Point4D{0.f, ndcUp, 1.f, 1.f} * transformMat;

  // At a fixed height the divisor is constant and each window axis follows one
  // room axis.
  const float kDivisor = kAtCentre.a[3];
  const float kLeftRightScale = kAtUnitLeftRight.a[0] - kAtCentre.a[0];
  const float kFrontBackScale = kAtUnitFrontBack.a[1] - kAtCentre.a[1];

  return Point4D{(ndcX * kDivisor - kAtCentre.a[0]) / kLeftRightScale, ndcUp,
                 (ndcY * kDivisor - kAtCentre.a[1]) / kFrontBackScale, 1.f};
}

Mat4 getIsoViewTransform() {
  const Mat4 ortho = {{
      {0.505556f, 0.0f, 0.0f, 0.0f},
      {0.f, 0.66667f, 0.0f, 0.0f},
      {0.f, 0.f, -0.02002, 0.f},
      {0.f, 0.f, -1.f, 1.f},
  }};

  Mat4 model, view, scale, identity = boost::qvm::identity_mat<float, 4>();
  model = view = scale = identity;
  model = boost::qvm::rotx_mat<4>((2.2f));
  model *= boost::qvm::roty_mat<4>((-2.39f));
  model *= boost::qvm::rotz_mat<4>((2.39f));
  scale.a[0][0] = (1.36f);
  scale.a[1][1] = (0.82f);
  model *= scale;
  view.a[3][2] = -5.f;
  return model * view * ortho;
}
}  // namespace Coordinates