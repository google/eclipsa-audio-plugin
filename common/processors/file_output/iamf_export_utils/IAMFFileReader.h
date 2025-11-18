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
#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <filesystem>
#include <iosfwd>
#include <memory>

#include "iamf/include/iamf_tools/iamf_decoder_factory.h"
#include "iamf/include/iamf_tools/iamf_decoder_interface.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

class IAMFFileReader {
 public:
  using Settings = iamf_tools::api::IamfDecoderFactory::Settings;
  using Decoder = iamf_tools::api::IamfDecoderInterface;

  struct StreamData {
    int numChannels = 0;
    unsigned sampleRate = 0, frameSize = 0;
    size_t numFrames = 0, currentFrameIdx = 0;
    Speakers::AudioElementSpeakerLayout playbackLayout = Speakers::kUnknown;
    bool valid = false;
  };

  static const inline IAMFFileReader::Settings kDefaultReaderSettings = {
      .requested_mix =
          {.output_layout =
               iamf_tools::api::OutputLayout::kItu2051_SoundSystemA_0_2_0},
      .requested_profile_versions =
          {iamf_tools::api::ProfileVersion::kIamfBaseEnhancedProfile},
      .requested_output_sample_type =
          iamf_tools::api::OutputSampleType::kInt32LittleEndian,
  };

  // Create a decoder with default settings
  static std::unique_ptr<IAMFFileReader> createIamfReader(
      const std::filesystem::path& iamfFilePath);
  // Create a decoder with custom settings and an option to abort construction
  static std::unique_ptr<IAMFFileReader> createIamfReader(
      const std::filesystem::path& iamfFilePath, const Settings& settings,
      std::atomic_bool& abortConstruction);

  IAMFFileReader(const IAMFFileReader&) = delete;
  IAMFFileReader& operator=(const IAMFFileReader&) = delete;
  IAMFFileReader(IAMFFileReader&&) noexcept = default;
  IAMFFileReader& operator=(IAMFFileReader&&) noexcept = default;
  ~IAMFFileReader();

  StreamData getStreamData() const { return streamData_; }
  size_t readFrame(juce::AudioBuffer<float>& buffer);
  size_t readFrame(juce::AudioBuffer<double>& buffer);
  bool seekFrame(const size_t frameIdx);
  bool resetLayout(const Speakers::AudioElementSpeakerLayout& layout);

  // Method to be called via a valid reader instance to index the file.
  // Takes a reference to an flag that can be set to halt indexing prematurely.
  // Returns the number of frames valid frames or -1 if halted.
  size_t indexFile(std::atomic_bool& haltIndexing);

 private:
  IAMFFileReader(const std::filesystem::path& iamfFilePath,
                 std::atomic_bool& abortConstruction);
  IAMFFileReader(const std::filesystem::path& iamfFilePath,
                 const Settings& settings, std::atomic_bool& abortConstruction);

  static constexpr size_t kBufferSize_ = 4096;

  bool prepareTemporalUnit(std::unique_ptr<Decoder>& decoder);
  size_t parseFrame(juce::AudioBuffer<float>* buffer = nullptr);

  const std::filesystem::path kFilePath_;
  Settings settings_;
  std::unique_ptr<std::ifstream> fileStream_;
  std::unique_ptr<char[]> tpuBuffer_, sampleBuffer_;
  std::unique_ptr<Decoder> iamfDecoder_;
  StreamData streamData_;
};