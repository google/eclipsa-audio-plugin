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

#include "components/src/SegmentedToggleButton.h"
// clang-format on

#include <gtest/gtest.h>

namespace {
// Simulates a user click on the button labeled `opt`.
void click(SegmentedToggleButton& row, const juce::String& opt) {
  for (int i = 0; i < row.getNumChildComponents(); ++i) {
    auto* button = dynamic_cast<juce::Button*>(row.getChildComponent(i));
    if (button != nullptr && button->getButtonText() == opt) {
      button->setToggleState(!button->getToggleState(), juce::sendNotification);
      return;
    }
  }
  FAIL() << "No button labeled " << opt;
}
}  // namespace

// A row built from a runtime list has one button per entry, in order.
TEST(SegmentedToggleButtonOptions, builds_buttons_from_runtime_list) {
  const juce::ScopedJuceInitialiser_GUI kJuce;
  const std::vector<juce::String> kOpts = {"Stereo", "5.1", "7.1.4"};
  SegmentedToggleButton row(kOpts, true);

  std::vector<juce::String> labels;
  for (const auto& [label, toggled] : row.getState()) {
    labels.push_back(label);
  }
  EXPECT_EQ(labels, kOpts);
}

// In single-select mode, clicking a button leaves only that button toggled and
// notifies once.
TEST(SegmentedToggleButtonOptions, single_select_click_toggles_only_one) {
  const juce::ScopedJuceInitialiser_GUI kJuce;
  SegmentedToggleButton row({"Stereo", "5.1", "7.1.4"}, true);
  row.toggleOn("5.1");
  int calls = 0;
  row.onChange([&calls] { ++calls; });

  click(row, "7.1.4");

  EXPECT_EQ(row.getToggled(), std::vector<juce::String>{"7.1.4"});
  EXPECT_EQ(calls, 1);
}

// In single-select mode, clicking the toggled button keeps it toggled and does
// not notify.
TEST(SegmentedToggleButtonOptions, single_select_click_on_toggled_keeps_it) {
  const juce::ScopedJuceInitialiser_GUI kJuce;
  SegmentedToggleButton row({"Stereo", "5.1", "7.1.4"}, true);
  row.toggleOn("5.1");
  int calls = 0;
  row.onChange([&calls] { ++calls; });

  click(row, "5.1");

  EXPECT_EQ(row.getToggled(), std::vector<juce::String>{"5.1"});
  EXPECT_EQ(calls, 0);
}

// The buttons tile the row edge to edge, so a width the count does not divide
// leaves no empty strip on the right.
TEST(SegmentedToggleButtonOptions, buttons_fill_full_width) {
  const juce::ScopedJuceInitialiser_GUI kJuce;
  SegmentedToggleButton row({"A", "B", "C", "D"}, true);
  row.setBounds(0, 0, 103, 20);
  juce::Image image(juce::Image::ARGB, 103, 20, true);
  juce::Graphics g(image);
  row.paint(g);

  int expectedLeft = 1;
  for (int i = 0; i < row.getNumChildComponents(); ++i) {
    const juce::Rectangle<int> kBounds = row.getChildComponent(i)->getBounds();
    EXPECT_EQ(kBounds.getX(), expectedLeft) << "button " << i;
    expectedLeft = kBounds.getRight();
  }
  EXPECT_EQ(expectedLeft, 102);
}
