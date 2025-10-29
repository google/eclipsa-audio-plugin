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

#pragma once

#include <filesystem>

#include "data_repository/implementation/FilePlaybackRepository.h"
#include "data_structures/src/FilePlayback.h"
#include "player/src/transport/IAMFDecoderSource.h"
#include "processors/file_output/iamf_export_utils/IAMFFileReader.h"

class IAMFPlaybackDevice : private juce::ValueTree::Listener {
 public:
  IAMFPlaybackDevice(const std::filesystem::path iamfPath,
                     const juce::String pbDeviceName,
                     FilePlaybackRepository& filePlaybackRepo,
                     juce::AudioDeviceManager& deviceManager);
  ~IAMFPlaybackDevice();

  void play();
  void pause();
  void stop();
  void seekTo(const float position);
  void configurePlaybackDevice(const juce::String deviceName);
  void configureDecodeLayout(const Speakers::AudioElementSpeakerLayout layout);
  void setVolume(const float volume);
  IAMFFileReader::StreamData getStreamData() const;

 private:
  struct PlaybackState {
    bool wasPlaying;
    FilePlayback::CurrentPlayerState state;
  };
  void valueTreePropertyChanged(juce::ValueTree& tree,
                                const juce::Identifier& property) override;
  // Various helper methods
  void setRepoState(const FilePlayback::CurrentPlayerState state);
  void setPlayerSource();
  // Playback configuration helper methods
  PlaybackState capturePbState() const;
  juce::AudioDeviceManager::AudioDeviceSetup setupAudioDevice(
      const juce::String& deviceName,
      const IAMFFileReader::StreamData& streamData, bool isInitialSetup);
  void updateResampler(unsigned sourceSampleRate, unsigned deviceSampleRate,
                       unsigned numChannels);

  const std::filesystem::path kPath_;
  juce::AudioDeviceManager& deviceManager_;
  FilePlaybackRepository& fpbr_;
  std::unique_ptr<IAMFDecoderSource> decoderSource_;
  std::unique_ptr<juce::ResamplingAudioSource> resampler_;
  juce::AudioSourcePlayer sourcePlayer_;
};