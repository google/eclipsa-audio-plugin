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

#include "SegmentedToggleButton.h"

class TimeFormatSegmentSelector : public juce::Component {
 public:
  enum Format { HoursMinutesSeconds = 0, BarsBeats = 1, Timecode = 2 };

  static constexpr int kNumFormats = 3;

  TimeFormatSegmentSelector()
      : segments_({formatNames_[0], formatNames_[1], formatNames_[2]}, true) {
    addAndMakeVisible(segments_);
    segments_.onChange([this] { handleSegmentChange(); });
    {
      juce::ScopedValueSetter<bool> guard(updating_, true);
      segments_.setOption("H:M:S", true);
    }
  }

  void resized() override { segments_.setBounds(getLocalBounds()); }

  void paint(juce::Graphics& g) override { juce::ignoreUnused(g); }

  void setFormatEnabled(Format f, bool enabled) {
    if (f < HoursMinutesSeconds || f > Timecode) return;
    const int formatIndex = static_cast<int>(f);
    enabled_[formatIndex] = enabled;
    {
      juce::ScopedValueSetter<bool> guard(updating_, true);
      segments_.setEnabledForOption(formatNames_[formatIndex], enabled);
    }
    if (!enabled && selected_ == formatIndex) {
      for (int i = 0; i < kNumFormats; ++i) {
        if (enabled_[i]) {
          setSelectedFormat(static_cast<Format>(i));
          break;
        }
      }
    }
  }

  void setSelectedFormat(Format f) {
    if (f < HoursMinutesSeconds || f > Timecode) return;
    selected_ = static_cast<int>(f);
    juce::ScopedValueSetter<bool> guard(updating_, true);
    segments_.setOption(formatNames_[selected_], true);
  }

  int getSelectedIndex() const { return selected_; }

  std::function<void(int)> onChange;

 private:
  void handleSegmentChange() {
    if (updating_) return;
    auto state = segments_.getState();
    for (int i = 0; i < state.size(); ++i) {
      if (state[i].second) {
        selected_ = i;
        if (onChange) onChange(selected_);
        return;
      }
    }
  }

  const juce::String formatNames_[kNumFormats] = {"H:M:S", "Bars", "TC"};
  SegmentedToggleButton segments_;
  bool enabled_[kNumFormats] = {true, true, true};
  int selected_{0};
  bool updating_{false};
};
