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
#include <bitset>

#include "data_structures/src/RepositoryItem.h"

class PlaybackMS final : public RepositoryItemBase {
 public:
  static const int kMaxNumPlaybackCh = 16;

  PlaybackMS();
  PlaybackMS(const std::bitset<kMaxNumPlaybackCh> mutedCh,
             const std::bitset<kMaxNumPlaybackCh> soloedCh);
  PlaybackMS(const juce::String mutedCh, const juce::String soloedCh);

  static PlaybackMS fromTree(const juce::ValueTree tree);
  virtual juce::ValueTree toValueTree() const override;

  std::bitset<kMaxNumPlaybackCh> getMutedChannels() const {
    return mutedChannels_;
  }
  std::bitset<kMaxNumPlaybackCh> getSoloedChannels() const {
    return soloedChannels_;
  }

  void setMutedChannels(const std::bitset<kMaxNumPlaybackCh> mutedChs) {
    mutedChannels_ = mutedChs;
  }
  void setSoloedChannels(const std::bitset<kMaxNumPlaybackCh> soloedChs) {
    soloedChannels_ = soloedChs;
  }

  void toggleMute(const int pos) { mutedChannels_.flip(pos); }
  void toggleSolo(const int pos) { soloedChannels_.flip(pos); }

  void reset() {
    mutedChannels_.reset();
    soloedChannels_.reset();
  }

  inline static const juce::Identifier kTreeType{"ms_playback"};
  inline static const juce::Identifier kMutedChannelsID{"muted_channels"};
  inline static const juce::Identifier kSoloedChannelsID{"soloed_channels"};

 private:
  std::bitset<kMaxNumPlaybackCh> mutedChannels_;
  std::bitset<kMaxNumPlaybackCh> soloedChannels_;
};