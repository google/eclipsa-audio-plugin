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

namespace Coordinates {

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