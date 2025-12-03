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

#include "GainProcessor.h"

#include "GainEditor.h"

//==============================================================================
GainProcessor::GainProcessor(MultiChannelRepository* gainRepository)
    : numChannels(getHostWideLayout().size()),
      gainrepo_id_(juce::Uuid()),
      channelGains_(gainRepository),  // passing an empty value tree, will
                                      // update in InitializeGainParameters
      // parameters(*this, nullptr, "APVTSGain",
      // createParameterLayout(getTotalNumInputChannels())), Will remove gain
      // when connecting to UI gain(dynamic_cast<juce::AudioParameterFloat
      // *>(parameters.getParameter("gain"))),
      gains_(InitializeGainParameters()),
      channelGainsDSP_(InitializeChannelGainsDSPs()) {
  channelGains_->registerListener(this);
}

GainProcessor::~GainProcessor() { channelGains_->deregisterListener(this); }

//==============================================================================
const juce::String GainProcessor::getName() const { return {"Gain"}; }

//==============================================================================
void GainProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  updateGains();
  m_samplesPerBlock_ = samplesPerBlock;
  for (auto i = 0; i < channelGainsDSP_.size(); ++i) {
    channelGainsDSP_[i].prepare(
        {sampleRate, static_cast<uint32_t>(samplesPerBlock),
         static_cast<uint32_t>(channelGainsDSP_.size())});
  }
}

void GainProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                 juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  juce::ScopedNoDenormals noDenormals;

  for (int i = 0; i < juce::jmin(static_cast<int>(channelGainsDSP_.size()),
                                 static_cast<int>(gains_.size()));
       i++) {
    setGain(i, gains_[i]->get());
  }

  for (int i = 0; i < juce::jmin(buffer.getNumChannels(),
                                 static_cast<int>(channelGainsDSP_.size()));
       i++) {
    juce::dsp::AudioBlock<float> block(&buffer.getArrayOfWritePointers()[i], 1,
                                       m_samplesPerBlock_);
    juce::dsp::ProcessContextReplacing<float> processContext(block);
    channelGainsDSP_[i].process(processContext);
  }
}

void GainProcessor::setGain(const int& channel, const float& gainValue) {
  channelGainsDSP_[channel].setGainLinear(gainValue);
}

void GainProcessor::ResetGains() {
  // the new ChannelGains object will have a default gain of 1.f
  channelGains_->update(ChannelGains(gainrepo_id_, "multichannel_Gains",
                                     getHostWideLayout().size()));
}

//==============================================================================
bool GainProcessor::hasEditor() const {
  return false;  // (change this to false if you choose to not supply an editor)
}

std::vector<juce::dsp::Gain<float>>
GainProcessor::InitializeChannelGainsDSPs() {
  std::vector<juce::dsp::Gain<float>> channelGainsDSPs(gains_.size());

  for (auto i = 0; i < gains_.size(); ++i) {
    channelGainsDSPs[i].setGainLinear(gains_[i]->get());
  }
  return channelGainsDSPs;
}

// This should only be called once
std::vector<std::shared_ptr<juce::AudioParameterFloat>>
GainProcessor::InitializeGainParameters() {
  // initialization list using tree constructor");
  channelGains_->update(
      ChannelGains(gainrepo_id_, "multichannel_Gains", numChannels));

  std::vector<std::shared_ptr<juce::AudioParameterFloat>> gainParameters(
      numChannels);

  for (int i = 0; i < numChannels; ++i) {
    std::string parameterID = "channelGain" + std::to_string(i);
    auto newGain = std::make_shared<juce::AudioParameterFloat>(
        parameterID, parameterID, juce::NormalisableRange<float>{0.0f, 2.0f},
        1.f, initParameterAttributes(2, juce::String()));
    gainParameters[i] = newGain;
  }
  return gainParameters;
}

void GainProcessor::updateAllAudioParameterFloats() {
  ChannelGains copy = channelGains_->get();
  std::vector<float> gains = copy.getGains();
  for (int i = 0; i < gains_.size(); i++) {
    *gains_[i] = gains[i];
  }
}

void GainProcessor::valueTreePropertyChanged(juce::ValueTree& tree,
                                             const juce::Identifier& property) {
  juce::ignoreUnused(tree);
  updateGains();
}

void GainProcessor::toggleChannelMute(const int& channel) {
  ChannelGains copy = channelGains_->get();
  copy.toggleChannelMute(channel);
  channelGains_->update(copy);
}
