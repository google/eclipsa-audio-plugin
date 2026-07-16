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

#include "DAWCompatibilityChecker.h"
#include "WarningBannerBase.h"
#include "data_repository/implementation/RoomSetupRepository.h"

class DAWWarningBanner : public WarningBannerBase {
 public:
  explicit DAWWarningBanner(RoomSetupRepository* roomSetupRepo)
      : m_roomSetupRepository_(roomSetupRepo),
        m_isDAWSupported(true),
        m_dismissedInRepo(false) {
    juce::PluginHostType hostType;
    m_hostName = hostType.getHostDescription();
    m_isDAWSupported = DAWCompatibilityChecker::isDAWSupported();

    m_dismissedInRepo = false;
    if (m_roomSetupRepository_) {
      m_dismissedInRepo =
          m_roomSetupRepository_->get().getDawWarningDismissed();
    }

    bool shouldBeVisible = !m_isDAWSupported && !m_dismissedInRepo;
    setVisible(shouldBeVisible);
  }

  void refreshVisibility() {
    m_isDAWSupported = DAWCompatibilityChecker::isDAWSupported();

    if (m_roomSetupRepository_) {
      m_dismissedInRepo =
          m_roomSetupRepository_->get().getDawWarningDismissed();
    }
    setVisible(!m_isDAWSupported && !m_dismissedInRepo);
  }

 protected:
  juce::String getMessageText() const override {
    return m_hostName +
           " support isn't officially tested yet—functionality may vary.";
  }

  void onDismiss() override {
    if (m_roomSetupRepository_) {
      RoomSetup currentRoomSetup = m_roomSetupRepository_->get();
      currentRoomSetup.setDawWarningDismissed(true);
      m_roomSetupRepository_->update(currentRoomSetup);
      m_dismissedInRepo = true;
    }
  }

 private:
  RoomSetupRepository* m_roomSetupRepository_;
  juce::String m_hostName;
  bool m_isDAWSupported = true;
  bool m_dismissedInRepo = false;
};
