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

IAMFFileReader::IAMFFileReader(const std::filesystem::path& iamfFilePath,
                               std::atomic_bool& abortConstruction)
    : IAMFFileReader(iamfFilePath, kDefaultReaderSettings, abortConstruction) {}

IAMFFileReader::IAMFFileReader(const std::filesystem::path& iamfFilePath,
                               const Settings& settings,
                               std::atomic_bool& abortConstruction)
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

  // Initialize sample buffer based on stream data
  // `sizeof(int32_t)` since decoder output is default 32-bit PCM
  const size_t kSampleBufferSize =
      streamData_.frameSize * streamData_.numChannels * sizeof(int32_t);
  sampleBuffer_ = std::make_unique<char[]>(kSampleBufferSize);
}

IAMFFileReader::~IAMFFileReader() {
  if (fileStream_ && fileStream_->is_open()) {
    fileStream_->close();
  }
}

std::unique_ptr<IAMFFileReader> IAMFFileReader::createIamfReader(
    const std::filesystem::path& iamfFilePath) {
  std::atomic_bool abortConstruction(false);
  return createIamfReader(iamfFilePath, kDefaultReaderSettings,
                          abortConstruction);
}

std::unique_ptr<IAMFFileReader> IAMFFileReader::createIamfReader(
    const std::filesystem::path& iamfFilePath, const Settings& settings,
    std::atomic_bool& abortConstruction) {
  if (!std::filesystem::exists(iamfFilePath)) {
    LOG_ERROR(0, "IAMFFileReader: IAMF file does not exist");
    return nullptr;
  }

  auto reader = std::unique_ptr<IAMFFileReader>(
      new IAMFFileReader(iamfFilePath, settings, abortConstruction));

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

  size_t bytesRead = 0;
  iamfDecoder_->GetOutputTemporalUnit(
      reinterpret_cast<uint8_t*>(sampleBuffer_.get()), kPCMSampleBufferSize,
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
            reinterpret_cast<int32_t*>(sampleBuffer_.get()), *buffer,
            kSampsPerCh, streamData_.numChannels);
      }
    }
    samplesRead = kSampsPerCh;
  }
  return samplesRead;
}

size_t IAMFFileReader::indexFile(std::atomic_bool& haltIndexing) {
  if (!iamfDecoder_ || !iamfDecoder_->IsDescriptorProcessingComplete()) {
    LOG_ERROR(0, "IAMFFileReader: Cannot index file - decoder not ready");
    return -1;
  }

  // Count temporal units by traversing raw OBU headers without decoding audio.
  //
  // IAMF temporal structure
  // -----------------------
  // An IAMF file is a flat sequence of Open Bitstream Units (OBUs). The first
  // group are Descriptor OBUs (Sequence Header, Codec Config, Audio Element,
  // Mix Presentation), which describe the file's structure. All remaining OBUs
  // form the audio payload, organised into Temporal Units — one per decoded
  // frame. A Temporal Unit contains one Audio Frame OBU per audio substream,
  // covering every substream from every Audio Element in the file.
  //
  // Crucially, Mix Presentations are metadata only. They describe how Audio
  // Elements should be mixed and rendered at decode time, but they do not
  // affect which substreams are written into the bitstream. A file with two
  // Mix Presentations — one referencing a stereo element, the other a 5.1
  // element — still writes BOTH elements' substreams into every Temporal Unit.
  // The decoder chooses which Mix Presentation to render; the encoder always
  // includes all audio data. This means the temporal unit count is a single
  // value for the entire file, independent of the number of Mix Presentations,
  // how many Audio Elements each references, or what speaker layouts they use.
  //
  // Counting strategy
  // -----------------
  // Because substream IDs are unique across the whole file and are assigned
  // sequentially starting at 0, substream 0 always belongs to the first Audio
  // Element's first substream. By the structure above, exactly one Audio Frame
  // OBU for substream 0 appears in every Temporal Unit, so counting those OBUs
  // gives the total frame count.
  //
  // The IAMF spec defines two OBU types for substream 0:
  //   Type 5  IA_Audio_Frame       implicit substream ID 0 (no ID in payload)
  //   Type 6  IA_Audio_Frame_Id0   explicit substream ID 0 (ID in payload)
  // Encoders use one style consistently; both mean the same thing here.
  //
  // OBU header format (IAMF spec §3.3)
  // -----------------------------------
  // Byte 0:  [obu_type: 5 bits][redundant_copy: 1][trimming_flag: 1][ext: 1]
  //          obu_type = (byte >> 3) & 0x1F
  // Byte 1:  Optional extension header, present when ext bit is set.
  // Next:    obu_size as an unsigned leb128 integer.
  // Then:    Payload of obu_size bytes (skipped entirely here).
  //
  // This traversal does no decoding — it reads a handful of bytes per OBU and
  // seeks past each payload, reducing indexing cost from O(decode all frames)
  // to O(file size / average OBU size), which is effectively just I/O speed.
  fileStream_->clear();
  fileStream_->seekg(0, std::ios::beg);

  size_t frameCount = 0;
  uint8_t headerByte;
  while (!haltIndexing &&
         fileStream_->read(reinterpret_cast<char*>(&headerByte), 1)) {
    const uint8_t obuType = (headerByte >> 3) & 0x1F;
    const bool extensionFlag = headerByte & 0x01;

    if (extensionFlag) {
      fileStream_->seekg(1, std::ios::cur);
    }

    // Read leb128 obu_size
    uint64_t obuSize = 0;
    int shift = 0;
    uint8_t leb;
    while (fileStream_->read(reinterpret_cast<char*>(&leb), 1)) {
      obuSize |= static_cast<uint64_t>(leb & 0x7F) << shift;
      shift += 7;
      if (!(leb & 0x80)) break;
    }

    // One of these appears exactly once per temporal unit (see above).
    if (obuType == 5 || obuType == 6) {
      ++frameCount;
    }

    fileStream_->seekg(static_cast<std::streamoff>(obuSize), std::ios::cur);
  }

  if (haltIndexing) {
    return -1;
  }

  streamData_.numFrames = frameCount;

  // Reset file and decoder state for normal playback.
  fileStream_->clear();
  fileStream_->seekg(0, std::ios::beg);
  iamfDecoder_ = iamf_tools::api::IamfDecoderFactory::Create(settings_);
  if (!iamfDecoder_) {
    LOG_ERROR(0,
              "IAMFFileReader: Failed to recreate IAMF decoder after indexing");
    streamData_.valid = false;
    return 0;
  }
  parseStreamData(iamfDecoder_, fileStream_);
  streamData_.currentFrameIdx = 0;

  return frameCount;
}

bool IAMFFileReader::seekFrame(const size_t frameIdx) {
  std::atomic_bool abort = false;
  return seekFrame(frameIdx, abort);
}

bool IAMFFileReader::seekFrame(const size_t frameIdx,
                               std::atomic_bool& abortSeek) {
  if (frameIdx >= streamData_.numFrames) {
    LOG_WARNING(0, "IAMFFileReader: Frame index out of range: " +
                       std::to_string(frameIdx));
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

  // Advance to the requested frame, checking abort flag
  while (streamData_.currentFrameIdx < frameIdx) {
    if (abortSeek) {
      LOG_INFO(0, "IAMFFileReader: Seek operation aborted");
      return false;
    }

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

  // Reinitialize sample buffer if stream data changed
  if (kNewStreamData.frameSize != streamData_.frameSize ||
      kNewStreamData.numChannels != streamData_.numChannels) {
    const size_t kSampleBufferSize =
        kNewStreamData.frameSize * kNewStreamData.numChannels * sizeof(int32_t);
    sampleBuffer_ = std::make_unique<char[]>(kSampleBufferSize);
  }

  streamData_ = kNewStreamData;
  streamData_.numChannels = layout.getNumChannels();
  streamData_.numFrames = kOriginalNumFrames;
  streamData_.currentFrameIdx = 0;

  return true;
}