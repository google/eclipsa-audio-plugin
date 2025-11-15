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

#include "MixPresentation.h"

#include <_types/_uint32_t.h>
#include <juce_core/juce_core.h>

#include <string>
#include <unordered_map>

#include "RepositoryItem.h"
#include "data_structures/src/MixPresentationLoudness.h"
#include "iamf/include/iamf_tools/commit_hash.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "mix_presentation.pb.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"
#include "user_metadata.pb.h"

static std::vector<std::string> splitStringByCarets(const std::string& input) {
  std::istringstream iss(input);
  std::vector<std::string> result;
  std::string temp;
  while (std::getline(iss, temp, '^')) {
    result.push_back(temp);
  }
  return result;
}

MixPresentationAudioElement::MixPresentationAudioElement()
    : RepositoryItemBase({}) {}

MixPresentation::MixPresentation() : RepositoryItemBase({}) {}

MixPresentation::MixPresentation(
    juce::Uuid id, juce::String name, float defaultMixGain,
    LanguageData::MixLanguages language,
    std::unordered_map<std::string, std::string> tags)
    : mixPresentationName_(name),
      RepositoryItemBase(id),
      defaultMixGain_(defaultMixGain),
      mixPresentationLanguage_(language),
      tags_(tags) {}

juce::String MixPresentation::languageToString(
    const LanguageData::MixLanguages& language) {
  return juce::String(LanguageData::getLanguageName(language));
}

LanguageData::MixLanguages MixPresentation::stringToLanguage(
    const juce::String& string) {
  return LanguageData::getLanguageEnum(string.toStdString());
}

void MixPresentation::populateIamfMixPresentationMetadata(
    const uint32_t mixPresentationId, const int sampleRate,
    iamf_tools_cli_proto::MixPresentationObuMetadata* mpMD,
    iamf_tools_cli_proto::UserMetadata& iamfMD,
    const MixPresentationLoudness mixPresentationLoudness,
    std::unordered_map<juce::Uuid, int>& audioElementIDMap) {
  static int mixGainParamBlockID = 100;

  mpMD->set_mix_presentation_id(mixPresentationId);
  // Indicate annotation language for this mix presentation.
  mpMD->set_count_label(1);
  mpMD->add_annotations_language(
      LanguageData::getLanguageCode(mixPresentationLanguage_));
  // Write mix presentation name.
  mpMD->add_localized_presentation_annotations(
      mixPresentationName_.toStdString());

  mpMD->set_include_mix_presentation_tags(true);
  writeMixPresentationTags(mpMD);

  // Only 1 submix currently.
  writeMixPresentationSubMix(mixGainParamBlockID, sampleRate, *mpMD,
                             mixPresentationLoudness, audioElementIDMap);
  ++mixGainParamBlockID;
}

void MixPresentation::writeMixPresentationSubMix(
    int& parameterBlockID, const int sampleRate,
    iamf_tools_cli_proto::MixPresentationObuMetadata& mpMD,
    const MixPresentationLoudness& mixPresentationLoudness,
    std::unordered_map<juce::Uuid, int>& audioElementIDMap) {
  auto submix = mpMD.add_sub_mixes();

  for (const auto& ae : audioElements_) {
    writeSubMixAudioElement(parameterBlockID, sampleRate, ae,
                            audioElementIDMap[ae.getId()],
                            *submix->add_audio_elements());
  }

  writeMixPresentationLayout(*submix, mixPresentationLoudness);
  writeOutputMixConfig(parameterBlockID, sampleRate, *submix);
}

void MixPresentation::writeSubMixAudioElement(
    int& parameterBlockID, const int sampleRate,
    const MixPresentationAudioElement& ae, int audioElementId,
    iamf_tools_cli_proto::SubMixAudioElement& submixAE) {
  submixAE.set_audio_element_id(audioElementId);
  submixAE.add_localized_element_annotations(ae.getName().toStdString());

  writeRenderingConfig(ae, submixAE);
  writeElementMixConfig(parameterBlockID, sampleRate, ae, submixAE);
}

void MixPresentation::writeRenderingConfig(
    const MixPresentationAudioElement& ae,
    iamf_tools_cli_proto::SubMixAudioElement& submixAE) {
  // Include the binaural rendering mode based on the `isBinaural` property
  if (ae.isBinaural()) {
    submixAE.mutable_rendering_config()->set_headphones_rendering_mode(
        iamf_tools_cli_proto::HeadPhonesRenderingMode::
            HEADPHONES_RENDERING_MODE_BINAURAL);
  } else {
    submixAE.mutable_rendering_config()->set_headphones_rendering_mode(
        iamf_tools_cli_proto::HeadPhonesRenderingMode::
            HEADPHONES_RENDERING_MODE_STEREO);
  }
}

void MixPresentation::writeElementMixConfig(
    int& parameterBlockID, const int sampleRate,
    const MixPresentationAudioElement& ae,
    iamf_tools_cli_proto::SubMixAudioElement& submixAE) {
  auto linearTodB = [&](float linear) -> float {
    return 20 * std::log10(std::max(linear,
                                    0.001f));  // Ensures log10(0) never happens
  };

  // Define a parameter block for 'element_mix_gain'.
  writeParamDefinition(
      parameterBlockID, sampleRate,
      *submixAE.mutable_element_mix_gain()->mutable_param_definition());

  submixAE.mutable_element_mix_gain()->set_default_mix_gain(
      convertToQ7Point8(linearTodB(ae.getDefaultMixGain())));
}

void MixPresentation::writeMixPresentationLayout(
    iamf_tools_cli_proto::MixPresentationSubMix& submix,
    const MixPresentationLoudness& mixPresentationLoudness) {
  // NOTE: Loudness could be measured by the plugin during export (maybe for the
  // actively selected playback layout(?)), but the IAMF encoder library is able
  // to compute this data if not present.
  auto layout = submix.add_layouts();
  writeLayout(*layout->mutable_loudness_layout(),
              Speakers::kStereo);  // Default to stereo for now.
  writeLoudnessInfo(*layout->mutable_loudness(), mixPresentationLoudness,
                    Speakers::kStereo);
  if (mixPresentationLoudness.getLargestLayout() == Speakers::kStereo) {
    return;
  }
  auto layout2 = submix.add_layouts();
  writeLayout(*layout2->mutable_loudness_layout(),
              mixPresentationLoudness.getLargestLayout());
  writeLoudnessInfo(*layout2->mutable_loudness(), mixPresentationLoudness,
                    mixPresentationLoudness.getLargestLayout());

  // For multiple layouts, write the larger layout before stereo
  submix.mutable_layouts()->SwapElements(0, 1);
}

void MixPresentation::writeLayout(
    iamf_tools_cli_proto::Layout& layout,
    const Speakers::AudioElementSpeakerLayout& aeSpeakerLayout) {
  writeLoudspeakersSsConventionLayout(layout);
  iamf_tools_cli_proto::SoundSystem soundSystem;

  // mapping audio element speaker layout to sound system
  // referencing 3.6.2 of the IAMF spec
  switch (aeSpeakerLayout) {
    case Speakers::kStereo:
      soundSystem = iamf_tools_cli_proto::SOUND_SYSTEM_A_0_2_0;
      break;
    case Speakers::k5Point1:
      soundSystem = iamf_tools_cli_proto::SOUND_SYSTEM_B_0_5_0;
      break;
    case Speakers::k5Point1Point2:
      soundSystem = iamf_tools_cli_proto::SOUND_SYSTEM_C_2_5_0;
      break;
    case Speakers::k5Point1Point4:
      soundSystem = iamf_tools_cli_proto::SOUND_SYSTEM_D_4_5_0;
      break;
    case Speakers::k7Point1:
      soundSystem = iamf_tools_cli_proto::SOUND_SYSTEM_I_0_7_0;
      break;
    case Speakers::k7Point1Point2:
      soundSystem = iamf_tools_cli_proto::SOUND_SYSTEM_10_2_7_0;
      break;
    case Speakers::k7Point1Point4:
      soundSystem = iamf_tools_cli_proto::SOUND_SYSTEM_J_4_7_0;
      break;
    case Speakers::k3Point1Point2:
      soundSystem = iamf_tools_cli_proto::SOUND_SYSTEM_11_2_3_0;
      break;
    case Speakers::kExpl9Point1Point6:
      soundSystem = iamf_tools_cli_proto::SOUND_SYSTEM_H_9_10_3;
      break;
  }
  layout.mutable_ss_layout()->set_sound_system(soundSystem);
  layout.mutable_ss_layout()->set_reserved(0);
}

void MixPresentation::writeLoudspeakersSsConventionLayout(
    iamf_tools_cli_proto::Layout& layout) {
  layout.set_layout_type(
      iamf_tools_cli_proto::LAYOUT_TYPE_LOUDSPEAKERS_SS_CONVENTION);
}

void MixPresentation::writeLoudnessInfo(
    iamf_tools_cli_proto::LoudnessInfo& loudnessInfo,
    const MixPresentationLoudness& mixPresLoudness,
    const Speakers::AudioElementSpeakerLayout& aeSpeakerLayout) {
  loudnessInfo.clear_info_type_bit_masks();
  loudnessInfo.add_info_type_bit_masks(
      iamf_tools_cli_proto::LOUDNESS_INFO_TYPE_TRUE_PEAK);
  loudnessInfo.set_integrated_loudness(convertToQ7Point8(
      mixPresLoudness.getLayoutIntegratedLoudness(aeSpeakerLayout)));
  loudnessInfo.set_true_peak(
      convertToQ7Point8(mixPresLoudness.getLayoutTruePeak(aeSpeakerLayout)));
  loudnessInfo.set_digital_peak(
      convertToQ7Point8(mixPresLoudness.getLayoutDigitalPeak(aeSpeakerLayout)));
}

void MixPresentation::writeOutputMixConfig(
    int& parameterBlockID, const int sampleRate,
    iamf_tools_cli_proto::MixPresentationSubMix& submix) {
  // Define a parameter block for 'output_mix_gain'.
  writeParamDefinition(
      parameterBlockID, sampleRate,
      *submix.mutable_output_mix_gain()->mutable_param_definition());

  submix.mutable_output_mix_gain()->set_default_mix_gain(
      convertToQ7Point8(getGainIndB()));
}

void MixPresentation::writeParamDefinition(
    int& parameterBlockID, const int sampleRate,
    iamf_tools_cli_proto::ParamDefinition& paramDef) {
  paramDef.set_parameter_id(parameterBlockID);
  paramDef.set_parameter_rate(sampleRate);
  paramDef.set_param_definition_mode(true);
  paramDef.set_reserved(0);
  parameterBlockID++;
}

bool MixPresentation::operator==(const MixPresentation& other) const {
  if (other.id_ != id_ || other.mixPresentationName_ != mixPresentationName_) {
    return false;
  }
  for (auto audioElement = audioElements_.begin();
       audioElement != audioElements_.end(); audioElement++) {
    if (std::find(other.audioElements_.begin(), other.audioElements_.end(),
                  *audioElement) == other.audioElements_.end()) {
      return false;
    }
  }
  return true;
}

MixPresentation MixPresentation::fromTree(const juce::ValueTree tree) {
  jassert(tree.hasProperty(kId));

  int i = static_cast<int>(tree[kLanguage]);
  LanguageData::MixLanguages language =
      static_cast<LanguageData::MixLanguages>(i);

  std::vector<std::string> tagNamesVec =
      splitStringByCarets(tree[kTagNames].toString().toStdString());

  std::vector<std::string> tagValuesVec =
      splitStringByCarets(tree[kTagValues].toString().toStdString());

  // handle cases where the listener fired but the tags were not fully set
  if (tagNamesVec.size() != tagValuesVec.size()) {
    return MixPresentation(juce::Uuid(tree[kId]), tree[kPresentationName],
                           tree[kDefaultMixGain], language, {});
  }

  std::unordered_map<std::string, std::string> tags;

  for (auto i = 0; i < tagNamesVec.size(); i++) {
    tags.insert({tagNamesVec[i], tagValuesVec[i]});
  }

  MixPresentation presentation(juce::Uuid(tree[kId]), tree[kPresentationName],
                               tree[kDefaultMixGain], language, tags);

  juce::ValueTree audioElements = tree.getChildWithName(kAudioElements);
  for (auto audioElement : audioElements) {
    presentation.audioElements_.push_back(
        MixPresentationAudioElement::fromTree(audioElement));
  }

  return presentation;
}

juce::ValueTree MixPresentation::toValueTree() const {
  // iterate through the map
  // add each key value pair to the tree
  std::string tagNames;
  std::string tagValues;

  for (auto tag : tags_) {
    tagNames += tag.first + "^";
    tagValues += tag.second + "^";
  }
  juce::ValueTree tree{
      kTreeType,
      {{kId, id_.toString()},
       {kPresentationName, mixPresentationName_},
       {kDefaultMixGain, defaultMixGain_},
       {kLanguage, static_cast<int>(mixPresentationLanguage_)},
       {kTagNames, juce::String(tagNames)},
       {kTagValues, juce::String(tagValues)}}};  // write false by default
  // allow the for loop to check if any AE is soloed
  juce::ValueTree elementsTree =
      tree.getOrCreateChildWithName(kAudioElements, nullptr);

  for (auto audioElement : audioElements_) {
    elementsTree.appendChild(audioElement.toValueTree(), nullptr);
  }
  return tree;
}

void MixPresentation::addAudioElement(const juce::Uuid id,
                                      const float defaultMixGain,
                                      const juce::String& name,
                                      bool isBinaural) {
  audioElements_.emplace_back(id, defaultMixGain, name, isBinaural);
}

void MixPresentation::removeAudioElement(juce::Uuid id) {
  audioElements_.erase(
      std::remove_if(audioElements_.begin(), audioElements_.end(),
                     [id](const MixPresentationAudioElement& audioElement) {
                       return audioElement.getId() == id;
                     }),
      audioElements_.end());
}

float MixPresentation::getAudioElementMixGain(juce::Uuid id) const {
  for (auto audioElement : audioElements_) {
    if (audioElement.getId() == id) {
      return audioElement.getDefaultMixGain();
    }
  }
  return -1;
}

void MixPresentation::setDefaultMixGain(juce::Uuid id, float defaultMixGain) {
  MixPresentationAudioElement* audioElement = findAudioElement(id);
  if (audioElement) {
    audioElement->setDefaultMixGain(defaultMixGain);
  }
}

void MixPresentation::setBinaural(juce::Uuid id, bool isBinaural) {
  MixPresentationAudioElement* audioElement = findAudioElement(id);
  if (audioElement) {
    audioElement->setBinaural(isBinaural);
  }
}

void MixPresentation::removeTag(const std::string& buttonText) {
  size_t nameEnd = buttonText.find(": ");

  // if ": " is not found, return
  if (nameEnd == std::string::npos) {
    return;
  }

  std::string tagName = buttonText.substr(0, nameEnd);
  tags_.erase(tagName);
}

void MixPresentation::writeMixPresentationTags(
    iamf_tools_cli_proto::MixPresentationObuMetadata* mpMD) {
  ::iamf_tools_cli_proto::MixPresentationTags* mixPresentationTags =
      new ::iamf_tools_cli_proto::MixPresentationTags();
  std::unordered_map<std::string, std::string> exportTags = tags_;

  // add the encoder tag if it is not already present
  if (!exportTags.contains(kEncoderTagName_)) {
    exportTags.insert({kEncoderTagName_, GIT_COMMIT_HASH});
  }

  for (auto& tag : exportTags) {
    std::string* name = new std::string(tag.first);
    std::string* value = new std::string(tag.second);
    ::iamf_tools_cli_proto::MixPresentationTag* newTag =
        mixPresentationTags->add_tags();
    newTag->set_allocated_tag_name(name);
    newTag->set_allocated_tag_value(value);
  }

  mpMD->set_allocated_mix_presentation_tags(mixPresentationTags);
}

bool MixPresentation::isAudioElementBinaural(juce::Uuid id) const {
  for (const auto& audioElement : audioElements_) {
    if (audioElement.getId() == id) {
      return audioElement.isBinaural();
    }
  }
  return false;
}

MixPresentationAudioElement* MixPresentation::findAudioElement(juce::Uuid id) {
  for (auto& audioElement : audioElements_) {
    if (audioElement.getId() == id) {
      return &audioElement;
    }
  }
  return nullptr;
}
