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

#include "MixMonitorProcessor.h"

#include "substream_rdr/substream_rdr_utils/Speakers.h"

MixMonitorProcessor::MixMonitorProcessor(RoomSetupRepository& repo,
                                         SpeakerMonitorData& data)
    : roomSetupRepo_(repo),
      rtData_(data),
      playbackLayout_(juce::AudioChannelSet::mono()) {
  // Register as a listener to the room setup repository.
  roomSetupRepo_.registerListener(this);

  // Clear stats on construction.
  rtData_.loudnessEBU128.update({});
}

void MixMonitorProcessor::prepareToPlay(double sampleRate,
                                        int samplesPerBlock) {
  // Update playback layout from repository.
  RoomSetup roomData = roomSetupRepo_.get();
  juce::AudioChannelSet currPlaybackLayout =
      roomData.getSpeakerLayout().getRoomSpeakerLayout().getChannelSet();
  // Update playback layout if valid.
  if (currPlaybackLayout != juce::AudioChannelSet::disabled()) {
    playbackLayout_ = currPlaybackLayout;
  }

  // Construct a measurement object if necessary.
  if (loudnessImpl_ == nullptr) {
    loudnessImpl_ = std::make_unique<MeasureEBU128>(sampleRate);
  }

  // Reset stats on playback start.
  rtData_.resetStats = true;
  rtData_.loudnessEBU128.update({});
}

void MixMonitorProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages) {
  // Get portion of buffer containing rendered channels.
  rdrBuffer_ = getRenderedBuffer(buffer);

  // UI triggered a stats update (rare).
  if (rtData_.resetStats.load()) {
    loudnessImpl_->reset(playbackLayout_, rdrBuffer_);
    rtData_.resetStats.store(false);
  }
  // Measure EBU128 loudness statistics.
  loudnessStats_ = loudnessImpl_->measureLoudness(playbackLayout_, rdrBuffer_);
  rtData_.loudnessEBU128.update(loudnessStats_);

  // Measure per-channel loudness in dB.
  std::vector<float> loudnesses(rdrBuffer_.getNumChannels());
  for (int i = 0; i < rdrBuffer_.getNumChannels(); ++i) {
    float loudness = 20.0f * std::log10(rdrBuffer_.getRMSLevel(
                                 i, 0, rdrBuffer_.getNumSamples()));
    loudnesses[i] = loudness;
  }
  rtData_.playbackLoudness.update(loudnesses);
}

const juce::AudioBuffer<float> MixMonitorProcessor::getRenderedBuffer(
    juce::AudioBuffer<float>& busBuff) {
  auto dataPtrs = busBuff.getArrayOfWritePointers();
  int numRdrCh = playbackLayout_.size();
  return juce::AudioBuffer<float>(dataPtrs, numRdrCh, busBuff.getNumSamples());
}

void MixMonitorProcessor::valueTreePropertyChanged(
    juce::ValueTree& treeWhosePropertyHasChanged,
    const juce::Identifier& property) {
  // The export profile does not change what is measured, so keep the stats.
  if (property == RoomSetup::kExportProfile) {
    return;
  }
  playbackLayout_ = roomSetupRepo_.get()
                        .getSpeakerLayout()
                        .getRoomSpeakerLayout()
                        .getChannelSet();
  rtData_.resetStats = true;
  rtData_.loudnessEBU128.update({});
}