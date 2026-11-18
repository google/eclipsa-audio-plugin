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

#include "AudioElementSpatialLayout.h"

#include "RepositoryItem.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

AudioElementSpatialLayout::AudioElementSpatialLayout()
    : RepositoryItemBase({}),
      audioElementId_(nullptr),
      firstChannel_(-1),
      channelLayout_(Speakers::kMono),
      elevation_(Elevation::kNone),
      layoutSelected_(false),
      panningEnabled_(false) {}

AudioElementSpatialLayout::AudioElementSpatialLayout(juce::Uuid id)
    : RepositoryItemBase(id),
      audioElementId_(nullptr),
      firstChannel_(-1),
      channelLayout_(Speakers::kMono),
      elevation_(Elevation::kNone),
      layoutSelected_(false),
      panningEnabled_(false) {}

AudioElementSpatialLayout::AudioElementSpatialLayout(
    juce::Uuid id, juce::String name, juce::Uuid audioElement,
    juce::int32 firstChannel, Speakers::AudioElementSpeakerLayout layout,
    bool panningEnabled, Elevation elevation, bool layoutSelected)
    : RepositoryItemBase(id),
      name_(name),
      audioElementId_(audioElement),
      firstChannel_(firstChannel),
      channelLayout_(layout),
      elevation_(elevation),
      layoutSelected_(layoutSelected),
      panningEnabled_(panningEnabled) {}

void AudioElementSpatialLayout::copyValuesFrom(
    const AudioElementSpatialLayout& other) {
  name_ = other.name_;
  audioElementId_ = other.audioElementId_;
  firstChannel_ = other.firstChannel_;
  channelLayout_ = other.channelLayout_;
  elevation_ = other.elevation_;
  layoutSelected_ = other.layoutSelected_;
  panningEnabled_ = other.panningEnabled_;
  nameIsCustom_ = other.nameIsCustom_;
}

AudioElementSpatialLayout AudioElementSpatialLayout::fromTree(
    const juce::ValueTree tree) {
  jassert(tree.hasProperty(kId));
  jassert(tree.hasProperty(kName));
  jassert(tree.hasProperty(kaudioElement));
  AudioElementSpatialLayout layout =
      tree.hasProperty(kElevation)
          ? AudioElementSpatialLayout(
                juce::Uuid(tree[kId]), tree[kName],
                juce::Uuid(tree[kaudioElement]), tree[kFirstChannel],
                Speakers::AudioElementSpeakerLayout(tree[kLayout]),
                tree[kPanningEnabled],
                static_cast<Elevation>(
                    tree[kElevation].toString().getIntValue()),
                tree[kLayoutSelected])
          : AudioElementSpatialLayout(
                juce::Uuid(tree[kId]), tree[kName],
                juce::Uuid(tree[kaudioElement]), tree[kFirstChannel],
                Speakers::AudioElementSpeakerLayout(tree[kLayout]),
                tree[kPanningEnabled]);
  // Sessions saved before the flag existed load as not custom.
  layout.setNameIsCustom(tree.getProperty(kNameIsCustom, false));
  return layout;
}

bool AudioElementSpatialLayout::operator==(
    const AudioElementSpatialLayout& other) const {
  return other.id_ == id_ && other.name_ == name_ &&
         other.audioElementId_ == audioElementId_;
}

bool AudioElementSpatialLayout::isInitialized() const { return name_ != ""; }

void AudioElementSpatialLayout::setName(juce::String name) { name_ = name; }

void AudioElementSpatialLayout::setAudioElementId(juce::Uuid audioElementId) {
  audioElementId_ = audioElementId;
}

void AudioElementSpatialLayout::setFirstChannel(juce::int32 firstChannel) {
  firstChannel_ = firstChannel;
}

void AudioElementSpatialLayout::setLayout(
    Speakers::AudioElementSpeakerLayout layout) {
  channelLayout_ = layout;
}

void AudioElementSpatialLayout::setElevation(Elevation elevation) {
  elevation_ = elevation;
}

void AudioElementSpatialLayout::setLayoutSelected(bool layoutSelected) {
  layoutSelected_ = layoutSelected;
};

void AudioElementSpatialLayout::setPanningEnabled(bool panningEnabled) {
  panningEnabled_ = panningEnabled;
}

void AudioElementSpatialLayout::setNameIsCustom(bool nameIsCustom) {
  nameIsCustom_ = nameIsCustom;
}

juce::ValueTree AudioElementSpatialLayout::toValueTree() const {
  return {kTreeType,
          {{kId, id_.toString()},
           {kName, name_},
           {kaudioElement, audioElementId_.toString()},
           {kFirstChannel, firstChannel_},
           {kLayout, (int)channelLayout_},
           {kElevation, static_cast<int>(elevation_)},
           {kLayoutSelected, layoutSelected_},
           {kPanningEnabled, panningEnabled_},
           {kNameIsCustom, nameIsCustom_}}};
}
