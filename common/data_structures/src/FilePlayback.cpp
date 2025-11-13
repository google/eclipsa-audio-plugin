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

#include "FilePlayback.h"

#include "substream_rdr/substream_rdr_utils/Speakers.h"

FilePlayback::FilePlayback()
    : RepositoryItemBase({}),
      volume_(0),
      playState_(CurrentPlayerState::kStop),
      playbackFile_(""),
      seekPosition_(0.0f),
      reqdDecodeLayout_(),
      playbackDevice_("") {}

FilePlayback::FilePlayback(int volume, CurrentPlayerState playState,
                           juce::String playbackFile, float seekPosition,
                           Speakers::AudioElementSpeakerLayout reqdDecodeLayout,
                           juce::String playbackDevice)
    : RepositoryItemBase({}),
      volume_(volume),
      playState_(playState),
      playbackFile_(playbackFile),
      seekPosition_(seekPosition),
      reqdDecodeLayout_(reqdDecodeLayout),
      playbackDevice_(playbackDevice) {}

FilePlayback FilePlayback::fromTree(const juce::ValueTree tree) {
  FilePlayback fpb;
  fpb.volume_ = tree[kVolume];
  fpb.playState_ = (CurrentPlayerState)(int)tree[kPlayState];
  fpb.playbackFile_ = tree[kPlaybackFile];
  fpb.seekPosition_ = tree[kSeekPosition];
  fpb.reqdDecodeLayout_ =
      (Speakers::AudioElementSpeakerLayout)(int)tree[kReqdDecodeLayout];
  fpb.playbackDevice_ = tree[kPlaybackDevice];
  return fpb;
}

juce::ValueTree FilePlayback::toValueTree() const {
  return {kTreeType,
          {{kVolume, volume_},
           {kPlayState, (int)playState_},
           {kPlaybackFile, playbackFile_},
           {kSeekPosition, seekPosition_},
           {kReqdDecodeLayout, (int)reqdDecodeLayout_},
           {kPlaybackDevice, playbackDevice_}}};
}