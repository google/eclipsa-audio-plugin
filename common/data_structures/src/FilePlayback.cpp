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
      playbackFile_(""),
      reqdDecodeLayout_(Speakers::kStereo),
      seekPosition_(0.0f),
      volume_(1.0f),
      playbackCommand_(PlaybackCommand::kStop) {}

FilePlayback::FilePlayback(
    const juce::String playbackFile,
    const Speakers::AudioElementSpeakerLayout reqdDecodeLayout,
    const float seekPosition, const float volume,
    const PlaybackCommand processorCommand)
    : RepositoryItemBase({}),
      playbackFile_(playbackFile),
      reqdDecodeLayout_(reqdDecodeLayout),
      seekPosition_(seekPosition),
      volume_(volume),
      playbackCommand_(processorCommand) {}

FilePlayback FilePlayback::fromTree(const juce::ValueTree tree) {
  return FilePlayback(
      tree[kPlaybackFile],
      static_cast<Speakers::AudioElementSpeakerLayout>(
          (int)tree[kReqdDecodeLayout]),
      (float)tree[kSeekPosition], (float)tree[kVolume],
      static_cast<PlaybackCommand>((int)tree[kPlaybackCommand]));
}

juce::ValueTree FilePlayback::toValueTree() const {
  return {kTreeType,
          {
              {kPlaybackCommand, (int)playbackCommand_},
              {kPlaybackFile, playbackFile_},
              {kReqdDecodeLayout, (int)reqdDecodeLayout_},
              {kSeekPosition, seekPosition_},
              {kVolume, volume_},
          }};
}