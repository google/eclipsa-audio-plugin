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
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class AudioElementSpatialLayout final : public RepositoryItemBase {
 public:
  enum class Elevation {
    kNone = -1,
    kFlat = 0,
    kTent = 1,
    kArch = 2,
    kDome = 3,
    kCurve = 4
  };

  AudioElementSpatialLayout();
  AudioElementSpatialLayout(juce::Uuid id);
  AudioElementSpatialLayout(juce::Uuid id, juce::String name,
                            juce::Uuid audioElement, juce::int32 firstChannel,
                            Speakers::AudioElementSpeakerLayout totalChannels,
                            bool panningEnabled = false,
                            Elevation elevation = Elevation::kNone,
                            bool layoutSelected = false);

  bool operator==(const AudioElementSpatialLayout& other) const;
  bool operator!=(const AudioElementSpatialLayout& other) const {
    return !(other == *this);
  }

  void copyValuesFrom(const AudioElementSpatialLayout& other);

  bool isInitialized() const;

  static AudioElementSpatialLayout fromTree(const juce::ValueTree tree);
  virtual juce::ValueTree toValueTree() const override;

  void setName(juce::String name);
  void setAudioElementId(juce::Uuid audioElementId);
  void setFirstChannel(juce::int32 firstChannel);
  void setLayout(Speakers::AudioElementSpeakerLayout layout);
  void setElevation(Elevation elevation);
  void setLayoutSelected(bool layoutSelected);
  void setPanningEnabled(bool panningEnabled);
  // True when the user typed the name, so the host's track name must not
  // replace it.
  void setNameIsCustom(bool nameIsCustom);

  juce::String getName() const { return name_; }
  juce::Uuid getAudioElementId() const { return audioElementId_; }
  juce::int32 getFirstChannel() const { return firstChannel_; }
  Speakers::AudioElementSpeakerLayout getChannelLayout() const {
    return channelLayout_;
  }
  Elevation getElevation() const { return elevation_; }
  bool isLayoutSelected() const { return layoutSelected_; }
  bool isPanningEnabled() const { return panningEnabled_; }
  bool isNameCustom() const { return nameIsCustom_; }

  inline static const juce::Identifier kTreeType{
      "audio_element_spatial_layout"};
  inline static const juce::Identifier kName{"name"};
  inline static const juce::Identifier kaudioElement{"audio_element_id"};
  inline static const juce::Identifier kFirstChannel{"first_channel"};
  inline static const juce::Identifier kLayout{"layout"};
  inline static const juce::Identifier kElevation{"elevation"};
  inline static const juce::Identifier kLayoutSelected{"layout_selected"};
  inline static const juce::Identifier kPanningEnabled{"panning_enabled"};
  inline static const juce::Identifier kNameIsCustom{"name_is_custom"};

 private:
  juce::String name_;
  juce::Uuid audioElementId_;
  juce::int32 firstChannel_;
  Speakers::AudioElementSpeakerLayout channelLayout_;
  Elevation elevation_;
  bool layoutSelected_;
  bool panningEnabled_;
  bool nameIsCustom_ = false;
};