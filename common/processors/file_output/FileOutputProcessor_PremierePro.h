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

#include "processors/file_output/FileOutputProcessor.h"

//==============================================================================
class PremiereProFileOutputProcessor final : public FileOutputProcessor {
 public:
  //==============================================================================
  PremiereProFileOutputProcessor(
      FileExportRepository& fileExportRepository,
      AudioElementRepository& audioElementRepository,
      MixPresentationRepository& mixPresentationRepository,
      MixPresentationLoudnessRepository& mixPresentationLoudnessRepository);
  ~PremiereProFileOutputProcessor() override;

  //==============================================================================
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void setNonRealtime(bool isNonRealtime) noexcept override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  void releaseResources() override;

 private:
  bool exportCompleted_;
  int estimatedSamplesToProcess_;
  int processedSamples_;
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PremiereProFileOutputProcessor)
};
