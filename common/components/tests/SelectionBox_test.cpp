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

// clang-format off
#include <components/components.h>

#include "components/src/SelectionBox.h"
// clang-format on

#include <gtest/gtest.h>

// setOptionEnabled addresses options by index, which is one less than the
// combo box item id.
TEST(SelectionBoxOption, set_option_enabled_maps_index_to_item_id) {
  const juce::ScopedJuceInitialiser_GUI kJuce;
  SelectionBox box("Profile");
  box.addOption("Simple");
  box.addOption("Base");
  box.addOption("Base Enhanced");
  const juce::ComboBox* combo = box.getComboBox();

  box.setOptionEnabled(2, false);
  EXPECT_FALSE(combo->isItemEnabled(3));
  EXPECT_TRUE(combo->isItemEnabled(1));
  EXPECT_TRUE(combo->isItemEnabled(2));

  box.setOptionEnabled(2, true);
  EXPECT_TRUE(combo->isItemEnabled(3));
}
