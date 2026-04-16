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

#include "FileOutputProcessor.h"

#include <logger/logger.h>

#include <filesystem>
#include <string>

#include "data_repository/implementation/FilePlaybackRepository.h"
#include "data_structures/src/AudioElement.h"
#include "data_structures/src/FileExport.h"
#include "data_structures/src/FilePlayback.h"
#include "iamf_export_utils/IAMFExportUtil.h"

//==============================================================================
FileOutputProcessor::FileOutputProcessor(
    FileExportRepository& fileExportRepository,
    FilePlaybackRepository& filePlaybackRepository,
    AudioElementRepository& audioElementRepository,
    MixPresentationRepository& mixPresentationRepository,
    MixPresentationLoudnessRepository& mixPresentationLoudnessRepository)
    : ProcessorBase(),
      performingRender_(false),
      fileExportRepository_(fileExportRepository),
      fpbr_(filePlaybackRepository),
      audioElementRepository_(audioElementRepository),
      mixPresentationRepository_(mixPresentationRepository),
      mixPresentationLoudnessRepository_(mixPresentationLoudnessRepository) {}

FileOutputProcessor::~FileOutputProcessor() = default;

//==============================================================================
const juce::String FileOutputProcessor::getName() const {
  return {"FileOutput"};
}
//==============================================================================
void FileOutputProcessor::prepareToPlay(const double sampleRate,
                                        const int samplesPerBlock) {
  FileExport configParams = fileExportRepository_.get();
  if (configParams.getSampleRate() != sampleRate) {
    LOG_ANALYTICS(0, "FileOutputProcessor sample rate changed to " +
                         std::to_string(sampleRate));
    configParams.setSampleRate(sampleRate);
    fileExportRepository_.update(configParams);
  }
  numSamples_ = samplesPerBlock;
  sampleTally_ = 0;
  sampleRate_ = sampleRate;
}

void FileOutputProcessor::setNonRealtime(const bool isNonRealtime) noexcept {
  // Bouncing in DAW and currently rendering || not bouncing and not rendering
  if (isNonRealtime == performingRender_) {
    return;
  }

  FileExport config = fileExportRepository_.get();
  // Initialize the writer if we are rendering in offline mode
  if (!performingRender_) {
    if ((config.getAudioFileFormat() == AudioFileFormat::IAMF) &&
        (config.getExportAudio())) {
      initializeFileExport(config);
    }
    return;
  }

  // Stop rendering if we are switching back to online mode
  if (performingRender_) {
    closeFileExport(config);
    performingRender_ = false;
  }
}

void FileOutputProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);

  if (!(shouldBufferBeWritten(buffer))) {
    // If we are not performing a render or the buffer is empty, do not write
    return;
  }

  // Process audio elements individually as Wav files
  for (int i = 0; i < iamfWavFileWriters_.size(); ++i) {
    iamfWavFileWriters_[i]->write(buffer);
  }

  // Process IAMF File
  if (iamfFileWriter_) {
    iamfFileWriter_->writeFrame(buffer);
  }
}

//==============================================================================
void FileOutputProcessor::initializeFileExport(FileExport& config) {
  LOG_ANALYTICS(0, "Beginning .iamf file export");
  performingRender_ = true;
  startSampleIdx_ = config.getStartSampleIdx();
  endSampleIdx_ = config.getEndSampleIdx();
  std::string exportFile = config.getExportFile().toStdString();

  // To create the IAMF file, create a list of all the audio element wav
  // files to be created
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository_.getAll(audioElements);
  iamfWavFileWriters_.clear();
  iamfWavFileWriters_.reserve(audioElements.size());
  for (int i = 0; i < audioElements.size(); i++) {
    const juce::String kElemName = audioElements[i]->getName();
    const juce::String kWavFilePath =
        config.getExportFile() + "_" + kElemName + ".wav";
    sampleRate_ = config.getSampleRate();

    iamfWavFileWriters_.emplace_back(new AudioElementFileWriter(
        kWavFilePath, config.getSampleRate(), config.getBitDepth(),
        config.getAudioCodec(), *audioElements[i]));
  }
  sampleTally_ = 0;

  // Set the sample tally in the configuration for FLAC encoding
  config.setSampleTally(sampleTally_);
  fileExportRepository_.update(config);
  // Reset the playback processor to stop any ongoing playback
  FilePlayback fpb = fpbr_.get();
  fpb.setPlaybackFile("");
  fpb.setPlaybackCommand(FilePlayback::PlaybackCommand::kPause);
  fpbr_.update(fpb);

  iamfFileWriter_ = nullptr;
  const juce::String kIamfPath = config.getExportFile();
  if (FileExport::validateFilePath(
          FileExport::expandTildePath(kIamfPath).toStdString(), false)) {
    // Create an IAMF file writer to perform the file writing
    iamfFileWriter_ = std::make_unique<IAMFFileWriter>(
        fileExportRepository_, audioElementRepository_,
        mixPresentationRepository_, mixPresentationLoudnessRepository_,
        numSamples_, config.getSampleRate());

    // Open the file for writing
    bool openSuccess = iamfFileWriter_->open(kIamfPath.toStdString());
    if (!openSuccess) {
      iamfFileWriter_ = nullptr;
      LOG_ERROR(0, "IAMF File Writer: Failed to open file for writing: " +
                       kIamfPath.toStdString());
    }
  } else {
    LOG_WARNING(
        0, "FileOutputProcessor: Cannot write IAMF data to an invalid path.");
  }
}

void FileOutputProcessor::closeFileExport(const FileExport& config) {
  LOG_ANALYTICS(0, "closing writers and exporting IAMF file");
  // close the output file, since rendering is completed
  for (const auto& writer : iamfWavFileWriters_) {
    writer->close();
  }

  // If muxing is enabled and audio export was successful, mux the audio and
  // video files.
  const bool kIamfExported = iamfFileWriter_ ? iamfFileWriter_->close() : false;
  if (kIamfExported && fileExportRepository_.get().getExportVideo()) {
    const bool kMuxIamfSuccess =
        IAMFExportHelper::muxIAMF(fileExportRepository_.get());

    if (!kMuxIamfSuccess) {
      LOG_WARNING(0,
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

  // Mark export as completed
  const FileExport kExport = fileExportRepository_.get();
  FilePlayback fpb = fpbr_.get();
  fpb.setPlaybackFile(kExport.getExportFile());
  fpb.setPlaybackCommand(FilePlayback::PlaybackCommand::kPause);
  fpbr_.update(fpb);
}

bool FileOutputProcessor::shouldBufferBeWritten(
    const juce::AudioBuffer<float>& buffer) {
  if (!performingRender_ || buffer.getNumSamples() < 1) {
    return false;
  }

  const long currentSample = sampleTally_;
  sampleTally_ += buffer.getNumSamples();

  // No range specified — write everything
  if (startSampleIdx_ <= 0 && endSampleIdx_ <= 0) {
    return true;
  }

  // Skip if buffer starts before the requested start sample
  if (startSampleIdx_ > 0 && currentSample < startSampleIdx_) {
    return false;
  }

  // Skip if buffer starts at or past the requested end sample
  if (endSampleIdx_ > 0 && currentSample >= endSampleIdx_) {
    return false;
  }

  return true;
}
