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

#include "FileOutputProcessor_PremierePro.h"

#include <logger/logger.h>

#include <string>
#include <unordered_map>  // Include the necessary header for HashTable

#include "data_structures/src/AudioElement.h"
#include "data_structures/src/FileExport.h"
#include "data_structures/src/MixPresentationLoudness.h"
#include "iamf_export_utils/IAMFExportUtil.h"
#include "processors/render/RenderProcessor.h"
#include "user_metadata.pb.h"

//==============================================================================
PremiereProFileOutputProcessor::PremiereProFileOutputProcessor(
    FileExportRepository& fileExportRepository,
    AudioElementRepository& audioElementRepository,
    MixPresentationRepository& mixPresentationRepository,
    MixPresentationLoudnessRepository& mixPresentationLoudnessRepository)
    : ProcessorBase(),
      performingRender_(false),
      fileExportRepository_(fileExportRepository),
      audioElementRepository_(audioElementRepository),
      mixPresentationRepository_(mixPresentationRepository),
      mixPresentationLoudnessRepository_(mixPresentationLoudnessRepository),
      estimatedSamplesToProcess_(0),
      processedSamples_(0) {}

PremiereProFileOutputProcessor::~PremiereProFileOutputProcessor() {}

//==============================================================================
const juce::String PremiereProFileOutputProcessor::getName() const {
  return {"FileOutput"};
}

void PremiereProFileOutputProcessor::initIamfMetadata(
    iamf_tools_cli_proto::UserMetadata& iamfMD,
    juce::String outputFileName) const {
  // NOTE: Leaving these test vector MD fields here for now as removal causes
  // silent export failure.
  iamfMD.mutable_test_vector_metadata()
      ->set_partition_mix_gain_parameter_blocks(false);
  iamfMD.mutable_test_vector_metadata()->set_file_name_prefix(
      outputFileName.toStdString());
}

void PremiereProFileOutputProcessor::updateIamfMDFromRepositories(
    iamf_tools_cli_proto::UserMetadata& iamfMD) {
  // From member repositories, update matching fields in the IAMF metadata.
  std::unordered_map<juce::Uuid, int>
      audioElementIDMap;  // Create a map of audio element UUID's to ints
  updateIamfMDFromRepository(fileExportRepository_, iamfMD);
  updateIamfMDFromRepository(audioElementRepository_, iamfMD,
                             audioElementIDMap);
  updateIamfMDFromRepository(mixPresentationRepository_, iamfMD,
                             audioElementIDMap);
}

void PremiereProFileOutputProcessor::updateIamfMDFromRepository(
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
          numSamples_, sampleTally_, fileExportData.getBitDepth(),
          fileExportData.getFlacCompressionLevel(), iamfMD);
      break;
    case AudioCodec::OPUS:
      IAMFExportHelper::writeOPUSConfigMD(
          sampleRate_, fileExportData.getOpusTotalBitrate(), iamfMD);
      break;
    case AudioCodec::LPCM:
    default:
      IAMFExportHelper::writeLPCMConfigMD(
          numSamples_, sampleTally_, sampleRate_,
          fileExportData.getLPCMSampleSize(), iamfMD);
      break;
  }
}

void PremiereProFileOutputProcessor::updateIamfMDFromRepository(
    AudioElementRepository& audioElementRepository,
    iamf_tools_cli_proto::UserMetadata& iamfMD,
    std::unordered_map<juce::Uuid, int>& audioElementIDMap) {
  // Pull down audio elements from repository.
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository.getAll(audioElements);

  // Clear any existing metadata.
  iamfMD.clear_audio_element_metadata();
  iamfMD.clear_audio_frame_metadata();

  // For each audio element, add and populate: audio_element_metadata and
  // audio_frame_metadata.
  int minAudioSubstreamForElement = 0;
  int firstAudioElementId = 500;
  for (int i = 0; i < iamfWavFileWriters_.size(); ++i) {
    auto aeMDToPopulate = iamfMD.add_audio_element_metadata();
    juce::Uuid elementID = iamfWavFileWriters_[i].get()->getElement().getId();
    audioElementIDMap[elementID] = ++firstAudioElementId;
    int aeID = audioElementIDMap[elementID];
    iamfWavFileWriters_[i].get()->getElement().populateIamfAudioElementMetadata(
        aeMDToPopulate, aeID, minAudioSubstreamForElement);
    auto afMDToPopulate = iamfMD.add_audio_frame_metadata();
    iamfWavFileWriters_[i].get()->getElement().populateIamfAudioFrameMetadata(
        afMDToPopulate, aeID, iamfWavFileWriters_[i].get()->getFilePath(),
        iamfWavFileWriters_[i].get()->getFramesWritten());
  }
}

void PremiereProFileOutputProcessor::updateIamfMDFromRepository(
    MixPresentationRepository& mixPresentationRepository,
    iamf_tools_cli_proto::UserMetadata& iamfMD,
    std::unordered_map<juce::Uuid, int>& audioElementIDMap) {
  // Pull down mix presentations from repository.
  juce::OwnedArray<MixPresentation> mixPresentations;
  // jassert(mixPresentationRepository.getItemCount() != 0);
  mixPresentationRepository.getAll(mixPresentations);

  // Clear any existing mix_presentation_metadata.
  iamfMD.clear_mix_presentation_metadata();

  // For each mix presentation, add and populate: mix_presentation_metadata.
  for (int i = 0; i < mixPresentations.size(); ++i) {
    const juce::Uuid mixPresentationId = mixPresentations[i]->getId();
    MixPresentationLoudness mixPresentationLoudness =
        mixPresentationLoudnessRepository_.get(mixPresentationId).value();
    auto mpMDToPopulate = iamfMD.add_mix_presentation_metadata();
    mixPresentations[i]->populateIamfMixPresentationMetadata(
        i, sampleRate_, sampleTally_, mpMDToPopulate, iamfMD,
        mixPresentationLoudness, audioElementIDMap);
  }
}
//==============================================================================
void PremiereProFileOutputProcessor::prepareToPlay(double sampleRate,
                                                   int samplesPerBlock) {
  FileExport configParams = fileExportRepository_.get();
  configParams.setSampleRate(sampleRate);
  fileExportRepository_.update(configParams);
  numSamples_ = samplesPerBlock;
  sampleTally_ = 0;
  sampleRate_ = sampleRate;

  FileExport config = fileExportRepository_.get();

  processedSamples_ = 0;

  int totalDuration = config.getEndTime() - config.getStartTime();

  estimatedSamplesToProcess_ = static_cast<int>(totalDuration * sampleRate);
  LOG_ANALYTICS(0, "FileOutput PremierePro, totalDuration: " +
                       std::to_string(totalDuration) +
                       ", Estimated samples to process: " +
                       std::to_string(estimatedSamplesToProcess_) + "\n");
}

void PremiereProFileOutputProcessor::setNonRealtime(
    bool isNonRealtime) noexcept {
  LOG_ANALYTICS(0, std::string("File Output Premiere Pro Set Non-Realtime ") +
                       (isNonRealtime ? "true" : "false"));
  // Initialize the writer if we are rendering in offline mode
  if (isNonRealtime && !performingRender_ && !exportCompleted_) {
    FileExport config = fileExportRepository_.get();
    if ((config.getAudioFileFormat() == AudioFileFormat::IAMF) &&
        (config.getExportAudio())) {
      performingRender_ = true;
      startTime_ = config.getStartTime();
      endTime_ = config.getEndTime();

      // To create the IAMF file, create a list of all the audio element wav
      // files to be created
      LOG_ANALYTICS(0, "FileOutput PremierePro, Beginning .iamf file export");
      juce::OwnedArray<AudioElement> audioElements;
      audioElementRepository_.getAll(audioElements);
      iamfWavFileWriters_.clear();
      iamfWavFileWriters_.reserve(audioElements.size());
      for (int i = 0; i < audioElements.size(); i++) {
        juce::String wavFilePath = config.getExportFile() + "_audio_element_ " +
                                   juce::String(i) + ".wav";
        sampleRate_ = config.getSampleRate();

        iamfWavFileWriters_.emplace_back(new AudioElementFileWriter(
            wavFilePath, config.getSampleRate(), config.getBitDepth(),
            config.getAudioCodec(), *audioElements[i]));
      }
      sampleTally_ = 0;
    }
    return;
  }

  // Stop rendering if we are switching back to online mode
  if (performingRender_ && !isNonRealtime && exportCompleted_) {
    LOG_ANALYTICS(0, "closing writers and exporting IAMF file");
    FileExport config = fileExportRepository_.get();
    // close the output file, since rendering is completed
    for (auto& writer : iamfWavFileWriters_) {
      writer->close();
    }
    juce::File outputFile = juce::File(config.getExportFile());
    outputFile.deleteFile();

    bool exportIAMFSuccess =
        exportIamfFile(config.getExportFolder(), config.getExportFolder());

    // If muxing is enabled and audio export was successful, mux the audio and
    // video files.
    if (exportIAMFSuccess && fileExportRepository_.get().getExportVideo()) {
      bool muxIAMFSuccess = IAMFExportHelper::muxIAMF(
          audioElementRepository_, mixPresentationRepository_,
          fileExportRepository_.get());

      if (!muxIAMFSuccess) {
        LOG_INFO(0,
                 "IAMF Muxing: Failed to mux IAMF file with provided video.");
      }
    }

    if (!config.getExportAudioElements()) {
      // Delete the extraneuos audio element files
      for (auto& writer : iamfWavFileWriters_) {
        juce::File audioElementFile(writer->getFilePath());
        audioElementFile.deleteFile();
      }
    }
    iamfWavFileWriters_.clear();
    performingRender_ = false;
    exportCompleted_ = false;
  }
}

void PremiereProFileOutputProcessor::dumpExportLogs(
    const absl::Status& status) const {
  juce::OwnedArray<AudioElement> audioElements;
  juce::OwnedArray<MixPresentation> mixPresentations;
  audioElementRepository_.getAll(audioElements);
  mixPresentationRepository_.getAll(mixPresentations);

  std::vector<std::string> audioElementInfo;
  std::vector<std::string> mixPresInfo;

  // Format Audio Element and Mix Presentation data for logging.
  for (const AudioElement* ae : audioElements) {
    juce::String aeStr("Audio Element - " + ae->getId().toString() + " " +
                       ae->getDescription());
    audioElementInfo.push_back(aeStr.toStdString());
  }
  for (const MixPresentation* mp : mixPresentations) {
    juce::String aeStr("Mix Presentation - " + mp->getId().toString() + " " +
                       mp->getName() + " " +
                       juce::String(mp->getDefaultMixGain()));
    mixPresInfo.push_back(aeStr.toStdString());
  }

  // Dump formatted Audio Element and Mix Presentation data.
  LOG_INFO(0, "IAMF Export: Export attempted with:\n");
  for (const std::string& ae : audioElementInfo) {
    LOG_INFO(0, ae + "\n");
  }
  for (const std::string& mp : mixPresInfo) {
    LOG_INFO(0, mp + "\n");
  }
  LOG_INFO(0, "IAMF Export: IAMF export attempt completed with status: " +
                  status.ToString());
}

bool PremiereProFileOutputProcessor::exportIamfFile(
    const juce::String input_wav_path, const juce::String output_iamf_path) {
  iamf_tools_cli_proto::UserMetadata iamfMetadata{};

  juce::String exportFilename = fileExportRepository_.get().getExportFile();
  if (exportFilename.endsWith(".iamf")) {
    // The IAMF export tool adds .iamf to the filename
    exportFilename = exportFilename.upToLastOccurrenceOf(".iamf", false, true);
  }
  initIamfMetadata(iamfMetadata, exportFilename);

  updateIamfMDFromRepositories(iamfMetadata);

  LOG_INFO(0, "Exporting IAMF File with Metadata");
  LOG_INFO(0, iamfMetadata.DebugString());

  auto res = iamf_tools::TestMain(iamfMetadata, input_wav_path.toStdString(),
                                  output_iamf_path.toStdString());

  dumpExportLogs(res);

  return res.ok();
}

void PremiereProFileOutputProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  if (!performingRender_ || buffer.getNumSamples() < 1) {
    return;
  }

  long currentTime = sampleTally_ / sampleRate_;
  long nextTime = (sampleTally_ + buffer.getNumSamples()) / sampleRate_;

  if (currentTime < startTime_ || nextTime > endTime_) {
    // Do not write to the wav file if we are outside the start and end time
    return;
  }

  if (processedSamples_ <= estimatedSamplesToProcess_) {
    LOG_ANALYTICS(
        0, std::string(
               "PremierePro FileOutput process block is performRendering"));
    LOG_ANALYTICS(0, "Processing an additional " +
                         std::to_string(buffer.getNumSamples()) +
                         " samples. Already processed " +
                         std::to_string(processedSamples_) + " of " +
                         std::to_string(estimatedSamplesToProcess_));
    processedSamples_ += buffer.getNumSamples();

    // Write the audio data to the wav file writers
    for (auto& writer : iamfWavFileWriters_) {
      writer->write(buffer);
    }
  } else {
    exportCompleted_ = true;
  }
}

//==============================================================================
bool PremiereProFileOutputProcessor::hasEditor() const { return false; }

juce::AudioProcessorEditor* PremiereProFileOutputProcessor::createEditor() {
  return nullptr;
}
