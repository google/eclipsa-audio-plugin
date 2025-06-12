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

#include "ChannelGains.h"

#include "RepositoryItem.h"

//==============================================================================
ChannelGains::ChannelGains()
    : RepositoryItemBase(juce::Uuid()),
      totalChannels_(1),
      gains_(std::vector<float>(1, 1.f)) {}

ChannelGains::ChannelGains(juce::Uuid id, std::vector<float> gains,
                           std::unordered_map<int, float> mutedChannels)
    : RepositoryItemBase(id),
      totalChannels_(gains.size()),
      gains_(gains),
      mutedChannels_(mutedChannels) {}

ChannelGains::ChannelGains(juce::Uuid id, juce::String name,
                           juce::int32 numChannels)
    : RepositoryItemBase(id),
      totalChannels_(numChannels),
      gains_(std::vector<float>(totalChannels_, 1.f)) {}

ChannelGains::~ChannelGains() {}

void ChannelGains::toggleChannelMute(const int& channel) {
  // The channel is not muted, mute it
  if (mutedChannels_.find(channel) == mutedChannels_.end()) {
    mutedChannels_[channel] = gains_[channel];
    gains_[channel] = 0.0f;
  } else {  // unmute the channel
    gains_[channel] = mutedChannels_[channel];
    mutedChannels_.erase(channel);
  }
}

ChannelGains ChannelGains::fromTree(juce::ValueTree tree) {
  jassert(tree.hasProperty(kId));
  jassert(tree.hasProperty(kTotalChannels));
  jassert(tree.hasProperty(kGains));
  jassert(tree.hasProperty(kMutedChannels));
  jassert(tree.hasProperty(kPrevGain));
  // muted channels and previous gain are optional
  // the map may be empty

  // parse the gains
  std::vector<std::string> string_vec =
      splitStringBySpace(tree[kGains].toString().toStdString());
  std::vector<float> gains = convertStringsToFloats(string_vec);

  if (tree[kMutedChannels].toString().isEmpty()) {
    return ChannelGains(juce::Uuid(tree[kId]), gains,
                        std::unordered_map<int, float>());
  }

  // parse the muted channels
  const std::vector<std::string> mutedChannelsStringVec =
      splitStringBySpace(tree[kMutedChannels].toString().toStdString());
  std::vector<int> mutedChannels = convertStringsToInts(mutedChannelsStringVec);

  // parse the previous gains
  const std::vector<std::string> prevGainStringVec =
      splitStringBySpace(tree[kPrevGain].toString().toStdString());
  std::vector<float> prevGains = convertStringsToFloats(prevGainStringVec);

  if (mutedChannels.size() != prevGains.size()) {
    return ChannelGains(juce::Uuid(tree[kId]), gains,
                        std::unordered_map<int, float>());
  }

  // create the unordered map
  std::unordered_map<int, float> mutedChannelsMap;
  for (int i = 0; i < mutedChannels.size(); i++) {
    mutedChannelsMap[mutedChannels[i]] = prevGains[i];
  }

  return ChannelGains(juce::Uuid(tree[kId]), gains, mutedChannelsMap);
}

juce::ValueTree ChannelGains::toValueTree() const {
  std::string gainsString;
  for (int i = 0; i < totalChannels_; i++) {
    gainsString += std::to_string(gains_[i]) + " ";
  }
  std::string mutedChannelsString;
  std::string prevGainString;
  for (auto pair : mutedChannels_) {
    mutedChannelsString += std::to_string(pair.first) + " ";
    prevGainString += std::to_string(pair.second) + " ";
  }
  juce::ValueTree tree{kTreeType,
                       {{kId, id_.toString()},
                        {kTotalChannels, totalChannels_},
                        {kGains, juce::String(gainsString)},
                        {kMutedChannels, juce::String(mutedChannelsString)},
                        {kPrevGain, juce::String(prevGainString)}}};

  return tree;
}

void ChannelGains::setTotalChannels(juce::int32 numChannels) {
  totalChannels_ = numChannels;
}

void ChannelGains::setChannelGain(const int& channel, const float& gain) {
  if (channel >= totalChannels_) {
    return;
  }
  gains_[channel] = gain;
}

void ChannelGains::setGains(std::vector<float> gains) {
  size_t n = std::min(gains.size(), gains_.size());
  for (int i = 0; i < n; ++i) {
    gains_[i] = gains[i];
  }
}

std::vector<std::string> ChannelGains::splitStringBySpace(
    const std::string& input) {
  std::istringstream iss(input);
  std::vector<std::string> result;
  std::string temp;
  while (iss >> temp) {
    result.push_back(temp);
  }
  return result;
}

std::vector<float> ChannelGains::convertStringsToFloats(
    const std::vector<std::string>& stringVec) {
  std::vector<float> floatVec;
  for (const auto& str : stringVec) {
    std::istringstream iss(str);
    float num;
    if (iss >> num) {  // Successfully converted to float
      floatVec.push_back(num);
    }
  }
  return floatVec;
}

std::vector<int> ChannelGains::convertStringsToInts(
    const std::vector<std::string>& stringVec) {
  std::vector<int> intVec;
  for (const auto& str : stringVec) {
    std::istringstream iss(str);
    int num;
    if (iss >> num) {  // Successfully converted to int
      intVec.push_back(num);
    }
  }
  return intVec;
}