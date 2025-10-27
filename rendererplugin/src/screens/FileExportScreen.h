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
#include <components/components.h>

#include "components/src/ExportValidation.h"
#include "components/src/SelectionBox.h"
#include "components/src/SliderButton.h"
#include "components/src/TimeFormatSegmentSelector.h"
#include "components/src/TitledLabel.h"
#include "components/src/TitledTextBox.h"
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/FileExport.h"

// Time format options for export start/end times
enum class TimeFormat {
  HoursMinutesSeconds,  // 00:00:00 (HH:MM:SS)
  BarsBeats,            // 1.1.000 (Bars.Beats.Ticks)
  Timecode              // 00:00:00:00 (HH:MM:SS:FF)
};

class FileExportScreen : public juce::Component,
                         public juce::ValueTree::Listener {
 public:
  FileExportScreen(MainEditor& editor, RepositoryCollection repos);

  ~FileExportScreen();

  void refreshComponents();

  void refreshFileExportComponents();

  void paint(juce::Graphics& g);

  void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) override;
  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;
  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;

 private:
  void configureCustomCodecParameter(AudioCodec format);

  // Time format conversion methods
  juce::String timeToString(int timeInSeconds, TimeFormat format);
  int stringToTime(juce::String val, TimeFormat format);

  // Helper methods for timing info and format availability
  void updateTimingInfoFromHost();
  bool isTimeFormatAvailable(TimeFormat format);

  bool validFileExportConfig(const FileExport& config);

  MainEditor& editor_;
  FileExportRepository* repository_;
  AudioElementRepository* aeRepository_;
  MixPresentationRepository* mpRepository_;
  FilePlaybackRepository* filePlaybackRepository_;

  /*
   * ==============================
   * Component Declarations
   *===============================
   */

  HeaderBar headerBar_;

  // Left side elements - Time inputs (use TitledTextBox like other fields)
  juce::Label exportParametersLabel_;
  TitledTextBox startTimer_;
  juce::Label startTimerErrorLabel_;
  TimeFormatSegmentSelector startFormatSegments_;
  juce::Label startTimeFormatLabel_;

  TitledTextBox endTimer_;
  juce::Label endTimerErrorLabel_;
  TimeFormatSegmentSelector endFormatSegments_;
  juce::Label endTimeFormatLabel_;

  // Left side elements - Export format selectors
  SelectionBox formatSelector_;
  SelectionBox codecSelector_;
  SelectionBox bitDepthSelector_;
  TitledLabel sampleRate_;
  TitledTextBox customCodecParameter_;
  juce::Label customCodecParameterErrorLabel_;
  TitledLabel mixPresentations_;
  TitledLabel audioElements_;

  // Time format state
  TimeFormat startTimeFormat_;
  TimeFormat endTimeFormat_;

  // Cached timing information from host
  juce::Optional<double> cachedBpm_;
  juce::Optional<juce::AudioPlayHead::TimeSignature> cachedTimeSignature_;
  juce::Optional<juce::AudioPlayHead::FrameRate> cachedFrameRate_;

  // Right side elements
  juce::Label exportAudioLabel_;
  SliderButton enableFileExport_;
  TitledTextBox exportPath_;
  juce::ImageButton browseButton_;
  juce::ToggleButton exportAudioElementsToggle_;
  juce::Label exportAudioElementsLabel_;
  juce::Label muxVidoeLabel_;
  SliderButton muxVideoToggle_;
  TitledTextBox exportVideoFolder_;
  juce::ImageButton browseVideoButton_;
  TitledTextBox videoSource_;
  juce::ImageButton browseVideoSourceButton_;

  // Player elements
  ExportValidationComponent exportValidation_;

  // File selection elements
  juce::FileChooser audioOutputSelect_;
  juce::FileChooser muxVideoSourceSelect_;
  juce::FileChooser muxVideoOutputSelect_;

  // Manual export button -- To be removed later
  juce::TextButton exportButton_;
  juce::Label warningLabel_;
};