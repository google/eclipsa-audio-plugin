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

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>

#include "../src/RepositoryItem.h"
#include "audio_element.pb.h"
#include "audio_frame.pb.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class AudioElement final : public RepositoryItemBase {
 public:
  using AudioElement_t = iamf_tools_cli_proto::AudioElementType;
  using LoudspeakerLayout_t = iamf_tools_cli_proto::LoudspeakerLayout;

  AudioElement();

  AudioElement(juce::Uuid id);

  AudioElement(juce::Uuid id, juce::String name,
               Speakers::AudioElementSpeakerLayout channelConfig,
               int firstChannel);

  AudioElement(juce::Uuid id, juce::String name, juce::String description,
               Speakers::AudioElementSpeakerLayout channelConfig,
               int firstChannel);

  bool operator==(const AudioElement& other) const;
  bool operator!=(const AudioElement& other) const { return !(other == *this); }

  bool isInitialized() const;

  static AudioElement fromTree(const juce::ValueTree tree);
  virtual juce::ValueTree toValueTree() const override;

  void setName(juce::String name);
  void setDescription(juce::String description);
  void setChannelConfig(Speakers::AudioElementSpeakerLayout channelConfig);
  void setFirstChannel(int firstChannel);

  juce::String getName() const { return name_; }
  juce::String getDescription() const { return description_; }
  Speakers::AudioElementSpeakerLayout getChannelConfig() const {
    return channelConfig_;
  }
  int getFirstChannel() const { return firstChannel_; }
  int getChannelCount() const { return channelConfig_.getNumChannels(); }

  void populateIamfAudioElementMetadata(
      iamf_tools_cli_proto::AudioElementObuMetadata* aeMD, const int aeID,
      int& minimumSubstreamId) const;

  void populateIamfAudioFrameMetadata(
      iamf_tools_cli_proto::AudioFrameObuMetadata* afMD, const int aeID) const;

  inline static const juce::Identifier kTreeType{"audio_element"};
  inline static const juce::Identifier kName{"name"};
  inline static const juce::Identifier kDescription{"description"};
  inline static const juce::Identifier kChannelConfig{"channel_config"};
  inline static const juce::Identifier kFirstChannel{"first_channel"};

 private:
  void populateScalableChannelLayoutConfig(
      iamf_tools_cli_proto::ScalableChannelLayoutConfig* sclc,
      const int coupledSubstreams, const int uncoupledSubstreams) const;

  void populateAmbisonicsConfig(
      iamf_tools_cli_proto::AmbisonicsConfig* ambisonicsConfig,
      const int aeNumSubstreams) const;

  void populateChannelMetadatas(
      iamf_tools_cli_proto::AudioFrameObuMetadata* afMD) const;

  juce::String name_;
  juce::String description_ = "description";
  Speakers::AudioElementSpeakerLayout channelConfig_;
  int firstChannel_;
};