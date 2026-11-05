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
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_repository/implementation/RoomSetupRepository.h"
#include "data_structures/src/MixPresentation.h"

// Warns under the YouTube export profile while the session has more than one
// mix presentation. Dismissal lasts until the condition clears and recurs; it
// is not persisted.
class MixPresentationWarningBanner : public WarningBannerBase,
                                     private juce::ValueTree::Listener {
 public:
  MixPresentationWarningBanner(
      MixPresentationRepository* mixPresentationRepository,
      RoomSetupRepository* roomSetupRepository)
      : mixPresentationRepository_(mixPresentationRepository),
        roomSetupRepository_(roomSetupRepository),
        profileListener_(roomSetupRepository, [weakSelf = weakSelf_]() {
          evaluateLater(weakSelf);
        }) {
    if (mixPresentationRepository_ != nullptr) {
      mixPresentationRepository_->registerListener(this);
    }
    wasWarning_ = currentCondition();
    setVisible(wasWarning_);
  }

  ~MixPresentationWarningBanner() override {
    if (mixPresentationRepository_ != nullptr) {
      mixPresentationRepository_->deregisterListener(this);
    }
  }

  static bool shouldWarn(ExportProfile profile, int numMixes) {
    return profile == ExportProfile::kYouTube && numMixes > 1;
  }

  static juce::String messageText() {
    return "YouTube only plays back the first mix presentation. Remove "
           "additional mix presentations if unneeded.";
  }

  std::function<void()> onVisibilityChanged;

 protected:
  juce::String getMessageText() const override { return messageText(); }

 private:
  static void evaluateLater(
      juce::Component::SafePointer<MixPresentationWarningBanner> weakSelf) {
    juce::MessageManager::callAsync([weakSelf] {
      if (weakSelf) weakSelf->evaluate();
    });
  }

  bool currentCondition() const {
    const ExportProfile profile =
        roomSetupRepository_ != nullptr
            ? roomSetupRepository_->get().getExportProfile()
            : ExportProfile::kAdvanced;
    const int numMixes = mixPresentationRepository_ != nullptr
                             ? mixPresentationRepository_->getItemCount()
                             : 0;
    return shouldWarn(profile, numMixes);
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

  void valueTreeChildAdded(juce::ValueTree&, juce::ValueTree& child) override {
    if (child.getType() == MixPresentation::kTreeType) evaluateLater(weakSelf_);
  }

  void valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree& child,
                             int) override {
    if (child.getType() == MixPresentation::kTreeType) evaluateLater(weakSelf_);
  }

  void valueTreeRedirected(juce::ValueTree&) override {
    evaluateLater(weakSelf_);
  }

  MixPresentationRepository* mixPresentationRepository_;
  RoomSetupRepository* roomSetupRepository_;
  bool wasWarning_ = false;
  const juce::Component::SafePointer<MixPresentationWarningBanner> weakSelf_{
      this};
  ExportProfileListener profileListener_;
};
