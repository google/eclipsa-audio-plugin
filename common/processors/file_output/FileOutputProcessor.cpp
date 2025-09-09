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

#include <string>

#include "data_structures/src/AudioElement.h"
#include "data_structures/src/FileExport.h"
#include "iamf_export_utils/IAMFExportUtil.h"

//==============================================================================
FileOutputProcessor::FileOutputProcessor(
    FileExportRepository& fileExportRepository,
    AudioElementRepository& audioElementRepository,
    MixPresentationRepository& mixPresentationRepository,
    MixPresentationLoudnessRepository& mixPresentationLoudnessRepository)
    : ProcessorBase(),
      performingRender_(false),
      fileExportRepository_(fileExportRepository),
      audioElementRepository_(audioElementRepository),
      mixPresentationRepository_(mixPresentationRepository),
      mixPresentationLoudnessRepository_(mixPresentationLoudnessRepository) {}

FileOutputProcessor::~FileOutputProcessor() {}

//==============================================================================
const juce::String FileOutputProcessor::getName() const {
  return {"FileOutput"};
}
//==============================================================================
void FileOutputProcessor::prepareToPlay(double sampleRate,
                                        int samplesPerBlock) {
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

void FileOutputProcessor::setNonRealtime(bool isNonRealtime) noexcept {
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

  for (int i = 0; i < iamfWavFileWriters_.size(); ++i) {
    iamfWavFileWriters_[i]->write(buffer);
  }

  iamfFileWriter_->writeFrame(buffer);
}

//==============================================================================
void FileOutputProcessor::initializeFileExport(FileExport& config) {
  LOG_ANALYTICS(0, "Beginning .iamf file export");
  performingRender_ = true;
  startTime_ = config.getStartTime();
  endTime_ = config.getEndTime();
  std::string exportFile = config.getExportFile().toStdString();

  // To create the IAMF file, create a list of all the audio element wav
  // files to be created
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository_.getAll(audioElements);
  iamfWavFileWriters_.clear();
  iamfWavFileWriters_.reserve(audioElements.size());
  for (int i = 0; i < audioElements.size(); i++) {
    juce::String wavFilePath =
        config.getExportFile() + "_audio_element_ " + juce::String(i) + ".wav";
    sampleRate_ = config.getSampleRate();

    iamfWavFileWriters_.emplace_back(new AudioElementFileWriter(
        wavFilePath, config.getSampleRate(), config.getBitDepth(),
        config.getAudioCodec(), *audioElements[i]));
  }
  sampleTally_ = 0;

  // Set the sample tally in the configuration for FLAC encoding
  config.setSampleTally(sampleTally_);
  fileExportRepository_.update(config);

  juce::String exportFilename = config.getExportFile();
  if (!exportFilename.endsWith(".iamf")) {
    // The IAMF export tool adds .iamf to the filename
    exportFilename = exportFilename + ".iamf";
  }

  // Clean up the file if it exists
  juce::File outputFile = juce::File(exportFilename);
  outputFile.deleteFile();

  // // Create an IAMF file writer to perform the file writing
  iamfFileWriter_ = std::make_unique<IAMFFileWriter>(
      fileExportRepository_, audioElementRepository_,
      mixPresentationRepository_, mixPresentationLoudnessRepository_,
      numSamples_, config.getSampleRate());

  // // Open the file for writing
  bool openSuccess = iamfFileWriter_->open(exportFilename.toStdString());
  if (!openSuccess) {
    LOG_ERROR(0, "IAMF File Writer: Failed to open file for writing: " +
                     exportFilename.toStdString());
    throw std::runtime_error("Failed to open IAMF file for writing");
  }
}

void FileOutputProcessor::closeFileExport(FileExport& config) {
  LOG_ANALYTICS(0, "closing writers and exporting IAMF file");
  // close the output file, since rendering is completed
  for (auto& writer : iamfWavFileWriters_) {
    writer->close();
  }

  bool exportIAMFSuccess = iamfFileWriter_->close();
  // If muxing is enabled and audio export was successful, mux the audio and
  // video files.
  if (exportIAMFSuccess && fileExportRepository_.get().getExportVideo()) {
    bool muxIAMFSuccess = IAMFExportHelper::muxIAMF(
        audioElementRepository_, mixPresentationRepository_,
        fileExportRepository_.get());

    if (!muxIAMFSuccess) {
      LOG_INFO(0, "IAMF Muxing: Failed to mux IAMF file with provided video.");
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
}

bool FileOutputProcessor::shouldBufferBeWritten(
    const juce::AudioBuffer<float>& buffer) {
  if (!performingRender_ || buffer.getNumSamples() < 1) {
    return false;
  }

  // Safety check to prevent division by zero during auval testing
  if (sampleRate_ <= 0) {
    return false;
  }

  // Calculate the current time with the existing number of samples that have
  // been processed
  long currentTime = sampleTally_ / sampleRate_;
  // update the sample tally
  sampleTally_ += buffer.getNumSamples();
  // with the updated sample tally, calculate the next time
  long nextTime = sampleTally_ / sampleRate_;

  if (startTime_ != 0 || endTime_ != 0) {
    // Handle the case where startTime and endTime are set, implying we
    // are only bouncing a subset of the mix

    // do not render
    if (currentTime < startTime_ || nextTime > endTime_) {
      return false;
    }
  }
  return true;
}
