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

#include "../RendererProcessor.h"
#include "components/src/SelectionBox.h"
#include "components/src/SliderButton.h"
#include "components/src/TitledLabel.h"
#include "components/src/TitledTextBox.h"
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/FileExport.h"

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
  juce::String timeToString(int timeInMs);
  int stringToTime(juce::String val);

  bool validFileExportConfig(const FileExport& config);

  FileExportRepository* repository_;
  AudioElementRepository* aeRepository_;
  MixPresentationRepository* mpRepository_;

  /*
   * ==============================
   * Component Declarations
   *===============================
   */

  HeaderBar headerBar_;

  // Left side elements
  TitledTextBox startTimer_;
  juce::Label startTimerErrorLabel_;
  TitledTextBox endTimer_;
  juce::Label endTimerErrorLabel_;
  SelectionBox formatSelector_;
  SelectionBox codecSelector_;
  SelectionBox bitDepthSelector_;
  TitledLabel sampleRate_;
  TitledTextBox customCodecParameter_;
  juce::Label customCodecParameterErrorLabel_;
  TitledLabel mixPresentations_;
  TitledLabel audioElements_;

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

  // File selection elements
  juce::FileChooser audioOutputSelect_;
  juce::FileChooser muxVideoSourceSelect_;
  juce::FileChooser muxVideoOutputSelect_;

  // Manual export button -- To be removed later
  juce::TextButton exportButton_;
  juce::Label warningLabel_;
};