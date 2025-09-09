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
#include <juce_dsp/juce_dsp.h>

#include <string>

#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/MixPresentationLoudnessRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "iamf/include/iamf_tools/iamf_encoder_interface.h"

struct AudioElementMetadata {
  int id;
  int firstChannel;
  int numChannels;
  std::vector<iamf_tools_cli_proto::ChannelLabel> channelLabels;

  AudioElementMetadata(
      int idIn, int firstChannelIn, int numChannelsIn,
      std::vector<iamf_tools_cli_proto::ChannelLabel> channelLabelsIn)
      : id(idIn),
        firstChannel(firstChannelIn),
        numChannels(numChannelsIn),
        channelLabels(channelLabelsIn) {}
};

class IAMFFileWriter {
 public:
  IAMFFileWriter(
      FileExportRepository& fileExportRepository,
      AudioElementRepository& audioElementRepository,
      MixPresentationRepository& mixPresentationRepository,
      MixPresentationLoudnessRepository& mixPresentationLoudnessRepository,
      int samplesPerFrame, int sampleRate);
  ~IAMFFileWriter() {};

  bool open(const std::string& filename);
  bool close();
  bool writeFrame(const juce::AudioBuffer<float>& buffer);

 private:
  void populateCodecInformationFromRepository(
      FileExportRepository& fileExportRepository,
      iamf_tools_cli_proto::UserMetadata& iamfMD);
  void populateAudioElementMetadataFromRepository(
      AudioElementRepository& audioElementRepository,
      iamf_tools_cli_proto::UserMetadata& iamfMD,
      std::unordered_map<juce::Uuid, int>& audioElementIDMap);
  void populateMixPresentationMetadataFromRepository(
      MixPresentationRepository& mixPresentationRepository,
      iamf_tools_cli_proto::UserMetadata& iamfMD,
      std::unordered_map<juce::Uuid, int>& audioElementIDMap);

  FileExportRepository& fileExportRepository_;
  AudioElementRepository& audioElementRepository_;
  MixPresentationRepository& mixPresentationRepository_;
  MixPresentationLoudnessRepository& mixPresentationLoudnessRepository_;

  std::unordered_map<juce::Uuid, int> audioElementIDMap_;
  int samplesPerFrame_;
  int sampleRate_;
  std::unique_ptr<iamf_tools_cli_proto::UserMetadata> userMetadata_;
  std::unique_ptr<iamf_tools::api::IamfEncoderInterface> iamfEncoder_;
  std::vector<AudioElementMetadata> audioElementInformation_;
  iamf_tools::api::IamfTemporalUnitData temporalUnitData_;
  juce::AudioBuffer<double> doubleBuffer_;
};
