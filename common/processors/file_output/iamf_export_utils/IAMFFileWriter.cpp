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

#include "IAMFFileWriter.h"

#include "IAMFExportUtil.h"
#include "iamf/include/iamf_tools/iamf_encoder_factory.h"

IAMFFileWriter::IAMFFileWriter(
    FileExportRepository& fileExportRepository,
    AudioElementRepository& audioElementRepository,
    MixPresentationRepository& mixPresentationRepository,
    MixPresentationLoudnessRepository& mixPresentationLoudnessRepository,
    int samplesPerFrame, int sampleRate)
    : fileExportRepository_(fileExportRepository),
      audioElementRepository_(audioElementRepository),
      mixPresentationRepository_(mixPresentationRepository),
      mixPresentationLoudnessRepository_(mixPresentationLoudnessRepository),
      samplesPerFrame_(samplesPerFrame),
      sampleRate_(sampleRate) {}

void IAMFFileWriter::populateCodecInformationFromRepository(
    FileExportRepository& fileExportRepository,
    iamf_tools_cli_proto::UserMetadata& iamfMD) {
  // Pull down file export data from repository
  FileExport fileExportData = fileExportRepository.get();

  iamfMD.clear_codec_config_metadata();
  iamfMD.clear_ia_sequence_header_metadata();

  IAMFExportHelper::writeIASeqHdr(fileExportData.getProfile(), iamfMD);

  switch (fileExportData.getAudioCodec()) {
    case AudioCodec::FLAC:
      IAMFExportHelper::writeFLACConfigMD(
          samplesPerFrame_, fileExportData.getSampleTally(),
          fileExportData.getBitDepth(),
          fileExportData.getFlacCompressionLevel(), sampleRate_, iamfMD);
      break;
    case AudioCodec::OPUS:
      IAMFExportHelper::writeOPUSConfigMD(
          sampleRate_, fileExportData.getOpusTotalBitrate(), iamfMD);
      break;
    case AudioCodec::LPCM:
    default:
      IAMFExportHelper::writeLPCMConfigMD(samplesPerFrame_, sampleRate_,
                                          fileExportData.getLPCMSampleSize(),
                                          iamfMD);
      break;
  }
}

void IAMFFileWriter::populateAudioElementMetadataFromRepository(
    AudioElementRepository& audioElementRepository,
    iamf_tools_cli_proto::UserMetadata& iamfMD,
    std::unordered_map<juce::Uuid, int>& audioElementIDMap) {
  // Pull down audio elements from repository.
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository.getAll(audioElements);

  // Clear any existing metadata.
  iamfMD.clear_audio_element_metadata();
  iamfMD.clear_audio_frame_metadata();
  audioElementInformation_.clear();

  // For each audio element, add and populate: audio_element_metadata and
  // audio_frame_metadata.
  int minAudioSubstreamForElement = 0;
  int firstAudioElementId = 500;
  for (int i = 0; i < audioElements.size(); ++i) {
    // Populate the metadata for this audio element
    auto aeMDToPopulate = iamfMD.add_audio_element_metadata();
    juce::Uuid elementID = audioElements[i]->getId();
    audioElementIDMap[elementID] = ++firstAudioElementId;
    int aeID = audioElementIDMap[elementID];
    audioElements[i]->populateIamfAudioElementMetadata(
        aeMDToPopulate, aeID, minAudioSubstreamForElement);
    auto afMDToPopulate = iamfMD.add_audio_frame_metadata();
    audioElements[i]->populateIamfAudioFrameMetadata(afMDToPopulate, aeID);

    // Populate the channel map information for encoding
    AudioElementMetadata aeMetadata(
        aeID, audioElements[i]->getFirstChannel(),
        audioElements[i]->getChannelCount(),
        audioElements[i]->getChannelConfig().getIamfChannelLabels());
    audioElementInformation_.emplace_back(aeMetadata);
  }
}

void IAMFFileWriter::populateMixPresentationMetadataFromRepository(
    MixPresentationRepository& mixPresentationRepository,
    iamf_tools_cli_proto::UserMetadata& iamfMD,
    std::unordered_map<juce::Uuid, int>& audioElementIDMap) {
  // Pull down mix presentations from repository.
  juce::OwnedArray<MixPresentation> mixPresentations;
  mixPresentationRepository.getAll(mixPresentations);

  // Clear any existing mix_presentation_metadata.
  iamfMD.clear_mix_presentation_metadata();

  // For each mix presentation, add and populate the mix_presentation_metadata.
  for (int i = 0; i < mixPresentations.size(); ++i) {
    const juce::Uuid mixPresentationId = mixPresentations[i]->getId();
    MixPresentationLoudness mixPresentationLoudness =
        mixPresentationLoudnessRepository_.get(mixPresentationId).value();
    auto mpMDToPopulate = iamfMD.add_mix_presentation_metadata();
    mixPresentations[i]->populateIamfMixPresentationMetadata(
        i, sampleRate_, mpMDToPopulate, iamfMD, mixPresentationLoudness,
        audioElementIDMap);
  }
}

bool IAMFFileWriter::open(const std::string& filename) {
  // Create a new instance of the user metadata to use
  userMetadata_ = std::make_unique<iamf_tools_cli_proto::UserMetadata>();

  // Configure the user metadata from the repositories
  audioElementIDMap_.clear();
  populateCodecInformationFromRepository(fileExportRepository_, *userMetadata_);
  populateAudioElementMetadataFromRepository(
      audioElementRepository_, *userMetadata_, audioElementIDMap_);
  populateMixPresentationMetadataFromRepository(
      mixPresentationRepository_, *userMetadata_, audioElementIDMap_);

  // Create an encoder instance
  auto localIamfEncoder =
      iamf_tools::api::IamfEncoderFactory::CreateFileGeneratingIamfEncoder(
          userMetadata_->SerializeAsString(), filename);
  if (!localIamfEncoder.ok()) {
    iamfEncoder_ = std::nullptr_t();
    return false;
  }

  // Globalize it for other methods after validating the absl return
  iamfEncoder_ = std::move(localIamfEncoder.value());

  // Configure the temporal unit data structure for later use
  temporalUnitData_ = iamf_tools::api::IamfTemporalUnitData();

  // Calculate total channels needed and initialize double buffer for frame
  // writing
  int totalChannels = 0;
  for (const auto& audioElement : audioElementInformation_) {
    totalChannels += audioElement.numChannels;
  }
  doubleBuffer_.setSize(totalChannels, samplesPerFrame_, false, false, true);

  // Initialize temporal unit data map entries
  for (const auto& audioElement : audioElementInformation_) {
    auto& audioData =
        temporalUnitData_.audio_element_id_to_data[audioElement.id];
    for (int i = 0; i < audioElement.numChannels; ++i) {
      auto channelLabel = audioElement.channelLabels[i];
      iamf_tools_cli_proto::ChannelLabelMessage channelLabelMsg;
      channelLabelMsg.set_channel_label(channelLabel);
      audioData[channelLabelMsg.SerializeAsString()];  // Initialize the map
                                                       // entry
    }
  }
  return true;
}

bool IAMFFileWriter::close() {
  if (iamfEncoder_ != nullptr) {
    // Step 1: Finalize the encoding process
    auto finalizeStatus = iamfEncoder_->FinalizeEncode();
    if (!finalizeStatus.ok()) {
      LOG_WARNING(1,
                  "Failed to finalize encoder: " + finalizeStatus.ToString());
      return false;
    }

    // Step 2: Flush all remaining temporal units
    while (iamfEncoder_->GeneratingTemporalUnits()) {
      std::vector<uint8_t> unused_temporal_unit_obus;
      auto res = iamfEncoder_->OutputTemporalUnit(unused_temporal_unit_obus);
      if (!res.ok()) {
        LOG_WARNING(
            1, "Failed to flush remaining temporal units: " + res.ToString());
        return false;
      }
    }

    // Step 3: Get final descriptors (these contain loudness info, etc)
    bool redundant_copy = false;
    bool output_obus_are_finalized;
    std::vector<uint8_t> descriptor_obus;
    auto res = iamfEncoder_->GetDescriptorObus(redundant_copy, descriptor_obus,
                                               output_obus_are_finalized);
    if (!res.ok()) {
      LOG_WARNING(1, "Failed to get final descriptor OBUs: " + res.ToString());
      return false;
    }

    if (!output_obus_are_finalized) {
      LOG_WARNING(1, "Final descriptor OBUs were not properly finalized");
      return false;
    }

    // Step 4: Sanity check that we're done generating all temporal units
    if (iamfEncoder_->GeneratingTemporalUnits()) {
      LOG_WARNING(1,
                  "Encoder still generating temporal units after finalization");
      return false;
    }

    // Step 5: Reset the encoder only after confirming everything is finalized
    iamfEncoder_.reset(nullptr);
  }
  return true;
}

inline void convertFloatToDouble(const juce::AudioBuffer<float>& src,
                                 juce::AudioBuffer<double>& dst) {
  const int numChannels = src.getNumChannels();
  const int numSamples = src.getNumSamples();

  dst.setSize(numChannels, numSamples, false, false, true);

  for (int ch = 0; ch < numChannels; ++ch) {
    const float* srcPtr = src.getReadPointer(ch);
    double* dstPtr = dst.getWritePointer(ch);

    std::transform(srcPtr, srcPtr + numSamples, dstPtr,
                   [](float s) { return static_cast<double>(s); });
  }
}

bool IAMFFileWriter::writeFrame(const juce::AudioBuffer<float>& buffer) {
  // First, ensure generation is enabled
  if (!iamfEncoder_->GeneratingTemporalUnits()) {
    return false;
  }

  convertFloatToDouble(buffer, doubleBuffer_);

  // Fill in the temporal unit data with the current frame's audio data
  for (const auto& audioElement : audioElementInformation_) {
    auto& audioData =
        temporalUnitData_.audio_element_id_to_data[audioElement.id];

    for (int i = 0; i < audioElement.numChannels; ++i) {
      auto channelLabel = audioElement.channelLabels[i];
      iamf_tools_cli_proto::ChannelLabelMessage channelLabelMsg;
      channelLabelMsg.set_channel_label(channelLabel);
      audioData[channelLabelMsg.SerializeAsString()] = absl::Span<const double>(
          doubleBuffer_.getReadPointer(audioElement.firstChannel + i),
          doubleBuffer_.getNumSamples());
    }
  }
  // Encode the temporal unit data
  auto res = iamfEncoder_->Encode(temporalUnitData_);
  if (!res.ok()) {
    LOG_WARNING(0, "Failed to encode temporal unit " + res.ToString());
  }

  // Output the temporal units
  std::vector<uint8_t> unused_temporal_unit_obus;
  res = iamfEncoder_->OutputTemporalUnit(unused_temporal_unit_obus);
  if (!res.ok()) {
    LOG_WARNING(0, "Failed to output temporal unit " + res.ToString());
    return false;
  }
  return true;
}