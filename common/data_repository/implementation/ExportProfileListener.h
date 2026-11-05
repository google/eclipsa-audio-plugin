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

#include <functional>

#include "RoomSetupRepository.h"

// Calls onChange only when the stored export profile changes, so speaker
// layout, room view and draw toggle edits on the same tree are ignored.
class ExportProfileListener : public juce::ValueTree::Listener {
 public:
  ExportProfileListener(RoomSetupRepository* repo,
                        std::function<void()> onChange)
      : repo_(repo), onChange_(std::move(onChange)) {
    if (repo_ != nullptr) {
      repo_->registerListener(this);
    }
  }

  ~ExportProfileListener() override {
    if (repo_ != nullptr) {
      repo_->deregisterListener(this);
    }
  }

  ExportProfileListener(const ExportProfileListener&) = delete;
  ExportProfileListener& operator=(const ExportProfileListener&) = delete;

  static bool isExportProfileChange(const juce::ValueTree& tree,
                                    const juce::Identifier& property) {
    return tree.getType() == RoomSetup::kTreeType &&
           property == RoomSetup::kExportProfile;
  }

  void valueTreePropertyChanged(juce::ValueTree& tree,
                                const juce::Identifier& property) override {
    if (isExportProfileChange(tree, property) && onChange_) {
      onChange_();
    }
  }

 private:
  RoomSetupRepository* repo_;
  std::function<void()> onChange_;
};
