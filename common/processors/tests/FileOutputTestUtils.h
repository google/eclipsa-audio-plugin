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

#include <filesystem>
#include <fstream>

#include "IAMF_decoder.h"
#include "IAMF_defines.h"
#include "data_structures/src/FileExport.h"
#include "dep_wavwriter.h"
#include "processors/file_output/FileOutputProcessor.h"
#include "processors/file_output/FileOutputProcessor_PremierePro.h"

extern "C" {
#include "mp4iamfpar.h"
}

// Define SOUND_SYSTEM_A for stereo
#define SOUND_SYSTEM_A 0

// Generate x samples of a sine wave at the given frequency and sample rate.
static juce::AudioBuffer<float> generateSineWave(float frequency,
                                                 int sampleRate,
                                                 int numSamples) {
  juce::AudioBuffer<float> buffer(1, numSamples);
  for (int i = 0; i < numSamples; ++i) {
    float sample =
        0.1f * std::sin(2 * M_PI * frequency * (float)i / (float)sampleRate);
    buffer.setSample(0, i, sample);
  }
  return buffer;
}

static float sampleSine(const unsigned freq, const float n,
                        const unsigned sampleRate) {
  return 0.2f * std::sin(2 * M_PI * freq * n / (float)sampleRate);
}

static unsigned totalAudioChannels(
    AudioElementRepository& audioElementRepository) {
  unsigned totalChannels = 0;
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository.getAll(audioElements);
  for (int i = 0; i < audioElements.size(); ++i) {
    totalChannels += audioElements[i]->getChannelCount();
  }
  return totalChannels;
}

static FileProfile profileFromAEs(
    AudioElementRepository& audioElementRepository) {
  juce::OwnedArray<AudioElement> audioElements;
  audioElementRepository.getAll(audioElements);

  // If any element has an expanded layout, we need Base-Enhanced
  for (int i = 0; i < audioElements.size(); ++i) {
    const auto& layout = audioElements[i]->getChannelConfig();
    if (layout.isExpandedLayout()) {
      return FileProfile::BASE_ENHANCED;
    }
  }

  // Otherwise layout is determined by channel and element count
  return FileProfileHelper::minimumProfile(
      totalAudioChannels(audioElementRepository), audioElements);
}

// Helper used by multiple tests to render a short non-realtime bounce.
static inline void bounceAudio(FileOutputProcessor& fio_proc,
                               AudioElementRepository& audioElementRepository,
                               unsigned sampleRate = 48e3,
                               unsigned frameSize = 128) {
  const unsigned kNumChannels = totalAudioChannels(audioElementRepository);
  const auto kSineTone = generateSineWave(440.0f, sampleRate, frameSize);

  fio_proc.prepareToPlay(sampleRate, frameSize);
  fio_proc.setNonRealtime(true);

  juce::AudioBuffer<float> audioBuffer(kNumChannels, frameSize);
  juce::MidiBuffer dummyMidiBuffer;
  for (int block = 0; block < 8; ++block) {
    for (unsigned i = 0; i < kNumChannels; ++i) {
      audioBuffer.copyFrom(i, 0, kSineTone, 0, 0, frameSize);
    }
    fio_proc.processBlock(audioBuffer, dummyMidiBuffer);
  }
  fio_proc.setNonRealtime(false);
}

// Helper used by multiple tests to render a short bounce using the premiere pro
// file output processor.
static inline void bouncePremiereProAudio(
    FileExportRepository& fileExportRepository,
    AudioElementRepository& audioElementRepository,
    MixPresentationRepository& mixPresentationRepository,
    MixPresentationLoudnessRepository& mixPresentationLoudnessRepository,
    unsigned sampleRate = 48e3, unsigned frameSize = 128) {
  // First, premiere pro starts a manual export
  auto fileExport = fileExportRepository.get();
  fileExport.setManualExport(true);
  fileExportRepository.update(fileExport);

  // Premiere pro reconstructs the file output processor each time, rather then
  // using an existing instance
  PremiereProFileOutputProcessor fio_proc_pp(
      fileExportRepository, audioElementRepository, mixPresentationRepository,
      mixPresentationLoudnessRepository);

  const unsigned kNumChannels = totalAudioChannels(audioElementRepository);
  const auto kSineTone = generateSineWave(440.0f, sampleRate, frameSize);

  // Premiere pro calls prepare to play, and set non-realtime correctly once
  fio_proc_pp.prepareToPlay(sampleRate, frameSize);
  fio_proc_pp.setNonRealtime(true);

  juce::AudioBuffer<float> audioBuffer(kNumChannels, frameSize);
  juce::MidiBuffer dummyMidiBuffer;
  for (int block = 0; block < 8; ++block) {
    for (unsigned i = 0; i < kNumChannels; ++i) {
      audioBuffer.copyFrom(i, 0, kSineTone, 0, 0, frameSize);
    }
    fio_proc_pp.processBlock(audioBuffer, dummyMidiBuffer);

    // Premiere pro calls set non-realtime incorrectly on each frame
    fio_proc_pp.setNonRealtime(false);
  }

  // Premiere pro completes by destroying the file output processor
  // Since it's a local instance, this happens automatically when this function
  // returns
}

class MP4IAMFDemuxer {
 public:
  MP4IAMFDemuxer() = default;
  ~MP4IAMFDemuxer() = default;

  bool verifyIAMFIntegrity(const juce::String& mp4FilePath,
                           const juce::String& originalIAMFPath, int sampleRate,
                           int bitDepth, int soundSystem = SOUND_SYSTEM_A,
                           float tolerance = 0.0001f) {
    juce::String demuxedIAMFPath = juce::File::getCurrentWorkingDirectory()
                                       .getChildFile("demuxed_test.iamf")
                                       .getFullPathName();
    juce::String originalDecodedWavPath =
        juce::File::getCurrentWorkingDirectory()
            .getChildFile("original_test.wav")
            .getFullPathName();
    juce::String demuxedDecodedWavPath =
        juce::File::getCurrentWorkingDirectory()
            .getChildFile("demuxed_test.wav")
            .getFullPathName();

    bool demuxSuccess = demuxIAMFFromMP4(mp4FilePath, demuxedIAMFPath);
    if (!demuxSuccess) {
      return false;
    }

    // Non-fatal IAMF file byte comparison.
    compareIAMFFiles(originalIAMFPath, demuxedIAMFPath);

    bool decodeOriginalSuccess =
        decodeIAMFToWAV(originalIAMFPath, originalDecodedWavPath, soundSystem,
                        sampleRate, bitDepth);
    bool decodeDemuxedSuccess =
        decodeIAMFToWAV(demuxedIAMFPath, demuxedDecodedWavPath, soundSystem,
                        sampleRate, bitDepth);
    if (!decodeOriginalSuccess || !decodeDemuxedSuccess) {
      juce::File(demuxedIAMFPath).deleteFile();
      juce::File(originalDecodedWavPath).deleteFile();
      juce::File(demuxedDecodedWavPath).deleteFile();
      return false;
    }

    bool wavComparisonResult = compareWAVFiles(
        originalDecodedWavPath, demuxedDecodedWavPath, tolerance);

    juce::File(demuxedIAMFPath).deleteFile();
    juce::File(originalDecodedWavPath).deleteFile();
    juce::File(demuxedDecodedWavPath).deleteFile();
    return wavComparisonResult;
  }

 private:
  bool demuxIAMFFromMP4(const juce::String& mp4FilePath,
                        const juce::String& outputIAMFPath) {
    MP4IAMFParser parser;
    IAMFHeader* header = nullptr;
    mp4_iamf_parser_init(&parser);
    if (mp4_iamf_parser_open_audio_track(&parser, mp4FilePath.toRawUTF8(),
                                         &header) <= 0) {
      mp4_iamf_parser_close(&parser);
      return false;
    }
    bool success = extractIAMFData(&parser, outputIAMFPath);
    mp4_iamf_parser_close(&parser);
    return success;
  }

  bool extractIAMFData(MP4IAMFParser* parser, const juce::String& outputPath) {
    std::ofstream outFile(outputPath.toRawUTF8(), std::ios::binary);
    if (!outFile.is_open()) return false;
    uint8_t* pktBuf = nullptr;
    uint32_t pktLen = 0;
    int sampleDelta = 0;
    int64_t sampleOffset = 0;
    int entryNo = 0;
    bool end = false;
    IAMFHeader* header = nullptr;
    if (mp4_iamf_parser_get_audio_track_header(parser, &header) <= 0 ||
        !header) {
      outFile.close();
      return false;
    }
    uint8_t* descriptionOBUs = nullptr;
    uint32_t descriptionSize = 0;
    if (iamf_header_read_description_OBUs(header, &descriptionOBUs,
                                          &descriptionSize) <= 0) {
      outFile.close();
      return false;
    }
    outFile.write(reinterpret_cast<char*>(descriptionOBUs), descriptionSize);
    free(descriptionOBUs);
    while (!end) {
      sampleDelta = mp4_iamf_parser_read_packet(parser, 0, &pktBuf, &pktLen,
                                                &sampleOffset, &entryNo);
      if (sampleDelta < 0 || !pktBuf) {
        end = true;
        continue;
      }
      outFile.write(reinterpret_cast<char*>(pktBuf), pktLen);
      free(pktBuf);
      pktBuf = nullptr;
    }
    outFile.close();
    return true;
  }

  bool decodeIAMFToWAV(const juce::String& iamfPath,
                       const juce::String& outputWavPath, int soundSystem,
                       int sampleRate, int bitDepth) {
    std::vector<uint8_t> iamfData = loadFileToMemory(iamfPath);
    if (iamfData.empty()) return false;
    IAMF_DecoderHandle decoder = IAMF_decoder_open();
    if (!decoder) return false;
    IAMF_decoder_set_bit_depth(decoder, bitDepth);
    IAMF_decoder_output_layout_set_sound_system(
        decoder, static_cast<IAMF_SoundSystem>(soundSystem));
    int channels = IAMF_layout_sound_system_channels_count(
        static_cast<IAMF_SoundSystem>(soundSystem));
    uint32_t bytesUsed = 0;
    if (IAMF_decoder_configure(decoder, iamfData.data(), iamfData.size(),
                               &bytesUsed) != IAMF_OK) {
      IAMF_decoder_close(decoder);
      return false;
    }
    IAMF_StreamInfo* info = IAMF_decoder_get_stream_info(decoder);
    if (!info) {
      IAMF_decoder_close(decoder);
      return false;
    }
    FILE* wavFile = (FILE*)dep_wav_write_open(outputWavPath.toRawUTF8(),
                                              sampleRate, bitDepth, channels);
    if (!wavFile) {
      IAMF_decoder_close(decoder);
      return false;
    }
    std::vector<uint8_t> pcmBuffer(bitDepth / 8 * info->max_frame_size *
                                   channels);
    size_t dataOffset = bytesUsed;
    while (dataOffset < iamfData.size()) {
      bytesUsed = 0;
      int samplesDecoded = IAMF_decoder_decode(
          decoder, iamfData.data() + dataOffset, iamfData.size() - dataOffset,
          &bytesUsed, pcmBuffer.data());
      if (samplesDecoded > 0) {
        dep_wav_write_data(wavFile, pcmBuffer.data(),
                           bitDepth / 8 * samplesDecoded * channels);
      }
      if (bytesUsed == 0 || samplesDecoded < 0) break;
      dataOffset += bytesUsed;
    }
    dep_wav_write_close(wavFile);
    IAMF_decoder_close(decoder);
    return true;
  }

  bool compareIAMFFiles(const juce::String& originalIAMFPath,
                        const juce::String& demuxedIAMFPath) {
    std::ifstream original(originalIAMFPath.toRawUTF8(), std::ios::binary);
    std::ifstream demuxed(demuxedIAMFPath.toRawUTF8(), std::ios::binary);
    if (!original.is_open() || !demuxed.is_open()) return false;
    original.seekg(0, std::ios::end);
    demuxed.seekg(0, std::ios::end);
    std::streamsize originalSize = original.tellg();
    std::streamsize demuxedSize = demuxed.tellg();
    original.seekg(0, std::ios::beg);
    demuxed.seekg(0, std::ios::beg);
    if (originalSize != demuxedSize) return false;
    const int bufferSize = 4096;
    char bufferOriginal[bufferSize];
    char bufferDemuxed[bufferSize];
    std::streamsize bytesRemaining = originalSize;
    while (bytesRemaining > 0) {
      std::streamsize bytesToRead =
          std::min(static_cast<std::streamsize>(bufferSize), bytesRemaining);
      original.read(bufferOriginal, bytesToRead);
      demuxed.read(bufferDemuxed, bytesToRead);
      if (memcmp(bufferOriginal, bufferDemuxed, bytesToRead) != 0) return false;
      bytesRemaining -= bytesToRead;
    }
    return true;
  }

  bool compareWAVFiles(const juce::String& originalWavPath,
                       const juce::String& demuxedWavPath, float tolerance) {
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> originalReader(
        formatManager.createReaderFor(juce::File(originalWavPath)));
    std::unique_ptr<juce::AudioFormatReader> demuxedReader(
        formatManager.createReaderFor(juce::File(demuxedWavPath)));
    if (!originalReader || !demuxedReader) return false;
    if (originalReader->numChannels != demuxedReader->numChannels ||
        originalReader->sampleRate != demuxedReader->sampleRate)
      return false;
    const int bufferSize = 1024;
    juce::AudioBuffer<float> originalBuffer(originalReader->numChannels,
                                            bufferSize);
    juce::AudioBuffer<float> demuxedBuffer(demuxedReader->numChannels,
                                           bufferSize);
    juce::int64 position = 0;
    const juce::int64 totalSamples = std::min(originalReader->lengthInSamples,
                                              demuxedReader->lengthInSamples);
    while (position < totalSamples) {
      const int numSamples = std::min(static_cast<juce::int64>(bufferSize),
                                      totalSamples - position);
      originalReader->read(&originalBuffer, 0, numSamples, position, true,
                           true);
      demuxedReader->read(&demuxedBuffer, 0, numSamples, position, true, true);
      for (int channel = 0; channel < originalReader->numChannels; ++channel) {
        const float* o = originalBuffer.getReadPointer(channel);
        const float* d = demuxedBuffer.getReadPointer(channel);
        for (int s = 0; s < numSamples; ++s) {
          if (std::abs(o[s] - d[s]) > tolerance) return false;
        }
      }
      position += numSamples;
    }
    return true;
  }

  std::vector<uint8_t> loadFileToMemory(const juce::String& filePath) {
    std::ifstream file(filePath.toRawUTF8(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) return {};
    return buffer;
  }
};

// Debug tool for writing audio data to wave files for offline tools
class WavFileWriter {
 public:
  WavFileWriter(const std::filesystem::path& filePath, int numChannels,
                double sampleRate = 48000)
      : numChannels_(numChannels) {
    wavFormat_.reset(new juce::WavAudioFormat());
    std::filesystem::remove(filePath);
    juce::File file(filePath.string());
    std::unique_ptr<juce::FileOutputStream> outputStream(
        file.createOutputStream());
    writer_.reset(wavFormat_->createWriterFor(outputStream.get(), sampleRate,
                                              numChannels_, 16, {}, 0));
    (void)outputStream.release();
  }

  ~WavFileWriter() { writer_->flush(); }

  bool write(const juce::AudioBuffer<float>& buffer, int numSamples) {
    if (!writer_ || buffer.getNumChannels() != numChannels_) {
      return false;
    }
    return writer_->writeFromAudioSampleBuffer(buffer, 0, numSamples);
  }

  bool isOpen() const { return writer_ != nullptr; }

 private:
  const int numChannels_;
  std::unique_ptr<juce::WavAudioFormat> wavFormat_;
  std::unique_ptr<juce::AudioFormatWriter> writer_;
};
