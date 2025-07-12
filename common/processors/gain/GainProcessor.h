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

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "../../data_repository/implementation/MultiChannelGainRepository.h"
#include "../processor_base/ProcessorBase.h"

//==============================================================================
class GainProcessor final : public ProcessorBase,
                            public juce::ValueTree::Listener {
 public:
  //==============================================================================
  GainProcessor(MultiChannelRepository* gainsRepository);
  ~GainProcessor() override;

  //==============================================================================
  // This is still currently being used by the GainEditor
  // Will replace with getGainParameters when developing the UI
  const juce::AudioParameterFloat* getGainParameter() const { return gain_; }
  std::vector<std::shared_ptr<juce::AudioParameterFloat>>
  InitializeGainParameters();

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  //==============================================================================
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  virtual void valueTreePropertyChanged(
      juce::ValueTree& tree, const juce::Identifier& property) override;

  int getGainRepoInputChannels() {
    return channelGains_->get().getTotalChannels();
  }

  std::vector<std::shared_ptr<juce::AudioParameterFloat>> getGains() {
    return gains_;
  }

  void reinitializeAfterStateRestore() override { updateGains(); }

  void toggleChannelMute(const int& channel);

  void ResetGains();

  void updateAllAudioParameterFloats();

  void updateGains() {
    updateAllAudioParameterFloats();
    channelGainsDSP_ = InitializeChannelGainsDSPs();
  }

  std::unordered_map<int, float> getMutedChannelsfromRepo() {
    return channelGains_->get().getMutedChannels();
  }

  //==============================================================================
  const int numChannels;

 private:
  std::vector<juce::dsp::Gain<float>> InitializeChannelGainsDSPs();

  //==============================================================================
  MultiChannelRepository* channelGains_;

  void setGain(const int& channel, const float& gainValue);

  juce::AudioParameterFloatAttributes initParameterAttributes(
      int decimalPlaces, juce::String&& label) const {
    return juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction([decimalPlaces](float value, int unused) {
          juce::ignoreUnused(unused);
          return juce::String(value, decimalPlaces, false);
        })
        .withLabel(label);
  }

  // juce::AudioProcessorValueTreeState parameters;
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout(
      const int& numChannels = 1) {
    juce::AudioProcessorValueTreeState::ParameterLayout params;
    params.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"gain", 1}, "Gain",
        juce::NormalisableRange<float>{0.0f, 2.0f}, 1.0f,
        initParameterAttributes(2, juce::String())));

    return params;
  }

  juce::Uuid gainrepo_id_;

  // This is still being used be the GainEditor
  // Will replace w/ gains when connecting to UI
  juce::AudioParameterFloat* gain_;
  std::vector<std::shared_ptr<juce::AudioParameterFloat>> gains_;

  std::vector<juce::dsp::Gain<float>> channelGainsDSP_;
  juce::dsp::Gain<float> gainDSP_;

  int m_samplesPerBlock_;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainProcessor)
};
