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
#include <unordered_map>
#include <utility>
#include <vector>

#include "../mix_monitoring/loudness_standards/MeasureEBU128.h"
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/MixPresentationLoudnessRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/MixPresentationLoudness.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

struct PremiereProExportContainer {
  PremiereProExportContainer(
      const juce::Uuid& mixPresId, const float& mixPresGain,
      const int& sampleRate, const int& samplesPerBlock,
      const Speakers::AudioElementSpeakerLayout& largestLayout,
      const std::vector<AudioElement>& audioElements)
      : mixPresentationId(mixPresId),
        mixPresentationGain(mixPresGain),
        largestLayout(largestLayout),
        kSampleRate(sampleRate),
        kSamplesPerBlock(samplesPerBlock),
        audioElementRenderers(createRenderers(audioElements)),
        loudnessExportData(std::make_unique<LoudnessExportData>()),
        loudnessImpls(createLoudnessImpls()),
        mixPresBuffers(createMixPresBuffers()) {}

  ~PremiereProExportContainer() {}

  // disable copy semantics (copy-ctor & copy-assignment) to prevent
  // accidental shallow copies and double-free bugs
  PremiereProExportContainer(const PremiereProExportContainer&) = delete;
  PremiereProExportContainer& operator=(const PremiereProExportContainer&) =
      delete;

  PremiereProExportContainer(PremiereProExportContainer&&) noexcept = default;
  PremiereProExportContainer& operator=(PremiereProExportContainer&&) noexcept =
      default;

  void process(juce::AudioBuffer<float>& buffer) {
    // clear buffers before mixing audio
    mixPresBuffers.first.clear();
    mixPresBuffers.second.clear();
    for (auto& rendererPair : audioElementRenderers) {
      renderAudioElement(*rendererPair.first, buffer, mixPresBuffers.first);
      if (rendererPair.second != nullptr &&
          mixPresBuffers.second.getNumChannels() >
              Speakers::kStereo.getNumChannels()) {
        renderAudioElement(*rendererPair.second, buffer, mixPresBuffers.second);
      }
    }

    measureStereoLoudness(
        getRenderedBuffer(mixPresBuffers.first, Speakers::kStereo));

    if (largestLayout != Speakers::kStereo && loudnessImpls.second != nullptr &&
        mixPresBuffers.second.getNumChannels() >
            Speakers::kStereo.getNumChannels()) {
      measureLayoutLoudness(
          getRenderedBuffer(mixPresBuffers.second, largestLayout));
    }
  }

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
  createRenderers(const std::vector<AudioElement>& audioElements) {
    std::vector<std::pair<std::unique_ptr<AudioElementRenderer>,
                          std::unique_ptr<AudioElementRenderer>>>
        rendererPairs;
    rendererPairs.reserve(audioElements.size());
    for (int j = 0; j < audioElements.size(); j++) {
      AudioElement audioElement = audioElements[j];

      if (largestLayout == Speakers::kStereo) {
        rendererPairs.emplace_back(std::make_pair(
            std::make_unique<AudioElementRenderer>(
                audioElement.getChannelConfig(), Speakers::kStereo,
                audioElement.getFirstChannel(), kSamplesPerBlock, kSampleRate),
            nullptr));
      } else {
        rendererPairs.emplace_back(std::make_pair(
            std::make_unique<AudioElementRenderer>(
                audioElement.getChannelConfig(), Speakers::kStereo,
                audioElement.getFirstChannel(), kSamplesPerBlock, kSampleRate),
            std::make_unique<AudioElementRenderer>(
                audioElement.getChannelConfig(), largestLayout,
                audioElement.getFirstChannel(), kSamplesPerBlock,
                kSampleRate)));
      }
    }
    return rendererPairs;
  }

  std::pair<std::unique_ptr<MeasureEBU128>, std::unique_ptr<MeasureEBU128>>
  createLoudnessImpls() {
    std::unique_ptr<MeasureEBU128> stereoImpl = std::make_unique<MeasureEBU128>(
        kSampleRate, Speakers::kStereo.getChannelSet());
    std::unique_ptr<MeasureEBU128> layoutImpl;
    if (largestLayout != Speakers::kStereo) {
      layoutImpl = std::make_unique<MeasureEBU128>(
          kSampleRate, largestLayout.getChannelSet());
    } else {
      layoutImpl = nullptr;
    }
    return {std::move(stereoImpl), std::move(layoutImpl)};
  }

  std::pair<juce::AudioBuffer<float>, juce::AudioBuffer<float>>
  createMixPresBuffers() {
    juce::AudioBuffer<float> stereoBuffer = juce::AudioBuffer<float>(
        Speakers::kStereo.getNumChannels(), kSamplesPerBlock);
    if (largestLayout == Speakers::kStereo) {
      return {stereoBuffer,
              juce::AudioBuffer<float>(Speakers::kMono.getNumChannels(), 1)};
    } else {
      return {stereoBuffer,
              juce::AudioBuffer<float>(largestLayout.getNumChannels(),
                                       kSamplesPerBlock)};
    }
  }

  void renderAudioElement(AudioElementRenderer& renderer,
                          juce::AudioBuffer<float>& buffer,
                          juce::AudioBuffer<float>& mixPresBuffer) {
    renderer.inputData.clear();
    renderer.outputData.clear();

    // Copy Audio Element substream data from the process block buffer to the
    // AudioElementRenderer's input buffer.
    for (int ch = 0; ch < renderer.inputData.getNumChannels(); ++ch) {
      renderer.inputData.copyFrom(ch, 0, buffer, renderer.firstChannel + ch, 0,
                                  buffer.getNumSamples());
    }

    if (renderer.renderer !=
        nullptr) {  // render the audio element to the stereo buffer
      renderer.renderer->render(renderer.inputData, renderer.outputData);
    }
    // If there is no valid renderer, copy the data from input to output.
    else {
      const int numSourceChannels = renderer.inputData.getNumChannels();
      for (int i = 0; i < numSourceChannels; ++i) {
        renderer.outputData.copyFrom(i, 0, renderer.inputData, i, 0,
                                     renderer.inputData.getNumSamples());
      }
    }

    // Mix rendered audio to the internal mix buffer.
    for (int k = 0; k < renderer.outputData.getNumChannels(); ++k) {
      mixPresBuffer.addFrom(k, 0, renderer.outputData, k, 0,
                            mixPresBuffer.getNumSamples(), mixPresentationGain);
    }
  }

  void measureStereoLoudness(const juce::AudioBuffer<float>& buffer) {
    jassert(buffer.getNumChannels() == Speakers::kStereo.getNumChannels());
    MeasureEBU128::LoudnessStats stats = loudnessImpls.first->measureLoudness(
        Speakers::kStereo.getChannelSet(), buffer);

    loudnessExportData->stereoEBU128.update(stats);
  }

  void measureLayoutLoudness(const juce::AudioBuffer<float>& buffer) {
    jassert(buffer.getNumChannels() == largestLayout.getNumChannels());
    MeasureEBU128::LoudnessStats stats = loudnessImpls.second->measureLoudness(
        largestLayout.getChannelSet(), buffer);

    loudnessExportData->layoutEBU128.update(stats);
  }

  const juce::AudioBuffer<float> getRenderedBuffer(
      juce::AudioBuffer<float>& busBuff,
      const Speakers::AudioElementSpeakerLayout& layout) {
    auto dataPtrs = busBuff.getArrayOfWritePointers();
    int numRdrCh = layout.getChannelSet().size();
    return juce::AudioBuffer<float>(dataPtrs, numRdrCh,
                                    busBuff.getNumSamples());
  }
};

class PremiereProLoudnessExportProcessor : public ProcessorBase,
                                           public juce::ValueTree::Listener {
 public:
  using EBU128Stats = MeasureEBU128::LoudnessStats;

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

  const std::vector<const PremiereProExportContainer*> getExportContainers()
      const {
    std::vector<const PremiereProExportContainer*> containers(
        exportContainers_.size());
    for (int i = 0; i < exportContainers_.size(); i++) {
      containers[i] = &exportContainers_[i];
    }
    return containers;
  }

 private:
  void copyExportContainerDataToRepo(
      const PremiereProExportContainer& exportContainer);

  void valueTreeChildAdded(juce::ValueTree& parentTree,
                           juce::ValueTree& childWhichHasBeenAdded) override;

  void valueTreeChildRemoved(juce::ValueTree& parentTree,
                             juce::ValueTree& childWhichHasBeenRemoved,
                             int indexFromWhichChildWasRemoved) override;

  void handleNewLayoutAdded(juce::ValueTree& parentTree,
                            juce::ValueTree& childWhichHasBeenAdded);

  Speakers::AudioElementSpeakerLayout getLargestLayoutFromTree(
      juce::ValueTree& mixPresentationAudioElementsTree);

  void intializeExportContainers();

  bool performingRender_;

  FileExportRepository& fileExportRepository_;
  MixPresentationRepository& mixPresentationRepository_;
  MixPresentationLoudnessRepository& loudnessRepo_;
  AudioElementRepository& audioElementRepository_;

  long sampleRate_;
  int currentSamplesPerBlock_;
  int sampleTally_;
  int startTime_;
  int endTime_;
  int estimatedSamplesToProcess_;
  int processedSamples_;

  std::vector<PremiereProExportContainer> exportContainers_;
  juce::OwnedArray<MixPresentationLoudness> mixPresentationLoudnesses_;
};