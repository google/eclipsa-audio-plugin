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

#include <string>
#include <unordered_map>
#include <vector>

#include "../src/RepositoryItem.h"
#include "LanguageCodeMetaData.h"
#include "data_structures/src/MixPresentationLoudness.h"
#include "iamf/include/iamf_tools/commit_hash.h"
#include "mix_presentation.pb.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"
#include "user_metadata.pb.h"

// Convert gain to Q7.8 format, which is a 16-bit signed integer with
// 8 fractional bits (see: https://en.wikipedia.org/wiki/Q_(number_format))
// In practice, this means multiplying the gain by 256.
static inline int convertToQ7Point8(const float value) { return value * 256; }

struct MixPresentationAudioElement : public RepositoryItemBase {
 public:
  MixPresentationAudioElement();
  MixPresentationAudioElement(juce::Uuid id, float defaultMixGain,
                              const juce::String& name, bool isBinaural = true)
      : RepositoryItemBase(id),
        defaultMixGain_(defaultMixGain),
        name_(name),
        isBinaural_(isBinaural) {}

  bool operator==(const MixPresentationAudioElement& other) const {
    return other.id_ == id_ && other.defaultMixGain_ == defaultMixGain_ &&
           other.isBinaural_ == isBinaural_;
  }
  bool operator!=(const MixPresentationAudioElement& other) const {
    return !(other == *this);
  }

  static MixPresentationAudioElement fromTree(const juce::ValueTree tree) {
    return MixPresentationAudioElement(juce::Uuid(tree[kId]),
                                       tree[kDefaultMixGain], tree[kAEName],
                                       tree[kIsBinaural]);
  }

  virtual juce::ValueTree toValueTree() const override {
    return {kTreeType,
            {{kId, id_.toString()},
             {kDefaultMixGain, defaultMixGain_},
             {kReferenceId, referenceId_},
             {kAEName, name_},
             {kIsBinaural, isBinaural_}}};
  }

  juce::String getName() const { return name_; }
  float getDefaultMixGain() const { return defaultMixGain_; }
  void setDefaultMixGain(float defaultMixGain) {
    defaultMixGain_ = defaultMixGain;
  }

  juce::Uuid getId() const { return id_; }
  unsigned int getReferenceId() const { return referenceId_; }
  bool isBinaural() const { return isBinaural_; }
  void setBinaural(bool binaural) { isBinaural_ = binaural; }

  inline static const juce::Identifier kTreeType{
      "mix_presentation_audio_element"};
  inline static const juce::Identifier kDefaultMixGain{"default_mix_gain"};
  inline static const juce::Identifier kReferenceId{"reference_id"};
  inline static const juce::Identifier kAEName{"name"};
  inline static const juce::Identifier kIsBinaural{"is_binaural"};

 private:
  juce::String name_;
  float defaultMixGain_ = 1.f;
  int referenceId_;
  bool isBinaural_ = false;
};

class MixPresentation final : public RepositoryItemBase {
 public:
  MixPresentation();

  MixPresentation(juce::Uuid id, juce::String name, float defaultMixGain,
                  LanguageData::MixLanguages language =
                      LanguageData::MixLanguages::Undetermined,
                  std::unordered_map<std::string, std::string> tags = {});

  bool operator==(const MixPresentation& other) const;
  bool operator!=(const MixPresentation& other) const {
    return !(other == *this);
  }

  void setName(juce::String name) { mixPresentationName_ = name; }

  void addAudioElement(const juce::Uuid id, const float defaultMixGain,
                       const juce::String& name, bool isBinaural = true);
  void removeAudioElement(juce::Uuid id);

  float getDefaultMixGain() const { return defaultMixGain_; }
  int getGainIndB() const { return 20.f * log10(defaultMixGain_); }
  float getAudioElementMixGain(juce::Uuid id) const;
  void setDefaultMixGain(juce::Uuid id, float defaultMixGain);
  bool isAudioElementBinaural(juce::Uuid id) const;
  void setBinaural(juce::Uuid id, bool isBinaural);

  static juce::String languageToString(
      const LanguageData::MixLanguages& language);
  static LanguageData::MixLanguages stringToLanguage(
      const juce::String& string);

  LanguageData::MixLanguages getMixPresentationLanguage() const {
    return mixPresentationLanguage_;
  }

  juce::String getName() const { return mixPresentationName_; }
  std::vector<MixPresentationAudioElement> getAudioElements() const {
    return audioElements_;
  }
  void setDefaultMixGain(float defaultMixGain) {
    defaultMixGain_ = defaultMixGain;
  }

  void setLanguage(const LanguageData::MixLanguages& language) {
    mixPresentationLanguage_ = language;
  }
  void setGainFromdB(int gainIndB) {
    defaultMixGain_ = std::pow(10.f, gainIndB / 20.f);
  }

  void populateIamfMixPresentationMetadata(
      const uint32_t mixPresentationId, const int sampleRate,
      iamf_tools_cli_proto::MixPresentationObuMetadata* mpMD,
      iamf_tools_cli_proto::UserMetadata& iamfMD,
      const MixPresentationLoudness mixPresentationLoudness,
      std::unordered_map<juce::Uuid, int>& audioElementIDMap);

  static MixPresentation fromTree(const juce::ValueTree tree);
  virtual juce::ValueTree toValueTree() const override;

  std::unordered_map<std::string, std::string> getTags() const { return tags_; }

  void addTagPair(const std::string& nameTag, const std::string& valueTag) {
    // handle overwriting existing tags
    if (tags_.contains(nameTag)) {
      tags_[nameTag] = valueTag;
    } else {
      tags_.insert({nameTag, valueTag});
    }
  }

  void removeTag(const std::string& buttonText);

  inline static const juce::Identifier kTreeType{"mix_presentation"};
  inline static const juce::Identifier kAudioElements{"audio_elements"};
  inline static const juce::Identifier kPresentationName{"presentation_name"};
  inline static const juce::Identifier kDefaultMixGain{"default_mix_gain"};
  inline static const juce::Identifier kLanguage{"language"};
  inline static const juce::Identifier kTagNames{"tag_names"};
  inline static const juce::Identifier kTagValues{"tag_values"};
  inline static const juce::Identifier kIsBinaural{"is_binaural"};

 private:
  // Methods for populating Mix Presentation metadata. Each method corresponds
  // to a message field in 'mix_presentation.proto'.
  void writeMixPresentationSubMix(
      int& parameterBlockID, const int sampleRate,
      iamf_tools_cli_proto::MixPresentationObuMetadata& mpMD,
      const MixPresentationLoudness& mixPresentationLoudness,
      std::unordered_map<juce::Uuid, int>& audioElementIDMap);
  void writeSubMixAudioElement(
      int& parameterBlockID, const int sampleRate,
      const MixPresentationAudioElement& ae, int audioElementId,
      iamf_tools_cli_proto::SubMixAudioElement& submixAE);
  void writeRenderingConfig(const MixPresentationAudioElement& ae,
                            iamf_tools_cli_proto::SubMixAudioElement& submixAE);
  void writeElementMixConfig(
      int& parameterBlockID, const int sampleRate,
      const MixPresentationAudioElement& ae,
      iamf_tools_cli_proto::SubMixAudioElement& submixAE);
  void writeMixPresentationLayout(
      iamf_tools_cli_proto::MixPresentationSubMix& submix,
      const MixPresentationLoudness& mixPresentationLoudness);
  void writeLayout(iamf_tools_cli_proto::Layout& layout,
                   const Speakers::AudioElementSpeakerLayout& aeSpeakerLayout);
  void writeLoudspeakersSsConventionLayout(
      iamf_tools_cli_proto::Layout& layout);
  void writeLoudnessInfo(
      iamf_tools_cli_proto::LoudnessInfo& loudnessInfo,
      const MixPresentationLoudness& mixPresLoudness,
      const Speakers::AudioElementSpeakerLayout& aeSpeakerLayout);
  void writeOutputMixConfig(
      int& parameterBlockID, const int sampleRate,
      iamf_tools_cli_proto::MixPresentationSubMix& submix);
  void writeParamDefinition(int& parameterBlockID, const int sampleRate,
                            iamf_tools_cli_proto::ParamDefinition& paramDef);
  void writeAnchoredLoudness(void) {}
  void writeAnchorElement(void) {}
  void writeMixPresentationTags(
      iamf_tools_cli_proto::MixPresentationObuMetadata* mpMD);
  MixPresentationAudioElement* findAudioElement(juce::Uuid id);

  const std::string kEncoderTagName_ = "iamf_encoder";

  std::vector<MixPresentationAudioElement> audioElements_;
  bool deleted_ = false;
  std::unordered_map<std::string, std::string> tags_;
  juce::String mixPresentationName_;
  float defaultMixGain_ = 1.f;
  LanguageData::MixLanguages mixPresentationLanguage_;
};