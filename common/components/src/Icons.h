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

#include "../components.h"
#include "BinaryData.h"

#define LOAD_IMAGE(name)                                     \
 private:                                                    \
  juce::Image name##icon_ = juce::ImageFileFormat::loadFrom( \
      BinaryData::name##_png, BinaryData::name##_pngSize);   \
                                                             \
 public:                                                     \
  juce::Image get##name##Icon() { return name##icon_; }

class IconStore {
 public:
  static IconStore& getInstance() {
    // In C++ 11 this is guaranteed to be thread safe
    static IconStore instance;
    return instance;
  }

  IconStore(IconStore const&) = delete;
  void operator=(IconStore const&) = delete;

  IconStore() {}

  /* =========================================================================
   * To add a new icon:
     - Place icon in "icons" folder
     - Update CMakeLists.txt in the "icons" folder to include the new icon
     - Run the build, which regnerates the JUCE binary data file
     - Add a new LOAD_IMAGE macro for the new icon, using the icon's filename
     - Fetch your new icon with IconStore::getInstance().getNewIcon()
  ============================================================================*/

  LOAD_IMAGE(BackArrow);
  LOAD_IMAGE(Delete);
  LOAD_IMAGE(Track);
  LOAD_IMAGE(Tooltip);
  LOAD_IMAGE(Folder);
  LOAD_IMAGE(Headphones);
  LOAD_IMAGE(Reset);
  LOAD_IMAGE(Checkmark);
  LOAD_IMAGE(Back);
  LOAD_IMAGE(Iso);
  LOAD_IMAGE(Left);
  LOAD_IMAGE(Top);
  LOAD_IMAGE(Add);
  LOAD_IMAGE(RemoveAE);
  LOAD_IMAGE(Plus);
  LOAD_IMAGE(ArchElevation);
  LOAD_IMAGE(CurveElevation);
  LOAD_IMAGE(FlatElevation);
  LOAD_IMAGE(DomeElevation);
  LOAD_IMAGE(TentElevation);
  LOAD_IMAGE(LeftChevron);
  LOAD_IMAGE(RightChevron);
  LOAD_IMAGE(Edit);
  LOAD_IMAGE(Carat);
  LOAD_IMAGE(settings);
  LOAD_IMAGE(Line);
  LOAD_IMAGE(Close);
};