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
#include <data_repository/data_repository.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <memory>

#include "../processor_base/ProcessorBase.h"
#include "AudioElementFileWriter.h"
#include "FileWriter.h"
#include "data_repository/implementation/RoomSetupRepository.h"
#include "data_structures/src/RoomSetup.h"
#include "user_metadata.pb.h"

//==============================================================================
class WavFileOutputProcessor final : public ProcessorBase,
                                     public juce::ValueTree::Listener {
 public:
  //==============================================================================
  WavFileOutputProcessor(FileExportRepository& fileExportRepository,
                         RoomSetupRepository& roomSetupRepository);
  ~WavFileOutputProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void setNonRealtime(bool isNonRealtime) noexcept override;

  //==============================================================================
  void checkManualExportStartStop();
  void valueTreeRedirected(juce::ValueTree& treeWhichHasBeenChanged) override;
  void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;
  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;
  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;

  //==============================================================================
  const juce::String getName() { return "WaveFileOutput"; }

 private:
  bool performingRender_;  // True if we are rendering in offline mode
  FileExportRepository& fileExportRepository_;
  RoomSetupRepository& roomSetupRepository_;
  FileWriter* fileWriter_;
  int numSamples_;
  int sampleRate_;
  int startTime_;
  int endTime_;
  juce::SpinLock lock_;
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavFileOutputProcessor)
};
