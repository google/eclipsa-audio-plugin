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
#include <data_structures/src/LoudnessExportData.h>
#include <processors/processor_base/ProcessorBase.h>
#include <processors/render/RenderProcessor.h>

#include <memory>
#include <utility>
#include <vector>

#include "../mix_monitoring/loudness_standards/MeasureEBU128.h"
#include "data_structures/src/AudioElement.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class MixPresentationLoudnessExportContainer {
 public:
  MixPresentationLoudnessExportContainer(
      const juce::Uuid& mixPresId, const float& mixPresGain,
      const int& sampleRate, const int& samplesPerBlock,
      const Speakers::AudioElementSpeakerLayout& largestLayout,
      const std::vector<AudioElement>& audioElements);

  ~MixPresentationLoudnessExportContainer();

  // disable copy semantics (copy-ctor & copy-assignment) to prevent
  // accidental shallow copies and double-free bugs
  MixPresentationLoudnessExportContainer(
      const MixPresentationLoudnessExportContainer&) = delete;
  MixPresentationLoudnessExportContainer& operator=(
      const MixPresentationLoudnessExportContainer&) = delete;

  MixPresentationLoudnessExportContainer(
      MixPresentationLoudnessExportContainer&&) noexcept = default;
  MixPresentationLoudnessExportContainer& operator=(
      MixPresentationLoudnessExportContainer&&) noexcept = default;

  void process(juce::AudioBuffer<float>& buffer);

  // internal copy of the mixPres ID
  const juce::Uuid mixPresentationId;

  // linear gain for the mix presentation
  const float mixPresentationGain;

  // internal copy of the largest layout
  const Speakers::AudioElementSpeakerLayout largestLayout;

  // internal copies of samples rates and samples per block
  const int kSampleRate;
  const int kSamplesPerBlock;

  // for each audio element in the mix presentation
  // there will be two renderers
  // the first element is for stereo
  // the second element is for the largest layout greater than stereo
  // if the largest layout is stereo, the second element is null
  std::vector<std::pair<std::unique_ptr<AudioElementRenderer>,
                        std::unique_ptr<AudioElementRenderer>>>
      audioElementRenderers;

  // stores the loudness data calculated in real time
  std::unique_ptr<LoudnessExportData> loudnessExportData;

  // the first element is for stereo
  // the second element is for the largest layout greater than stereo
  // if the largest layout is stereo, the second element is null
  std::pair<std::unique_ptr<MeasureEBU128>, std::unique_ptr<MeasureEBU128>>
      loudnessImpls;

  // the first element is for mix audio into a stereo playback layout
  // the second element is for mix audio into the largest layout
  // greater than stereo
  // if the largest layout is stereo, the second element is null
  std::pair<juce::AudioBuffer<float>, juce::AudioBuffer<float>> mixPresBuffers;

 private:
  std::vector<std::pair<std::unique_ptr<AudioElementRenderer>,
                        std::unique_ptr<AudioElementRenderer>>>
  createRenderers(const std::vector<AudioElement>& audioElements);

  std::pair<std::unique_ptr<MeasureEBU128>, std::unique_ptr<MeasureEBU128>>
  createLoudnessImpls();

  std::pair<juce::AudioBuffer<float>, juce::AudioBuffer<float>>
  createMixPresBuffers();

  void renderAudioElement(AudioElementRenderer& renderer,
                          juce::AudioBuffer<float>& buffer,
                          juce::AudioBuffer<float>& mixPresBuffer);

  void measureStereoLoudness(const juce::AudioBuffer<float>& buffer);

  void measureLayoutLoudness(const juce::AudioBuffer<float>& buffer);

  const juce::AudioBuffer<float> getRenderedBuffer(
      juce::AudioBuffer<float>& busBuff,
      const Speakers::AudioElementSpeakerLayout& layout);
};