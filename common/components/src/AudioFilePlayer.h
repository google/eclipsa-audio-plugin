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

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

#include "components/icons/svg/SvgIconComponent.h"
#include "components/src/ColouredSlider.h"
#include "components/src/RoundImageButton.h"
#include "data_repository/implementation/FilePlaybackRepository.h"
#include "data_structures/src/FilePlaybackProcessorData.h"

class AudioFilePlayer : public juce::Component, private juce::Timer {
 public:
  AudioFilePlayer(FilePlaybackRepository& filePlaybackRepo,
                  FilePlaybackProcessorData& fpbData);
  ~AudioFilePlayer() override;

  void paint(juce::Graphics& g) override;
  void resized() override;
  void update();
  void timerCallback() override;

 private:
  class Spinner;

  void updateComponentVisibility();

  // Components
  RoundImageButton playButton_, pauseButton_, stopButton_;
  ColouredSlider volumeSlider_{ColouredSlider::Circle};
  juce::Label timeLabel_;
  ColouredSlider playbackSlider_{ColouredSlider::FlatBar};
  SvgIconComponent volumeIcon_;
  std::unique_ptr<Spinner> spinner_;
  juce::Label fileSelectLabel_{"fileSelectLabel",
                               "Invalid IAMF file selected for playback"};
  // State
  FilePlaybackRepository& fpbr_;
  FilePlaybackProcessorData& fpbData_;
};