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
#include <vector>

#include "../src/RepositoryItem.h"

struct AudioElementSoloMute : public RepositoryItemBase {
 public:
  AudioElementSoloMute() : RepositoryItemBase({}) {};
  AudioElementSoloMute(juce::Uuid id, int referenceID, const juce::String& name,
                       const bool isSoloed = false, const bool isMuted = false)
      : RepositoryItemBase(id),
        name_(name),
        referenceId_(referenceID),
        isSoloed_(isSoloed),
        isMuted_(isMuted) {}

  bool operator==(const AudioElementSoloMute& other) const {
    return other.id_ == id_ && other.isSoloed_ == isSoloed_ &&
           other.isMuted_ == isMuted_ && other.name_ == name_;
  }
  bool operator!=(const AudioElementSoloMute& other) const {
    return !(other == *this);
  }

  void setSoloed(bool isSoloed) { isSoloed_ = isSoloed; }

  void setMuted(bool isMuted) { isMuted_ = isMuted; }

  bool isSoloed() const { return isSoloed_; }

  bool isMuted() const { return isMuted_; }

  static AudioElementSoloMute fromTree(const juce::ValueTree tree) {
    return AudioElementSoloMute(juce::Uuid(tree[kId]), tree[kReferenceId],
                                tree[kAEName], tree[kSoloed], tree[kMuted]);
  }

  virtual juce::ValueTree toValueTree() const override {
    return {kTreeType,
            {{kId, id_.toString()},
             {kReferenceId, referenceId_},
             {kAEName, name_},
             {kSoloed, isSoloed_},
             {kMuted, isMuted_}}};
  }

  juce::String getName() const { return name_; }
  juce::Uuid getId() const { return id_; }
  unsigned int getReferenceId() const { return referenceId_; }

  inline static const juce::Identifier kTreeType{"audio_element_solo_mute"};
  inline static const juce::Identifier kReferenceId{"reference_id"};
  inline static const juce::Identifier kAEName{"name"};
  inline static const juce::Identifier kSoloed{"Soloed"};
  inline static const juce::Identifier kMuted{"Muted"};

 private:
  juce::String name_;
  int referenceId_;
  bool isSoloed_;
  bool isMuted_;
};

class MixPresentationSoloMute final : public RepositoryItemBase {
 public:
  MixPresentationSoloMute();
  MixPresentationSoloMute(juce::Uuid id, bool anySoloed = false);

  bool operator==(const MixPresentationSoloMute& other) const;
  bool operator!=(const MixPresentationSoloMute& other) const {
    return !(other == *this);
  }

  static MixPresentationSoloMute fromTree(const juce::ValueTree tree);
  virtual juce::ValueTree toValueTree() const override;

  void addAudioElement(const juce::Uuid id, const int referenceID,
                       const juce::String& name);

  void removeAudioElement(const juce::Uuid id);

  void setAudioElementSolo(const juce::Uuid& id, const bool isSoloed);

  void setAudioElementMute(const juce::Uuid& id, const bool isMuted);

  AudioElementSoloMute getAudioElement(const juce::Uuid& id) const;
  std::vector<AudioElementSoloMute> getAudioElements() const {
    return audioElements_;
  }

  bool getAnySoloed() const { return anySoloed_; }

  bool isAudioElementSoloed(const juce::Uuid& id) const;

  bool isAudioElementMuted(const juce::Uuid& id) const;

  inline static const juce::Identifier kTreeType{"mix_presentation_solo_mute"};
  inline static const juce::Identifier kAudioElements{"audio_elements"};
  inline static const juce::Identifier kAnySoloed{"any_soloed"};

 private:
  std::vector<AudioElementSoloMute> audioElements_;
  bool anySoloed_;
};