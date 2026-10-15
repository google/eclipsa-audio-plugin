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

#include "data_structures/src/AudioElementSpatialLayout.h"

// Where the panner's listener head sits in the draw order.
//
// Placed in a header so it can be reached by tests.
namespace ListenerHead {

/**
 * @brief Whether the head is drawn before the elevation surface, so the
 * surface tints it.
 *
 * The head sits at the room's centre at height 0. Tent, Arch, Dome and Curve
 * are all above 0 there, so they pass over it; Flat passes over it unless its
 * height is below 0.
 *
 * @param elevation the panner's elevation pattern
 * @param flatHeight the Flat pattern's height in room-view NDC, -1 to 1
 */
inline bool drawnBeforeElevation(
    const AudioElementSpatialLayout::Elevation elevation,
    const float flatHeight) {
  if (elevation == AudioElementSpatialLayout::Elevation::kFlat) {
    return flatHeight >= 0.f;
  }
  return true;
}

}  // namespace ListenerHead
