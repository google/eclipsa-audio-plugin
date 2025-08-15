/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License,
 * Version 2.0 (the "License");
 * you may not use this file except in
 * compliance with the License.
 * You may obtain a copy of the License at
 *
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by
 * applicable law or agreed to in writing, software
 * distributed under the
 * License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the
 * specific language governing permissions and
 * limitations under the
 * License.
 */

#pragma once
#include <data_repository/data_repository.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <memory>

#include "../processor_base/ProcessorBase.h"
#include "AudioElementFileWriter.h"
#include "data_repository/implementation/MixPresentationLoudnessRepository.h"
#include "iamf/include/iamf_tools/encoder_main_lib.h"
#include "iamf/include/iamf_tools/iamf_encoder_factory.h"
#include "user_metadata.pb.h"

//==============================================================================
class FileOutputProcessor : public ProcessorBase {
 public:
  //==============================================================================
  FileOutputProcessor(
      FileExportRepository& fileExportRepository,
      AudioElementRepository& audioElementRepository,
      MixPresentationRepository& mixPresentationRepository,
      MixPresentationLoudnessRepository& mixPresentationLoudnessRepository);
  ~FileOutputProcessor() override;

  //==============================================================================
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  using AudioProcessor::processBlock;

  void setNonRealtime(bool isNonRealtime) noexcept override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;

  //==============================================================================
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  // Update the default IAMF metadata from repository sources.
  void updateIamfMDFromRepositories(iamf_tools_cli_proto::UserMetadata& iamfMD);

  // From the FileExportRepository, updates:
  // codec_config_metadata.
  void updateIamfMDFromRepository(FileExportRepository& fileExportRepository,
                                  iamf_tools_cli_proto::UserMetadata& iamfMD);

  // From the AudioElementRepository, updates:
  // audio_element_metadata, audio_frame_metadata.
  void updateIamfMDFromRepository(
      AudioElementRepository& audioElementRepository,
      iamf_tools_cli_proto::UserMetadata& iamfMD,
      std::unordered_map<juce::Uuid, int>& audioElementIDMap);

  // From the MixPresentationRepository, updates:
  // mix_presentation_metadata.
  void updateIamfMDFromRepository(
      MixPresentationRepository& mixPresentationRepository,
      iamf_tools_cli_proto::UserMetadata& iamfMD,
      std::unordered_map<juce::Uuid, int>& audioElementIDMap);

  // Export an IAMF file and handle possible errors.
  bool exportIamfFile(const juce::String input_wav_path,
                      const juce::String output_iamf_path);

  // Initialize IAMF metadata to reasonable defaults.
  void initIamfMetadata(iamf_tools_cli_proto::UserMetadata& iamfMD,
                        juce::String outputFilename) const;

 protected:
  void dumpExportLogs(const absl::Status& status) const;

  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout params;
    return params;
  }

  juce::AudioParameterFloatAttributes initParameterAttributes(
      int decimalPlaces, juce::String&& label) const {
    return juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction([decimalPlaces](float value, int unused) {
          juce::ignoreUnused(unused);
          return juce::String(value, decimalPlaces, false);
        })
        .withLabel(label);
  }

  void initializeFileExport(FileExport& config);

  void closeFileExport(FileExport& config);

  bool shouldBufferBeWritten(const juce::AudioBuffer<float>& buffer);

  bool performingRender_;  // True if we are rendering in offline mode
  FileExportRepository& fileExportRepository_;
  AudioElementRepository& audioElementRepository_;
  MixPresentationRepository& mixPresentationRepository_;
  MixPresentationLoudnessRepository& mixPresentationLoudnessRepository_;
  std::vector<std::unique_ptr<AudioElementFileWriter>> iamfWavFileWriters_;
  int numSamples_;
  long sampleRate_;
  int startTime_;
  int endTime_;
  long sampleTally_;
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileOutputProcessor)
};
