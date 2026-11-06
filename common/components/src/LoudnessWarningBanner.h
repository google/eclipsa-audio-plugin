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

#include <functional>

#include "WarningBannerBase.h"
#include "data_repository/implementation/ExportProfileListener.h"
#include "data_repository/implementation/RoomSetupRepository.h"
#include "data_structures/src/SpeakerMonitorData.h"
#include "loudness_meter/LoudnessStats.h"

// Warns under the YouTube export profile while the displayed integrated
// loudness is above YouTube's -14 LUFS normalization target. Dismissal lasts
// until the condition clears and recurs; it is not persisted.
class LoudnessWarningBanner : public WarningBannerBase, private juce::Timer {
 public:
  LoudnessWarningBanner(SpeakerMonitorData& data,
                        RoomSetupRepository* roomSetupRepository)
      : data_(data),
        roomSetupRepository_(roomSetupRepository),
        profileListener_(roomSetupRepository, [weakSelf = weakSelf_]() {
          juce::MessageManager::callAsync([weakSelf] {
            if (weakSelf) weakSelf->evaluate();
          });
        }) {
    wasWarning_ = currentCondition();
    setVisible(wasWarning_);
    startTimerHz(1);
  }

  ~LoudnessWarningBanner() override { stopTimer(); }

  static constexpr double kThresholdLufs = -14.0;

  static bool shouldWarn(ExportProfile profile, float integrated, bool reset) {
    if (profile != ExportProfile::kYouTube || reset ||
        !LoudnessStats::isValidLoudness(integrated)) {
      return false;
    }
    // Compare the one-decimal value the loudness stats display.
    return juce::String(integrated, 1).getDoubleValue() > kThresholdLufs;
  }

  static juce::String messageText() {
    return "Audio louder than -14 LUFS will be normalized by YouTube. To "
           "ensure audio sounds correct, reduce integrated loudness to below "
           "-14 LUFS.";
  }

  std::function<void()> onVisibilityChanged;

 protected:
  juce::String getMessageText() const override { return messageText(); }

 private:
  bool currentCondition() const {
    const ExportProfile profile =
        roomSetupRepository_ != nullptr
            ? roomSetupRepository_->get().getExportProfile()
            : ExportProfile::kAdvanced;
    MeasureEBU128::LoudnessStats stats{};
    data_.loudnessEBU128.read(stats);
    return shouldWarn(profile, stats.loudnessIntegrated,
                      data_.resetStats.load());
  }

  void evaluate() {
    const bool isWarning = currentCondition();
    const bool visible = nextVisibility(wasWarning_, isWarning, isVisible());
    wasWarning_ = isWarning;
    if (visible != isVisible()) {
      setVisible(visible);
      if (onVisibilityChanged) onVisibilityChanged();
    }
  }

  void timerCallback() override { evaluate(); }

  SpeakerMonitorData& data_;
  RoomSetupRepository* roomSetupRepository_;
  bool wasWarning_ = false;
  const juce::Component::SafePointer<LoudnessWarningBanner> weakSelf_{this};
  ExportProfileListener profileListener_;
};
