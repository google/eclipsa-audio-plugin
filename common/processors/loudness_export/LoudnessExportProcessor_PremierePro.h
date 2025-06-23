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

#include "processors/loudness_export/LoudnessExportProcessor.h"

class PremiereProLoudnessExportProcessor : public LoudnessExportProcessor {
 public:
  void setNonRealtime(bool isNonRealtime) noexcept override;

  PremiereProLoudnessExportProcessor(
      FileExportRepository& fileExportRepo,
      MixPresentationRepository& mixPresentationRepo,
      MixPresentationLoudnessRepository& loudnessRepo,
      AudioElementRepository& audioElementRepo);

  ~PremiereProLoudnessExportProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  void processBlock(juce::AudioBuffer<float>& buffer,
                    juce::MidiBuffer& midiMessages) override;

  void releaseResources() override;

 private:
  bool exportCompleted_;
  int estimatedSamplesToProcess_;
  int processedSamples_;
};