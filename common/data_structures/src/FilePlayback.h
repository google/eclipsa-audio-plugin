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
#include <juce_data_structures/juce_data_structures.h>

#include "data_structures/src/RepositoryItem.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

// Using a macro here to help minimize the amount of code
//  If we need custom getters/setters we can
//  break this macro up into variable creation and getter/setter creation
//  and then only define the getter/setter when default getter/setters are
//  needed
#define EXPORT_VALUE(type, x, y)          \
 private:                                 \
  type x##_;                              \
                                          \
 public:                                  \
  void set##y(const type x) { x##_ = x; } \
  type get##y() const { return x##_; }    \
  inline static const juce::Identifier k##y{#x};

class FilePlayback final : public RepositoryItemBase {
 public:
  enum CurrentPlayerState { kDisabled, kBuffering, kPlay, kPause, kStop };

  FilePlayback();
  FilePlayback(int volume, CurrentPlayerState playState,
               juce::String playbackFile, float seekPosition,
               Speakers::AudioElementSpeakerLayout reqdDecodeLayout,
               juce::String playbackDevice = "");

  ~FilePlayback() = default;

  static FilePlayback fromTree(const juce::ValueTree tree);
  virtual juce::ValueTree toValueTree() const override;

  inline static const juce::Identifier kTreeType{"file_playback"};

 private:
  EXPORT_VALUE(int, volume, Volume);
  EXPORT_VALUE(CurrentPlayerState, playState, PlayState);
  EXPORT_VALUE(juce::String, playbackFile, PlaybackFile);
  EXPORT_VALUE(float, seekPosition, SeekPosition);
  EXPORT_VALUE(Speakers::AudioElementSpeakerLayout, reqdDecodeLayout,
               ReqdDecodeLayout);
  EXPORT_VALUE(juce::String, playbackDevice, PlaybackDevice);
};
