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

#include "components/src/TextEditorControlledDial.h"
// clang-format on

#include <gtest/gtest.h>

#include <vector>

namespace {

// Drives a dial through its child components, as a user would, and records
// every value it hands to its value-updated callback.
class DialHarness {
 public:
  explicit DialHarness(int currValue)
      : dial_("X", 0, currValue, -50, 50, makeChevron(), makeChevron()) {
    dial_.setValueUpdatedCallback(
        [this](int value) { written_.push_back(value); });
    for (juce::Component* child : dial_.getChildren()) {
      if (auto* button = dynamic_cast<juce::ImageButton*>(child)) {
        buttons_.push_back(button);
      }
      // The box adds its editor as a child only on layout, so reach it here.
      if (auto* box = dynamic_cast<TitledTextBox*>(child)) {
        textEditor_ = const_cast<juce::TextEditor*>(box->getTextEditor());
      }
    }
  }

  void pressLeft() { buttons_.at(0)->onClick(); }
  void pressRight() { buttons_.at(1)->onClick(); }

  void typeAndReturn(const juce::String& text) {
    textEditor_->setText(text, false);
    textEditor_->onReturnKey();
  }

  TextEditorControlledDial& dial() { return dial_; }
  const std::vector<int>& written() const { return written_; }
  bool hasControls() const {
    return buttons_.size() == 2 && textEditor_ != nullptr;
  }

 private:
  static juce::Image makeChevron() {
    return juce::Image(juce::Image::ARGB, 4, 4, true);
  }

  const juce::ScopedJuceInitialiser_GUI kJuce_;
  TextEditorControlledDial dial_;
  std::vector<juce::ImageButton*> buttons_;
  juce::TextEditor* textEditor_ = nullptr;
  std::vector<int> written_;
};

}  // namespace

// The chevrons step to the neighbouring multiples of 5 on the correct side for
// a negative value, not by C++'s negative remainder.
TEST(test_text_editor_controlled_dial, chevronsStepToNeighbourMultiplesBelow0) {
  DialHarness left(-13);
  ASSERT_TRUE(left.hasControls());
  left.pressLeft();
  EXPECT_EQ(left.written(), std::vector<int>({-15}));

  DialHarness right(-13);
  right.pressRight();
  EXPECT_EQ(right.written(), std::vector<int>({-10}));
}

// From a negative multiple of 5, each chevron moves one full step.
TEST(test_text_editor_controlled_dial, chevronsStepFromNegativeMultipleOf5) {
  DialHarness dial(-15);
  dial.pressLeft();
  dial.pressRight();
  EXPECT_EQ(dial.written(), std::vector<int>({-20, -15}));
}

// A chevron press is written when the parameter moved elsewhere since the dial
// last wrote, even if the target equals the dial's own last write.
TEST(test_text_editor_controlled_dial, chevronWritesAfterExternalMove) {
  DialHarness dial(10);
  ASSERT_TRUE(dial.hasControls());
  dial.dial().setValue(13.f);
  dial.pressLeft();
  EXPECT_EQ(dial.written(), std::vector<int>({10}));
}

// A typed value is written when the parameter moved elsewhere since the dial
// last wrote, even if it equals the dial's own last write.
TEST(test_text_editor_controlled_dial, typedValueWritesAfterExternalMove) {
  DialHarness dial(10);
  ASSERT_TRUE(dial.hasControls());
  dial.dial().setValue(13.f);
  dial.typeAndReturn("10");
  EXPECT_EQ(dial.written(), std::vector<int>({10}));
}

// Typing the value the parameter already holds writes nothing.
TEST(test_text_editor_controlled_dial, typedCurrentValueWritesNothing) {
  DialHarness dial(10);
  ASSERT_TRUE(dial.hasControls());
  dial.dial().setValue(13.f);
  dial.typeAndReturn("13");
  EXPECT_TRUE(dial.written().empty());
}
