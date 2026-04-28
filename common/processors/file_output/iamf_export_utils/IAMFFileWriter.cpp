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

#include <vector>

#include "IAMFExportUtil.h"
#include "data_structures/src/MixPresentation.h"
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
    MixPresentationRepository& mixPresentationRepository,
    iamf_tools_cli_proto::UserMetadata& iamfMD,
    std::unordered_map<juce::Uuid, int>& audioElementIDMap) {
  // Pull down audio elements from repository.
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository.getAll(audioElements);
  // Pull down mix presentations from repository.
  juce::OwnedArray<MixPresentation> mixPresentations;
  mixPresentationRepository.getAll(mixPresentations);

  // Filter audio elements to only those that are active in at least one mix
  // presentation.
  const std::vector<const AudioElement*> kActiveAudioElements =
      IAMFExportHelper::filterFreeAudioElements(audioElements,
                                                mixPresentations);

  // Clear any existing metadata.
  iamfMD.clear_audio_element_metadata();
  iamfMD.clear_audio_frame_metadata();
  audioElementInformation_.clear();

  // For each audio element, add and populate: audio_element_metadata and
  // audio_frame_metadata.
  int minAudioSubstreamForElement = 0;
  int firstAudioElementId = 500;
  bool isExtendedAudioElementPresent = false;
  for (const AudioElement* audioElement : kActiveAudioElements) {
    // Populate the metadata for this audio element
    auto aeMDToPopulate = iamfMD.add_audio_element_metadata();
    juce::Uuid elementID = audioElement->getId();
    audioElementIDMap[elementID] = ++firstAudioElementId;
    int aeID = audioElementIDMap[elementID];
    audioElement->populateIamfAudioElementMetadata(aeMDToPopulate, aeID,
                                                   minAudioSubstreamForElement);
    auto afMDToPopulate = iamfMD.add_audio_frame_metadata();
    audioElement->populateIamfAudioFrameMetadata(afMDToPopulate, aeID);

    // Populate the channel map information for encoding
    AudioElementMetadata aeMetadata(
        aeID, audioElement->getFirstChannel(), audioElement->getChannelCount(),
        audioElement->getChannelConfig().getIamfChannelLabels());
    audioElementInformation_.emplace_back(aeMetadata);
  }

  // Finally, write out the minimum profile level required for the audio
  // elements
  IAMFExportHelper::writeIASeqHdr(
      FileProfileHelper::minimumProfile(iamfMD.audio_element_metadata_size(),
                                        audioElements),
      iamfMD);
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
      audioElementRepository_, mixPresentationRepository_, *userMetadata_,
      audioElementIDMap_);
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

  // Opus requires exactly 20ms frames regardless of the DAW block size.
  // Compute the required frame size and prepare the accumulation buffer.
  const AudioCodec codec = fileExportRepository_.get().getAudioCodec();
  if (codec == AudioCodec::OPUS) {
    OpusAccum accum;
    switch (sampleRate_) {
      case 16000:
        accum.frameSize = 320;
        break;
      case 24000:
        accum.frameSize = 480;
        break;
      default:
        accum.frameSize = 960;
        break;
    }
    accum.buffer.setSize(totalChannels, accum.frameSize, false, true, false);
    accum.samplesAccumulated = 0;
    opusAccum_ = std::move(accum);
  } else {
    opusAccum_.reset();
  }

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

bool IAMFFileWriter::finalizeWriting() {
  if (iamfEncoder_ != nullptr) {
    // Flush any partially accumulated Opus frame by zero-padding to a full
    // frame. This is the correct behaviour per the Opus/IAMF spec for the
    // final packet of a stream.
    if (opusAccum_ && opusAccum_->samplesAccumulated > 0) {
      auto& accum = *opusAccum_;
      for (int ch = 0; ch < accum.buffer.getNumChannels(); ++ch) {
        accum.buffer.clear(ch, accum.samplesAccumulated,
                           accum.frameSize - accum.samplesAccumulated);
      }
      encodeBuffer(accum.buffer);
      accum.samplesAccumulated = 0;
    }

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
  }
  return true;
}

bool IAMFFileWriter::close() {
  bool result = finalizeWriting();
  // Always clear the encoder in order to release the file
  iamfEncoder_ = nullptr;
  return result;
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

bool IAMFFileWriter::encodeBuffer(const juce::AudioBuffer<double>& buf) {
  for (const auto& audioElement : audioElementInformation_) {
    auto& audioData =
        temporalUnitData_.audio_element_id_to_data[audioElement.id];

    for (int i = 0; i < audioElement.numChannels; ++i) {
      auto channelLabel = audioElement.channelLabels[i];
      iamf_tools_cli_proto::ChannelLabelMessage channelLabelMsg;
      channelLabelMsg.set_channel_label(channelLabel);
      audioData[channelLabelMsg.SerializeAsString()] = absl::Span<const double>(
          buf.getReadPointer(audioElement.firstChannel + i),
          buf.getNumSamples());
    }
  }

  auto res = iamfEncoder_->Encode(temporalUnitData_);
  if (!res.ok()) {
    LOG_WARNING(0, "Failed to encode temporal unit " + res.ToString());
  }

  std::vector<uint8_t> unused_temporal_unit_obus;
  res = iamfEncoder_->OutputTemporalUnit(unused_temporal_unit_obus);
  if (!res.ok()) {
    LOG_WARNING(0, "Failed to output temporal unit " + res.ToString());
    return false;
  }
  return true;
}

bool IAMFFileWriter::writeFrame(const juce::AudioBuffer<float>& buffer) {
  if (!iamfEncoder_->GeneratingTemporalUnits()) {
    return false;
  }

  convertFloatToDouble(buffer, doubleBuffer_);

  // Non-Opus codecs: encode the block directly — frame size matches block size.
  if (!opusAccum_) {
    return encodeBuffer(doubleBuffer_);
  }

  // Opus requires exactly opusAccum_->frameSize samples per encoded frame
  // regardless of the DAW block size. Accumulate until a full frame is ready.
  auto& accum = *opusAccum_;
  int srcOffset = 0;
  int srcRemaining = doubleBuffer_.getNumSamples();
  while (srcRemaining > 0) {
    const int toCopy =
        std::min(srcRemaining, accum.frameSize - accum.samplesAccumulated);
    for (int ch = 0; ch < doubleBuffer_.getNumChannels(); ++ch) {
      accum.buffer.copyFrom(ch, accum.samplesAccumulated, doubleBuffer_, ch,
                            srcOffset, toCopy);
    }
    accum.samplesAccumulated += toCopy;
    srcOffset += toCopy;
    srcRemaining -= toCopy;

    if (accum.samplesAccumulated == accum.frameSize) {
      if (!encodeBuffer(accum.buffer)) return false;
      accum.samplesAccumulated = 0;
    }
  }
  return true;
}