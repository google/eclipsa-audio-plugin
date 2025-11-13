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

#include "IAMFFileReader.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>

#include "iamf_tools_api_types.h"
#include "logger/logger.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"

// Parse descriptors to determine audio stream params for the selected mix
// presentation
static IAMFFileReader::StreamData parseOBUs(
    std::unique_ptr<IAMFFileReader::Decoder>& decoder,
    std::unique_ptr<std::ifstream>& fileStream) {
  const size_t kBufferSize = 4096;
  IAMFFileReader::StreamData streamData;

  char buffer[kBufferSize];
  while (fileStream->read(buffer, kBufferSize) || fileStream->gcount() > 0) {
    decoder->Decode(reinterpret_cast<const uint8_t*>(buffer),
                    fileStream->gcount());

    if (decoder->IsDescriptorProcessingComplete()) {
      streamData.valid = true;
      decoder->GetNumberOfOutputChannels(streamData.numChannels);
      decoder->GetSampleRate(streamData.sampleRate);
      decoder->GetFrameSize(streamData.frameSize);
      // Requested playback layout may differ from actual output layout.
      iamf_tools::api::SelectedMix selectedMix;
      decoder->GetOutputMix(selectedMix);
      streamData.playbackLayout =
          Speakers::AudioElementSpeakerLayout(selectedMix.output_layout);
      return streamData;
    }
  }

  return streamData;
}

static IAMFFileReader::StreamData parseStreamData(
    std::unique_ptr<IAMFFileReader::Decoder>& decoder,
    std::unique_ptr<std::ifstream>& fileStream) {
  return parseOBUs(decoder, fileStream);
}

IAMFFileReader::IAMFFileReader(const std::filesystem::path& iamfFilePath)
    : IAMFFileReader(iamfFilePath, kDefaultReaderSettings) {}

IAMFFileReader::IAMFFileReader(const std::filesystem::path& iamfFilePath,
                               const Settings& settings)
    : kFilePath_(iamfFilePath),
      settings_(settings),
      tpuBuffer_(std::make_unique<char[]>(kBufferSize_)) {
  // Create an initial decoder to parse Descriptor OBUs
  iamfDecoder_ = iamf_tools::api::IamfDecoderFactory::Create(settings_);
  if (!iamfDecoder_) {
    LOG_ERROR(0, "IAMFFileReader: Failed to create IAMF decoder");
    return;
  }

  fileStream_ = std::make_unique<std::ifstream>(kFilePath_, std::ios::binary);
  if (!fileStream_->is_open()) {
    LOG_ERROR(0, "IAMFFileReader: Failed to open IAMF file");
    return;
  }

  streamData_ = parseStreamData(iamfDecoder_, fileStream_);
  if (!streamData_.valid) {
    LOG_ERROR(0, "IAMFFileReader: Failed to parse IAMF file");
    return;
  }
  countFrames(iamfDecoder_, fileStream_);
}

IAMFFileReader::~IAMFFileReader() {
  if (fileStream_ && fileStream_->is_open()) {
    fileStream_->close();
  }
}

std::unique_ptr<IAMFFileReader> IAMFFileReader::createIamfReader(
    const std::filesystem::path& iamfFilePath) {
  return createIamfReader(iamfFilePath, kDefaultReaderSettings);
}

std::unique_ptr<IAMFFileReader> IAMFFileReader::createIamfReader(
    const std::filesystem::path& iamfFilePath, const Settings& settings) {
  if (!std::filesystem::exists(iamfFilePath)) {
    LOG_ERROR(0, "IAMFFileReader: IAMF file does not exist");
    return nullptr;
  }

  auto reader = std::unique_ptr<IAMFFileReader>(
      new IAMFFileReader(iamfFilePath, settings));

  // Check if initialization was successful by verifying streamData is valid
  if (!reader->streamData_.valid) {
    return nullptr;
  }

  return reader;
}

bool IAMFFileReader::prepareTemporalUnit(std::unique_ptr<Decoder>& decoder) {
  while (!iamfDecoder_->IsTemporalUnitAvailable()) {
    if (fileStream_->read(tpuBuffer_.get(), kBufferSize_) ||
        fileStream_->gcount() > 0) {
      iamfDecoder_->Decode(reinterpret_cast<const uint8_t*>(tpuBuffer_.get()),
                           fileStream_->gcount());
    } else {
      // End of file reached, signal decoder to flush remaining temporal units
      iamfDecoder_->SignalEndOfDecoding();
      return iamfDecoder_->IsTemporalUnitAvailable();
    }
  }
  return true;
}

static void convertAndCopyChannelMajor(const int32_t* input,
                                       juce::AudioBuffer<float>& output,
                                       int numSamples, int numChannels) {
  constexpr float kScale = 1.0f / INT32_MAX;

  for (int channel = 0; channel < numChannels; ++channel) {
    float* out = output.getWritePointer(channel);

    // Stride through input to pick out only this channel
    const int32_t* kIn = input + channel;

    for (int sample = 0; sample < numSamples; ++sample) {
      out[sample] = static_cast<float>(*kIn) * kScale;
      kIn += numChannels;  // Jump to next sample for this channel
    }
  }
}

size_t IAMFFileReader::readFrame(juce::AudioBuffer<float>& buffer) {
  if (buffer.getNumChannels() != streamData_.numChannels ||
      buffer.getNumSamples() != streamData_.frameSize) {
    LOG_ERROR(0, "IAMFFileReader: Buffer size does not match stream data");
    return 0;
  }

  return parseFrame(&buffer);
}

size_t IAMFFileReader::parseFrame(juce::AudioBuffer<float>* buffer) {
  if (!prepareTemporalUnit(iamfDecoder_)) {
    return 0;
  }

  const size_t kPCMSampleBufferSize =
      streamData_.frameSize * streamData_.numChannels * sizeof(int32_t);
  std::unique_ptr<char[]> sampleBuffer = std::make_unique<char[]>(
      streamData_.frameSize * streamData_.numChannels * sizeof(int32_t));

  size_t bytesRead = 0;
  iamfDecoder_->GetOutputTemporalUnit(
      reinterpret_cast<uint8_t*>(sampleBuffer.get()), kPCMSampleBufferSize,
      bytesRead);
  size_t samplesRead = 0;
  if (bytesRead > 0) {
    // Samples are interleaved 32-bit ints to be parsed out
    ++streamData_.currentFrameIdx;
    const size_t kSampsTotal = bytesRead / sizeof(int32_t);
    const size_t kSampsPerCh = kSampsTotal / streamData_.numChannels;
    if (kSampsTotal / streamData_.numChannels != streamData_.frameSize) {
      LOG_INFO(0, "IAMFFileReader: Incomplete frame");
    }

    if (buffer) {
      for (int i = 0; i < streamData_.numChannels; ++i) {
        convertAndCopyChannelMajor(
            reinterpret_cast<int32_t*>(sampleBuffer.get()), *buffer,
            kSampsPerCh, streamData_.numChannels);
      }
    }
    samplesRead = kSampsPerCh;
  }
  return samplesRead;
}

// To be called after parsing OBUs.
// Counts frames in the file.
size_t IAMFFileReader::countFrames(std::unique_ptr<Decoder>& decoder,
                                   std::unique_ptr<std::ifstream>& fileStream) {
  jassert(decoder->IsDescriptorProcessingComplete());

  size_t frameCount = 0;
  while (parseFrame()) {
    frameCount++;
  }
  streamData_.numFrames = frameCount;

  fileStream->clear();
  fileStream->seekg(0, std::ios::beg);
  decoder = iamf_tools::api::IamfDecoderFactory::Create(settings_);
  if (!decoder) {
    LOG_ERROR(0,
              "IAMFFileReader: Failed to recreate IAMF decoder after indexing");
    streamData_.valid = false;
    return 0;
  }
  parseStreamData(decoder, fileStream);
  streamData_.currentFrameIdx = 0;
  return frameCount;
}

bool IAMFFileReader::seekFrame(const size_t frameIdx) {
  if (frameIdx >= streamData_.numFrames) {
    LOG_WARNING(0, "IAMFFileReader: Frame index out of range");
    return false;
  }

  // If seeking backward, reset decoder and file position, then advance
  if (frameIdx < streamData_.currentFrameIdx) {
    // Reset file position
    fileStream_->clear();
    fileStream_->seekg(0, std::ios::beg);

    // Recreate decoder
    iamfDecoder_ = iamf_tools::api::IamfDecoderFactory::Create(settings_);
    if (!iamfDecoder_) {
      LOG_ERROR(0, "IAMFFileReader: Failed to recreate decoder during seek");
      return false;
    }

    // Reposition decoder to start of temporal units after
    const StreamData kStreamData = parseStreamData(iamfDecoder_, fileStream_);
    if (!kStreamData.valid) {
      LOG_ERROR(0, "IAMFFileReader: Failed to reparse stream data during seek");
      return false;
    }

    streamData_.currentFrameIdx = 0;
  }

  // Advance to the requested frame
  while (streamData_.currentFrameIdx < frameIdx) {
    if (parseFrame() == 0) {
      return false;
    }
  }

  return true;
}

bool IAMFFileReader::resetLayout(
    const Speakers::AudioElementSpeakerLayout& layout) {
  // Update settings with new layout
  settings_.requested_mix.output_layout = layout.getIamfOutputLayout();

  // Reset file position
  fileStream_->clear();
  fileStream_->seekg(0, std::ios::beg);

  // Recreate decoder with new settings
  iamfDecoder_ = iamf_tools::api::IamfDecoderFactory::Create(settings_);
  if (!iamfDecoder_) {
    LOG_ERROR(0,
              "IAMFFileReader: Failed to recreate decoder during layout reset");
    streamData_.valid = false;
    return false;
  }

  // Reparse stream data with new layout
  const StreamData kNewStreamData = parseStreamData(iamfDecoder_, fileStream_);
  if (!kNewStreamData.valid) {
    LOG_ERROR(
        0, "IAMFFileReader: Failed to parse stream data during layout reset");
    streamData_.valid = false;
    return false;
  }

  // Update stream data with new layout information, but keep frame count
  const size_t kOriginalNumFrames = streamData_.numFrames;
  streamData_ = kNewStreamData;
  streamData_.numFrames = kOriginalNumFrames;
  streamData_.currentFrameIdx = 0;

  return true;
}