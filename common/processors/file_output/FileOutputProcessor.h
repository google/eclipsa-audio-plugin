/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License,
 * Version 2.0 (the "License");
 * you may not use this file except in
 * compliance with the License.
 * You may obtain a copy of the License at
 *
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by
 * applicable law or agreed to in writing, software
 * distributed under the
 * License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the
 * specific language governing permissions and
 * limitations under the
 * License.
 */

#pragma once
#include <data_repository/data_repository.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <memory>

#include "../processor_base/ProcessorBase.h"
#include "AudioElementFileWriter.h"
#include "data_repository/implementation/MixPresentationLoudnessRepository.h"
#include "iamf_export_utils/IAMFFileWriter.h"

//==============================================================================
class FileOutputProcessor : public ProcessorBase {
 public:
  //==============================================================================
  FileOutputProcessor(
      FileExportRepository& fileExportRepository,
      AudioElementRepository& audioElementRepository,
      MixPresentationRepository& mixPresentationRepository,
      MixPresentationLoudnessRepository& mixPresentationLoudnessRepository);
  ~FileOutputProcessor() override;

  //==============================================================================
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void setNonRealtime(bool isNonRealtime) noexcept override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  //==============================================================================
  juce::AudioProcessorEditor* createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }

  //==============================================================================
  const juce::String getName() const override;

 protected:
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout params;
    return params;
  }

  juce::AudioParameterFloatAttributes initParameterAttributes(
      int decimalPlaces, juce::String&& label) const {
    return juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction([decimalPlaces](float value, int unused) {
          juce::ignoreUnused(unused);
          return juce::String(value, decimalPlaces, false);
        })
        .withLabel(label);
  }

  void initializeFileExport(FileExport& config);

  void closeFileExport(FileExport& config);

  bool shouldBufferBeWritten(const juce::AudioBuffer<float>& buffer);

  bool performingRender_;  // True if we are rendering in offline mode
  FileExportRepository& fileExportRepository_;
  AudioElementRepository& audioElementRepository_;
  MixPresentationRepository& mixPresentationRepository_;
  MixPresentationLoudnessRepository& mixPresentationLoudnessRepository_;
  std::vector<std::unique_ptr<AudioElementFileWriter>> iamfWavFileWriters_;
  int numSamples_;
  long sampleRate_;
  int startTime_;
  int endTime_;
  long sampleTally_;
  std::unique_ptr<IAMFFileWriter> iamfFileWriter_;
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileOutputProcessor)
};
