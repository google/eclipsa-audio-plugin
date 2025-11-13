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
#include "FileExportScreen.h"
#include "components/src/ImageTextButton.h"

class RoomMonitoringScreen : public juce::Component,
                             public juce::ValueTree::Listener,
                             public juce::Timer {
 public:
  RoomMonitoringScreen(RepositoryCollection repos, SpeakerMonitorData& data,
                       MainEditor& editor);
  ~RoomMonitoringScreen();

  void paint(juce::Graphics& g);

 private:
  void initializeSpeakerSetup();
  void updateSpeakerSetup();
  void updateRoomOpts();
  void toggleLabelsDrawable();
  void updateRoomView();
  void updateActiveIDs(RepositoryCollection& repos,
                       std::unique_ptr<PerspectiveRoomView>& prv);
  void updateActiveTrackData();
  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;
  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;
  void timerCallback() override;

  RepositoryCollection repos_;
  SpeakerMonitorData& monitorData_;
  std::unordered_set<juce::Uuid> activeAudioElementIDs_;
  // Components.
  SelectionBox speakerSetup_;
  ImageTextButton exportButton;
  FileExportScreen fileExportScreen_;
  std::unique_ptr<PerspectiveRoomView> roomView_;
  SegmentedToggleButton selRoomOpts_, selRoomView_;
};