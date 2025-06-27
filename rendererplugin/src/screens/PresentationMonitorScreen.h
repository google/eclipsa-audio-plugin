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
#include "EditPresentationScreen.h"
#include "ElementRoutingScreen.h"
#include "components/src/ImageTextButton.h"  // Include the ImageTextButton header
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/MixPresentationSoloMuteRepository.h"
#include "data_structures/src/MixPresentation.h"
#include "mix_tabs/MixPresentationViewPort.h"

class CustomTabbedComponent : public juce::TabbedComponent {
 public:
  CustomTabbedComponent();

  ~CustomTabbedComponent() override;

  void currentTabChanged(int newCurrentTabIndex,
                         const juce::String& newCurrentTabName) override;

  void setTabRestorationMode(bool isRestoring) {
    isRestoringTabs_ = isRestoring;
  }

 private:
  bool isRestoringTabs_ = false;
};

class PresentationMonitorScreen : public juce::Component,
                                  public juce::ValueTree::Listener {
  ImageTextButton editPresentationButton;
  ImageTextButton changeRoutingButton;

  juce::TextButton backButton;

  // Define an instance of the routing screen here to transition to
  ElementRoutingScreen elementRoutingScreen_;
  EditPresentationScreen editPresentationScreen_;

 public:
  PresentationMonitorScreen(
      MainEditor& editor, AudioElementRepository* ae_repository,
      MultibaseAudioElementSpatialLayoutRepository*
          audioElementSpatialLayout_repository,
      MixPresentationRepository* mixPresentationRepository,
      MixPresentationSoloMuteRepository* mixPresentationSoloMuteRepository,
      MultiChannelRepository* multiChannelRepository,
      ActiveMixRepository* activeMixRepo,
      ChannelMonitorProcessor* channelMonitorProcessor,
      FileExportRepository* fileExportRepository, int totalChannelCount);

  ~PresentationMonitorScreen();

  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;

  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int index) override;

  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;

  void paint(juce::Graphics& g);

 private:
  void updateTabButtonBounds(
      const juce::Rectangle<int>& audioElementMonitoringBounds);

  void updateMixPresentations();

  void updatePresentationTabs();
  int initialTabIndex_;
  juce::Rectangle<int> presentationTabBounds_ =
      juce::Rectangle<int>(0, 0, 0, 0);
  MixPresentationRepository* mixPresentationRepository_;
  MixPresentationSoloMuteRepository* mixPresentationSoloMuteRepository_;
  AudioElementRepository* audioElementRepository_;
  ActiveMixRepository* activeMixRepository_;
  MultiChannelRepository* multiChannelRepository_;

  juce::OwnedArray<MixPresentation> mixPresentationArray_;
  int numMixes_ = 0;
  std::unique_ptr<CustomTabbedComponent> presentationTabs_;
  ChannelMonitorProcessor* channelMonitorProcessor_;
};