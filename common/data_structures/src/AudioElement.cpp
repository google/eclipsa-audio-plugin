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

#include "AudioElement.h"

#include "RepositoryItem.h"
#include "audio_element.pb.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

AudioElement::AudioElement() : RepositoryItemBase({}) {}

AudioElement::AudioElement(juce::Uuid id) : RepositoryItemBase(id) {}

AudioElement::AudioElement(juce::Uuid id, juce::String name,
                           Speakers::AudioElementSpeakerLayout channelConfig,
                           int firstChannel)
    : RepositoryItemBase(id),
      name_(name),
      channelConfig_(channelConfig),
      firstChannel_(firstChannel) {}

AudioElement::AudioElement(juce::Uuid id, juce::String name,
                           juce::String description,
                           Speakers::AudioElementSpeakerLayout channelConfig,
                           int firstChannel)
    : RepositoryItemBase(id),
      name_(name),
      description_(description),
      channelConfig_(channelConfig),
      firstChannel_(firstChannel) {}

AudioElement AudioElement::fromTree(const juce::ValueTree tree) {
  jassert(tree.hasProperty(kId));
  jassert(tree.hasProperty(kName));
  jassert(tree.hasProperty(kDescription));
  jassert(tree.hasProperty(kChannelConfig));
  jassert(tree.hasProperty(kFirstChannel));
  return AudioElement(
      juce::Uuid(tree[kId]), tree[kName], tree[kDescription],
      Speakers::AudioElementSpeakerLayout((int)tree[kChannelConfig]),
      tree[kFirstChannel]);
}

bool AudioElement::operator==(const AudioElement& other) const {
  return other.id_ == id_ && other.name_ == name_ &&
         other.description_ == description_ &&
         other.channelConfig_ == channelConfig_;
}

bool AudioElement::isInitialized() const { return name_ != ""; }

void AudioElement::setName(juce::String name) { name_ = name; }

void AudioElement::setDescription(juce::String description) {
  description_ = description;
}

void AudioElement::setChannelConfig(
    Speakers::AudioElementSpeakerLayout channelConfig) {
  channelConfig_ = channelConfig;
}

void AudioElement::setFirstChannel(int firstChannel) {
  firstChannel_ = firstChannel;
}

juce::ValueTree AudioElement::toValueTree() const {
  return {kTreeType,
          {
              {kId, id_.toString()},
              {kName, name_},
              {kDescription, description_},
              {kChannelConfig, (int)channelConfig_},
              {kFirstChannel, firstChannel_},
          }};
}

void AudioElement::populateIamfAudioElementMetadata(
    iamf_tools_cli_proto::AudioElementObuMetadata* aeMD, const int aeID,
    int& minimumSubstreamId) const {
  aeMD->set_audio_element_id(aeID);

  AudioElement_t aeType =
      channelConfig_.isAmbisonics()
          ? iamf_tools_cli_proto::AUDIO_ELEMENT_SCENE_BASED
          : iamf_tools_cli_proto::AUDIO_ELEMENT_CHANNEL_BASED;
  aeMD->set_audio_element_type(aeType);

  aeMD->set_reserved(0);
  aeMD->set_codec_config_id(200);

  int aeCoupledSubstreams = channelConfig_.getCoupledChannelCount();
  int aeUncoupledSubstreams = channelConfig_.getUncoupledChannelCount();
  aeMD->set_num_substreams(aeCoupledSubstreams + aeUncoupledSubstreams);
  for (int i = minimumSubstreamId;
       i < aeMD->num_substreams() + minimumSubstreamId; ++i) {
    aeMD->add_audio_substream_ids(i);
  }
  minimumSubstreamId = minimumSubstreamId + aeMD->num_substreams();

  aeMD->set_num_parameters(0);

  // Generate scalable_channel_layout_config for a channel-based AE.
  if (aeType == iamf_tools_cli_proto::AUDIO_ELEMENT_CHANNEL_BASED) {
    auto scLayoutConfig = aeMD->mutable_scalable_channel_layout_config();
    populateScalableChannelLayoutConfig(scLayoutConfig, aeCoupledSubstreams,
                                        aeUncoupledSubstreams);
  }
  // Generate Ambisonics config for scene-based AE.
  else if (aeType == iamf_tools_cli_proto::AUDIO_ELEMENT_SCENE_BASED) {
    auto ambisonicsConfig = aeMD->mutable_ambisonics_config();
    populateAmbisonicsConfig(ambisonicsConfig, aeMD->num_substreams());
  }
}

void AudioElement::populateScalableChannelLayoutConfig(
    iamf_tools_cli_proto::ScalableChannelLayoutConfig* sclc,
    const int coupledSubstreams, const int uncoupledSubstreams) const {
  // NOTE: There should be a channel config per layer. Keeping things simple
  // with 1 layer for now.
  const int kNumLayers = 1;

  sclc->set_num_layers(kNumLayers);
  sclc->set_reserved(0);

  while (sclc->channel_audio_layer_configs_size() < kNumLayers) {
    auto clCfg = sclc->add_channel_audio_layer_configs();
    clCfg->set_loudspeaker_layout(channelConfig_.getIamfLayout());
    clCfg->set_output_gain_is_present_flag(0);
    clCfg->set_recon_gain_is_present_flag(0);
    clCfg->set_reserved_a(0);
    clCfg->set_substream_count(coupledSubstreams + uncoupledSubstreams);
    clCfg->set_coupled_substream_count(coupledSubstreams);

    if (channelConfig_.isExpandedLayout()) {
      clCfg->set_expanded_loudspeaker_layout(channelConfig_.getIamfExpl());
    }
  }
}

void AudioElement::populateAmbisonicsConfig(
    iamf_tools_cli_proto::AmbisonicsConfig* ambisonicsConfig,
    const int aeNumSubstreams) const {
  // NOTE: Only using AMBISONICS_MODE_MONO currently.
  const auto kAmbisonicsMode = iamf_tools_cli_proto::AMBISONICS_MODE_MONO;
  ambisonicsConfig->set_ambisonics_mode(kAmbisonicsMode);

  if (kAmbisonicsMode == iamf_tools_cli_proto::AMBISONICS_MODE_MONO) {
    auto ambiMonoCfg = ambisonicsConfig->mutable_ambisonics_mono_config();

    ambiMonoCfg->set_output_channel_count(aeNumSubstreams);
    ambiMonoCfg->set_substream_count(aeNumSubstreams);

    for (int i = 0; i < aeNumSubstreams; ++i) {
      ambiMonoCfg->add_channel_mapping(i);
    }
  } else if (kAmbisonicsMode ==
             iamf_tools_cli_proto::AMBISONICS_MODE_PROJECTION) {
    auto ambiProjCfg = ambisonicsConfig->mutable_ambisonics_projection_config();

    ambiProjCfg->set_output_channel_count(-1);
    ambiProjCfg->set_substream_count(aeNumSubstreams);
    ambiProjCfg->set_coupled_substream_count(aeNumSubstreams);
  }
}

void AudioElement::populateIamfAudioFrameMetadata(
    iamf_tools_cli_proto::AudioFrameObuMetadata* afMD, const int aeID) const {
  afMD->set_audio_element_id(aeID);
  afMD->set_samples_to_trim_at_end(0);
  afMD->set_samples_to_trim_at_start(0);
  afMD->set_samples_to_trim_at_end_includes_padding(false);
  afMD->set_samples_to_trim_at_start_includes_codec_delay(false);

  populateChannelMetadatas(afMD);
}

void AudioElement::populateChannelMetadatas(
    iamf_tools_cli_proto::AudioFrameObuMetadata* afMD) const {
  const std::vector chLabels = channelConfig_.getIamfChannelLabels();
  for (int i = 0; i < chLabels.size(); ++i) {
    auto chMD = afMD->add_channel_metadatas();
    chMD->set_channel_id(i);
    chMD->set_channel_label(chLabels[i]);
  }
}