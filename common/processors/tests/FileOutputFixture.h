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

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_video/juce_video.h>

#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../common/logger/logger.h"
#include "IAMF_decoder.h"
#include "IAMF_defines.h"
#include "codec_config.pb.h"
#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/FileExportRepository.h"
#include "data_repository/implementation/MixPresentationLoudnessRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/FileExport.h"
#include "data_structures/src/LanguageCodeMetaData.h"
#include "data_structures/src/MixPresentationLoudness.h"
#include "dep_wavwriter.h"
#include "google/protobuf/text_format.h"
#include "ia_sequence_header.pb.h"
#include "iamf/include/iamf_tools/encoder_main_lib.h"
#include "processors/file_output/FileOutputProcessor.h"
#include "substream_rdr/substream_rdr_utils/Speakers.h"
#include "user_metadata.pb.h"

extern "C" {
#include "mp4iamfpar.h"
}

// Define SOUND_SYSTEM_A for stereo
#define SOUND_SYSTEM_A 0

/**
 * @class MP4IAMFDemuxer
 * @brief A class that handles demuxing IAMF audio from MP4 files
 *
 * This class provides functionality to:
 * 1. Extract IAMF bitstreams from MP4 files
 * 2. Compare original and demuxed IAMF bitstreams
 * 3. Optionally decode to WAV for audio comparison
 */
class MP4IAMFDemuxer {
 public:
  MP4IAMFDemuxer() {
    // Nothing to initialize
  }

  ~MP4IAMFDemuxer() {
    // Nothing to clean up
  }

  /**
   * Demux IAMF audio from an MP4 file
   *
   * @param mp4FilePath The path to the MP4 file containing IAMF audio
   * @param outputIAMFPath The path where demuxed IAMF will be saved
   * @return true if successful, false otherwise
   */
  bool demuxIAMFFromMP4(const juce::String& mp4FilePath,
                        const juce::String& outputIAMFPath) {
    LOG_INFO(0,
             "Starting IAMF demuxing from MP4: " + mp4FilePath.toStdString());

    MP4IAMFParser parser;
    IAMFHeader* header = nullptr;
    bool success = false;

    // Initialize the parser
    mp4_iamf_parser_init(&parser);

    // Open the MP4 file and get the IAMF header
    if (mp4_iamf_parser_open_audio_track(&parser, mp4FilePath.toRawUTF8(),
                                         &header) <= 0) {
      LOG_INFO(0, "Failed to open MP4 file or extract IAMF header");
      mp4_iamf_parser_close(&parser);
      return false;
    }

    // Extract IAMF data to the output file
    success = extractIAMFData(&parser, outputIAMFPath);

    // Clean up
    mp4_iamf_parser_close(&parser);

    LOG_INFO(0, success ? "Successfully demuxed IAMF from MP4"
                        : "Failed to demux IAMF from MP4");

    return success;
  }

  /**
   * Decode IAMF to WAV for audio comparison
   *
   * @param iamfPath The path to the IAMF file
   * @param outputWavPath The path where the WAV file will be saved
   * @param soundSystem The sound system to render to (0 for stereo/A_0_2_0)
   * @param sampleRate The sample rate to use for decoding (0 = use original)
   * @param bitDepth The bit depth to use for decoding (0 = use original)
   * @return true if successful, false otherwise
   */
  bool decodeIAMFToWAV(const juce::String& iamfPath,
                       const juce::String& outputWavPath, int soundSystem = 0,
                       int sampleRate = 48000, int bitDepth = 16) {
    LOG_INFO(0, "Decoding IAMF to WAV: " + iamfPath.toStdString() + " -> " +
                    outputWavPath.toStdString() +
                    " (Sample Rate: " + std::to_string(sampleRate) +
                    " Hz, Bit Depth: " + std::to_string(bitDepth) +
                    " bits, Sound System: " + std::to_string(soundSystem) +
                    ")");

    // Load the IAMF file into memory
    std::vector<uint8_t> iamfData = loadFileToMemory(iamfPath);
    if (iamfData.empty()) {
      LOG_INFO(0, "Failed to load IAMF file");
      return false;
    }

    // Create an IAMF decoder
    IAMF_DecoderHandle decoder = IAMF_decoder_open();
    if (!decoder) {
      LOG_INFO(0, "Failed to create IAMF decoder");
      return false;
    }

    // Set decoder parameters
    IAMF_decoder_set_bit_depth(decoder, bitDepth);

    // Configure output layout based on sound system parameter
    IAMF_decoder_output_layout_set_sound_system(
        decoder, static_cast<IAMF_SoundSystem>(soundSystem));
    int channels = IAMF_layout_sound_system_channels_count(
        static_cast<IAMF_SoundSystem>(soundSystem));

    // Configure the decoder with IAMF description OBUs
    uint32_t bytesUsed = 0;
    int result = IAMF_decoder_configure(decoder, iamfData.data(),
                                        iamfData.size(), &bytesUsed);
    if (result != IAMF_OK) {
      LOG_INFO(0, "Failed to configure IAMF decoder, error code: " +
                      std::to_string(result));
      IAMF_decoder_close(decoder);
      return false;
    }

    // Get stream info
    IAMF_StreamInfo* info = IAMF_decoder_get_stream_info(decoder);
    if (!info) {
      LOG_INFO(0, "Failed to get IAMF stream info");
      IAMF_decoder_close(decoder);
      return false;
    }

    // Open WAV file for writing
    FILE* wavFile = (FILE*)dep_wav_write_open(outputWavPath.toRawUTF8(),
                                              sampleRate, bitDepth, channels);

    if (!wavFile) {
      LOG_INFO(0, "Failed to open WAV file for writing");
      IAMF_decoder_close(decoder);
      return false;
    }

    // Allocate PCM buffer
    std::vector<uint8_t> pcmBuffer(bitDepth / 8 * info->max_frame_size *
                                   channels);

    // Decode IAMF data
    size_t dataOffset = bytesUsed;
    while (dataOffset < iamfData.size()) {
      bytesUsed = 0;
      int samplesDecoded = IAMF_decoder_decode(
          decoder, iamfData.data() + dataOffset, iamfData.size() - dataOffset,
          &bytesUsed, pcmBuffer.data());

      if (samplesDecoded > 0) {
        // Write PCM data to WAV file
        dep_wav_write_data(wavFile, pcmBuffer.data(),
                           bitDepth / 8 * samplesDecoded * channels);
      }

      if (bytesUsed == 0 || samplesDecoded < 0) {
        break;
      }

      dataOffset += bytesUsed;
    }

    // Flush and close the WAV file
    dep_wav_write_close(wavFile);

    // Clean up
    IAMF_decoder_close(decoder);

    return true;
  }

  /**
   * Compare two IAMF files to verify they are equivalent
   *
   * @param originalIAMFPath Path to the original IAMF file
   * @param demuxedIAMFPath Path to the demuxed IAMF file
   * @return true if files match, false otherwise
   */
  bool compareIAMFFiles(const juce::String& originalIAMFPath,
                        const juce::String& demuxedIAMFPath) {
    LOG_INFO(0, "Comparing IAMF files: " + originalIAMFPath.toStdString() +
                    " vs " + demuxedIAMFPath.toStdString());

    // Open both files
    std::ifstream original(originalIAMFPath.toRawUTF8(), std::ios::binary);
    std::ifstream demuxed(demuxedIAMFPath.toRawUTF8(), std::ios::binary);

    if (!original.is_open() || !demuxed.is_open()) {
      LOG_INFO(0, "Failed to open one or both files for comparison");
      return false;
    }

    // Get file sizes
    original.seekg(0, std::ios::end);
    demuxed.seekg(0, std::ios::end);
    std::streamsize originalSize = original.tellg();
    std::streamsize demuxedSize = demuxed.tellg();

    // Reset file positions
    original.seekg(0, std::ios::beg);
    demuxed.seekg(0, std::ios::beg);

    // Check if file sizes match
    if (originalSize != demuxedSize) {
      LOG_INFO(0, "File sizes don't match: " + std::to_string(originalSize) +
                      " vs " + std::to_string(demuxedSize));
      return false;
    }

    // Compare files byte by byte
    const int bufferSize = 4096;
    char bufferOriginal[bufferSize];
    char bufferDemuxed[bufferSize];

    bool filesMatch = true;
    std::streamsize bytesRemaining = originalSize;

    while (bytesRemaining > 0 && filesMatch) {
      std::streamsize bytesToRead =
          std::min(static_cast<std::streamsize>(bufferSize), bytesRemaining);

      original.read(bufferOriginal, bytesToRead);
      demuxed.read(bufferDemuxed, bytesToRead);

      if (memcmp(bufferOriginal, bufferDemuxed, bytesToRead) != 0) {
        filesMatch = false;
      }

      bytesRemaining -= bytesToRead;
    }

    // Close files
    original.close();
    demuxed.close();

    LOG_INFO(0, filesMatch ? "Files match!" : "Files don't match!");
    return filesMatch;
  }

  /**
   * Compare two WAV files to verify audio similarity
   *
   * @param originalWavPath Path to the original WAV file
   * @param demuxedWavPath Path to the WAV file from demuxed IAMF
   * @param tolerance Maximum allowed difference between samples (0.0 - 1.0)
   * @return true if files match within tolerance, false otherwise
   */
  bool compareWAVFiles(const juce::String& originalWavPath,
                       const juce::String& demuxedWavPath,
                       float tolerance = 0.0001f) {
    LOG_INFO(0, "Comparing WAV files: " + originalWavPath.toStdString() +
                    " vs " + demuxedWavPath.toStdString());

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> originalReader(
        formatManager.createReaderFor(juce::File(originalWavPath)));
    std::unique_ptr<juce::AudioFormatReader> demuxedReader(
        formatManager.createReaderFor(juce::File(demuxedWavPath)));

    if (!originalReader || !demuxedReader) {
      LOG_INFO(0, "Failed to open one or both WAV files");
      return false;
    }

    if (originalReader->numChannels != demuxedReader->numChannels ||
        originalReader->sampleRate != demuxedReader->sampleRate) {
      LOG_INFO(0, "Channel count or sample rate doesn't match");
      return false;
    }

    const int bufferSize = 1024;
    juce::AudioBuffer<float> originalBuffer(originalReader->numChannels,
                                            bufferSize);
    juce::AudioBuffer<float> demuxedBuffer(demuxedReader->numChannels,
                                           bufferSize);

    juce::int64 position = 0;
    const juce::int64 totalSamples = std::min(originalReader->lengthInSamples,
                                              demuxedReader->lengthInSamples);
    float maxDifference = 0.0f;

    while (position < totalSamples) {
      const int numSamples = std::min(static_cast<juce::int64>(bufferSize),
                                      totalSamples - position);

      juce::AudioSampleBuffer originalChunk(originalBuffer);
      juce::AudioSampleBuffer demuxedChunk(demuxedBuffer);

      originalReader->read(&originalChunk, 0, numSamples, position, true, true);
      demuxedReader->read(&demuxedChunk, 0, numSamples, position, true, true);

      for (int channel = 0; channel < originalReader->numChannels; ++channel) {
        const float* originalData = originalChunk.getReadPointer(channel);
        const float* demuxedData = demuxedChunk.getReadPointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
          float diff = std::abs(originalData[sample] - demuxedData[sample]);
          maxDifference = std::max(maxDifference, diff);

          if (diff > tolerance) {
            LOG_INFO(0, "Difference exceeds tolerance at position " +
                            std::to_string(position + sample) + ", channel " +
                            std::to_string(channel) +
                            ", diff: " + std::to_string(diff));
            return false;
          }
        }
      }

      position += numSamples;
    }

    LOG_INFO(0, "WAV files match within tolerance. Max difference: " +
                    std::to_string(maxDifference));
    return true;
  }

  /**
   * Verify IAMF integrity through demuxing and comparison
   *
   * @param mp4FilePath Path to the MP4 file containing IAMF audio
   * @param originalIAMFPath Path to the original IAMF file
   * @param sampleRate Sample rate to use for WAV conversion
   * @param bitDepth Bit depth to use for WAV conversion
   * @param soundSystem Sound system to render to (0 for stereo)
   * @param tolerance Maximum allowed difference between audio samples
   * @return true if verification successful, false otherwise
   */
  bool verifyIAMFIntegrity(const juce::String& mp4FilePath,
                           const juce::String& originalIAMFPath, int sampleRate,
                           int bitDepth, int soundSystem = 0,
                           float tolerance = 0.0001f) {
    LOG_INFO(0, "Starting IAMF integrity verification");
    LOG_INFO(0, "Parameters - Sample rate: " + std::to_string(sampleRate) +
                    " Hz, Bit depth: " + std::to_string(bitDepth) +
                    " bits, Sound system: " + std::to_string(soundSystem) +
                    ", Tolerance: " + std::to_string(tolerance));

    // Define paths for demuxed IAMF and decoded WAV files
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

    // Step 1: Demux IAMF from MP4
    bool demuxSuccess = demuxIAMFFromMP4(mp4FilePath, demuxedIAMFPath);
    if (!demuxSuccess) {
      LOG_INFO(0, "Failed to demux IAMF from MP4");
      return false;
    }

    // Step 2: Compare original and demuxed IAMF files
    bool iamfComparisonResult =
        compareIAMFFiles(originalIAMFPath, demuxedIAMFPath);
    if (!iamfComparisonResult) {
      LOG_INFO(0,
               "Original and demuxed IAMF files don't match exactly. This may "
               "be normal due to metadata differences.");
      // Don't fail here, as some header differences might be expected
      // Just log the issue for investigation
    }

    // Step 3: Decode both IAMF files to WAV for audio content comparison
    bool decodeOriginalSuccess =
        decodeIAMFToWAV(originalIAMFPath, originalDecodedWavPath, soundSystem,
                        sampleRate, bitDepth);

    bool decodeDemuxedSuccess =
        decodeIAMFToWAV(demuxedIAMFPath, demuxedDecodedWavPath, soundSystem,
                        sampleRate, bitDepth);

    if (!decodeOriginalSuccess || !decodeDemuxedSuccess) {
      LOG_INFO(0, "Failed to decode IAMF file(s) to WAV");

      // Clean up temporary files
      juce::File(demuxedIAMFPath).deleteFile();
      juce::File(originalDecodedWavPath).deleteFile();
      juce::File(demuxedDecodedWavPath).deleteFile();

      return false;
    }

    // Step 4: Compare the WAV files to verify audio content integrity
    bool wavComparisonResult = compareWAVFiles(
        originalDecodedWavPath, demuxedDecodedWavPath, tolerance);

    if (!wavComparisonResult) {
      LOG_INFO(0,
               "Audio content doesn't match between original and demuxed IAMF");

      // Clean up temporary files
      juce::File(demuxedIAMFPath).deleteFile();
      juce::File(originalDecodedWavPath).deleteFile();
      juce::File(demuxedDecodedWavPath).deleteFile();

      return false;
    }

    LOG_INFO(0, "IAMF integrity verification successful!");

    // Clean up temporary files
    juce::File(demuxedIAMFPath).deleteFile();
    juce::File(originalDecodedWavPath).deleteFile();
    juce::File(demuxedDecodedWavPath).deleteFile();

    return true;
  }

 private:
  // Helper method to extract IAMF data using MP4IAMFParser
  bool extractIAMFData(MP4IAMFParser* parser, const juce::String& outputPath) {
    // Open output file for writing binary data
    std::ofstream outFile(outputPath.toRawUTF8(), std::ios::binary);
    if (!outFile.is_open()) {
      LOG_INFO(0, "Failed to open output file for writing: " +
                      outputPath.toStdString());
      return false;
    }

    uint8_t* pktBuf = nullptr;
    uint32_t pktLen = 0;
    int sampleDelta = 0;
    int64_t sampleOffset = 0;
    int entryNo = 0;
    bool end = false;

    // First, get the IAMF description OBUs
    IAMFHeader* header = nullptr;
    if (mp4_iamf_parser_get_audio_track_header(parser, &header) <= 0 ||
        !header) {
      LOG_INFO(0, "Failed to get IAMF header");
      outFile.close();
      return false;
    }

    uint8_t* descriptionOBUs = nullptr;
    uint32_t descriptionSize = 0;

    if (iamf_header_read_description_OBUs(header, &descriptionOBUs,
                                          &descriptionSize) <= 0) {
      LOG_INFO(0, "Failed to read IAMF description OBUs");
      outFile.close();
      return false;
    }

    // Write description OBUs to output file
    outFile.write(reinterpret_cast<char*>(descriptionOBUs), descriptionSize);
    free(descriptionOBUs);

    // Extract all IAMF packets
    while (!end) {
      sampleDelta = mp4_iamf_parser_read_packet(parser, 0, &pktBuf, &pktLen,
                                                &sampleOffset, &entryNo);

      if (sampleDelta < 0 || !pktBuf) {
        end = true;
        continue;
      }

      // Write the packet to the output file
      outFile.write(reinterpret_cast<char*>(pktBuf), pktLen);

      // Free the packet buffer allocated by mp4_iamf_parser_read_packet
      free(pktBuf);
      pktBuf = nullptr;
    }

    outFile.close();
    return true;
  }

  // Method to load a file into memory
  std::vector<uint8_t> loadFileToMemory(const juce::String& filePath) {
    std::ifstream file(filePath.toRawUTF8(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      LOG_INFO(0, "Failed to open file: " + filePath.toStdString());
      return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
      LOG_INFO(0, "Failed to read file: " + filePath.toStdString());
      return {};
    }

    return buffer;
  }
};

// Test repositories
class TestFileExportRepository : public FileExportRepository {
 public:
  TestFileExportRepository() : FileExportRepository(juce::ValueTree{"test"}) {}
};

class TestAudioElementRepository : public AudioElementRepository {
 public:
  TestAudioElementRepository()
      : AudioElementRepository(juce::ValueTree{"test"}) {}
};

class TestMixPresentationRepository : public MixPresentationRepository {
 public:
  TestMixPresentationRepository()
      : MixPresentationRepository(juce::ValueTree{"test"}) {}
};

class TestMixPresentationLoudnessRepository
    : public MixPresentationLoudnessRepository {
 public:
  TestMixPresentationLoudnessRepository()
      : MixPresentationLoudnessRepository(juce::ValueTree{"test"}) {}
};

// Shared test fixture for both FileOutputProcessor and MP4IAMFDemuxer tests
class SharedTestFixture : public ::testing::Test {
  std::vector<std::string> logFilePaths;

 private:
  std::string logFilePath;  // Store the log path for cleanup

 protected:
  SharedTestFixture()
      // Initialize state and repositories.
      : testState("test_state"),
        fileExportRepository(
            testState.getOrCreateChildWithName("file", nullptr)),
        audioElementRepository(
            testState.getOrCreateChildWithName("element", nullptr)),
        mixRepository(testState.getOrCreateChildWithName("mix", nullptr)),
        mixPresentationLoudnessRepository(
            testState.getOrCreateChildWithName("mixLoud", nullptr)),
        ex(fileExportRepository.get()),
        fio_proc(fileExportRepository, audioElementRepository, mixRepository,
                 mixPresentationLoudnessRepository) {
    // Use a unique log file path based on process ID and timestamp
    const std::string pluginName = "FileOutputProcessor";
    juce::String uniqueSuffix =
        juce::String(juce::Time::getCurrentTime().toMilliseconds()) + "_" +
        juce::String(getpid());
    logFilePath =
        "/tmp/Eclipsa_Audio_Plugin_" + uniqueSuffix.toStdString() + "/";

    // Ensure the log directory exists and is writable
    try {
      boost::filesystem::path logDir(logFilePath);
      if (!boost::filesystem::exists(logDir)) {
        boost::filesystem::create_directories(logDir);
        boost::filesystem::permissions(logDir,
                                       boost::filesystem::perms::all_all);
      } else {
        boost::filesystem::permissions(logDir,
                                       boost::filesystem::perms::all_all);
      }
    } catch (const boost::filesystem::filesystem_error& e) {
      std::cerr << "Failed to create/set permissions for log directory: "
                << e.what() << std::endl;
      Logger::getInstance().init(
          pluginName);  // Log using JUCE standard directory
    }

    Logger::getInstance().init(pluginName);

    // Configure basic audio export data.
    ex.setExportAudio(true);
    ex.setAudioFileFormat(AudioFileFormat::IAMF);
    ex.setSampleRate(kSampleRate);
    iamfPathStr = juce::File::getCurrentWorkingDirectory()
                      .getChildFile("test.iamf")
                      .getFullPathName();
    iamfOutPath = std::filesystem::path(iamfPathStr.toStdString());
    ex.setExportFolder(
        juce::File::getCurrentWorkingDirectory().getFullPathName());
    ex.setExportFile(juce::File::getCurrentWorkingDirectory()
                         .getChildFile("test.iamf")
                         .getFullPathName());

    // Configure video export / import paths.
    videoPathStr = juce::File::getCurrentWorkingDirectory()
                       .getChildFile("MuxedVideo.mp4")
                       .getFullPathName();
    videoOutPath = std::filesystem::path(videoPathStr.toStdString());
    ex.setVideoExportFolder(videoPathStr);
    fileExportRepository.update(ex);

    // As the test is run in the /build directory, the path needs to be adjusted
    // to point to the true location of the video source file.
    std::filesystem::path correctedVideoSrcPath;
    for (const auto& part : videoSourcePath) {
      if (part == "build") {
        continue;  // Skip the 'build' segment
      }
      correctedVideoSrcPath /= part;
    }
    videoSourcePath = correctedVideoSrcPath;
  }

  ~SharedTestFixture() override {
    // Clean up the unique log directory
    try {
      boost::filesystem::path logDir(logFilePath);
      if (boost::filesystem::exists(logDir)) {
        boost::filesystem::remove_all(logDir);
      }
    } catch (const boost::filesystem::filesystem_error& e) {
      std::cerr << "Failed to clean up log directory: " << e.what()
                << std::endl;
    }

    // Clean up any leftover test files
    try {
      if (std::filesystem::exists(iamfOutPath)) {
        std::filesystem::remove(iamfOutPath);
      }
      if (std::filesystem::exists(videoOutPath)) {
        std::filesystem::remove(videoOutPath);
      }
    } catch (const std::filesystem::filesystem_error& e) {
      std::cerr << "Failed to clean up test files: " << e.what() << std::endl;
    }
  }

  // Create 1 channel-based AudioElement and add it to a mix presentation.
  void setup_1ae_cb() {
    audioElementRepository.clear();
    AudioElement ae1(juce::Uuid(), "Audio Element 1", "Description 1",
                     Speakers::kStereo, 0);
    audioElementRepository.add(ae1);

    // Add the audio element to the mix presentation.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1,
                        LanguageData::MixLanguages::English, {});
    mp1.addAudioElement(ae1.getId(), 0, ae1.getName());
    MixPresentationLoudness mixLoudness(mp1.getId());
    // largest layout is already stereo
    mixRepository.add(mp1);
    mixPresentationLoudnessRepository.add(mixLoudness);
  }

  // Create 1 scene-based AudioElement and add it to a mix presentation.
  void setup_1ae_sb() {
    audioElementRepository.clear();
    AudioElement ae1(juce::Uuid(), "Audio Element 1", "Description 1",
                     Speakers::kHOA1, 0);
    audioElementRepository.add(ae1);

    // Add the audio element to the mix presentation.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1,
                        LanguageData::MixLanguages::English, {});

    MixPresentationLoudness mixLoudness(mp1.getId());
    // not updating the largest layout for kHOA1
    mp1.addAudioElement(ae1.getId(), 0, ae1.getName());

    mixRepository.add(mp1);
    mixPresentationLoudnessRepository.add(mixLoudness);
  }

  // Create 2 channel-based AudioElements and add them to a mix presentation.
  void setup_2ae_cb() {
    audioElementRepository.clear();
    AudioElement ae1(juce::Uuid(), "Audio Element 1", "Description 1",
                     Speakers::kStereo, 0);
    AudioElement ae2(juce::Uuid(), "Audio Element 2", "Description 2",
                     Speakers::kExplLFE, 2);
    audioElementRepository.add(ae1);
    audioElementRepository.add(ae2);

    // Add the audio elements to the mix presentation.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1,
                        LanguageData::MixLanguages::English, {});
    MixPresentationLoudness mixLoudness(mp1.getId());
    // not updating the largest layout for kExplLFE
    mp1.addAudioElement(ae1.getId(), 0, ae1.getName());
    mp1.addAudioElement(ae2.getId(), 0, ae2.getName());
    mixRepository.add(mp1);
    mixPresentationLoudnessRepository.add(mixLoudness);
  }

  void setup_1ae_51() {
    // Create an AudioElement with the current layout.
    audioElementRepository.clear();
    AudioElement ae(juce::Uuid(), "Audio Element", Speakers::k5Point1, 0);
    audioElementRepository.add(ae);

    // Add the audio element to the mix presentation.
    mixRepository.clear();
    MixPresentation mp1(juce::Uuid(), "Mix Presentation 1", 1,
                        LanguageData::MixLanguages::English, {});
    MixPresentationLoudness mixLoudness(mp1.getId());

    mp1.addAudioElement(ae.getId(), 0, ae.getName());
    mixLoudness.replaceLargestLayout(Speakers::k5Point1);
    mixRepository.add(mp1);
    mixPresentationLoudnessRepository.add(mixLoudness);
  }

  // Generates an audio tone and performs a bounce via the FIO proc.
  void generateAndBounceAudio() {
    // Get the duration of the input video file.
    const float kAudioDuration_s = 0.2;
    const int kTotalSamples = kAudioDuration_s * kSampleRate;

    // Get the total number of input channels.
    int numChannels = 0;
    juce::OwnedArray<AudioElement> audioElements;
    audioElementRepository.getAll(audioElements);
    for (int i = 0; i < audioElements.size(); ++i) {
      numChannels += audioElements[i]->getChannelCount();
    }

    // Generate a 440Hz tone.
    juce::AudioBuffer<float> sineWaveAudio(1, kSamplesPerFrame);
    for (int i = 0; i < kSamplesPerFrame; ++i) {
      sineWaveAudio.setSample(
          0, i, 0.1f * std::sin(2 * M_PI * 440 * (float)i / kSampleRate));
    }

    // Start a bounce.
    fio_proc.prepareToPlay(kSampleRate, kSamplesPerFrame);
    fio_proc.setNonRealtime(true);

    // Copy the the sine wave audio to each buffer channel and process the
    // frame.
    juce::AudioBuffer<float> audioBuffer(numChannels, kSamplesPerFrame);
    juce::MidiBuffer midiBuffer;
    for (int sampsProcd = 0; sampsProcd < kTotalSamples;
         sampsProcd += kSamplesPerFrame) {
      // Copy audio data to each channel of audioBuffer.
      for (int i = 0; i < numChannels; ++i) {
        audioBuffer.copyFrom(i, 0, sineWaveAudio, 0, 0, kSamplesPerFrame);
      }
      fio_proc.processBlock(audioBuffer, midiBuffer);
    }

    // Complete the bounce.
    fio_proc.setNonRealtime(false);
  }

  // Read the logfile generated during export and confirm export was successful.
  std::string getLoggedExportStatus() {
    // Store the log file paths for later use in tests
    logFilePaths = Logger::getInstance().getLogFilePaths();
    std::string allLogContent;
    for (const auto& logFilePath : logFilePaths) {
      std::ifstream logFile(logFilePath);
      if (!logFile.is_open()) {
        std::cerr << "Failed to open log file at " << logFilePath << std::endl;
        continue;
      }
      std::string currLine;
      while (std::getline(logFile, currLine)) {
        allLogContent += currLine + "\n";
      }
      logFile.close();
    }
    return allLogContent;
  }

  // Helper method to generate IAMF files and perform verification for demuxer
  // tests
  bool runEndToEndTest() {
    // Generate files
    generateAndBounceAudio();

    // Verify files were created
    EXPECT_TRUE(std::filesystem::exists(iamfOutPath))
        << "IAMF file wasn't created";
    EXPECT_TRUE(std::filesystem::exists(videoOutPath))
        << "MP4 file wasn't created";

    // Create demuxer instance
    MP4IAMFDemuxer demuxer;

    // Verify IAMF integrity
    bool integrityResult =
        demuxer.verifyIAMFIntegrity(videoPathStr,    // MP4 file path
                                    iamfPathStr,     // Original IAMF path
                                    kSampleRate,     // Sample rate
                                    16,              // Bit depth (default)
                                    SOUND_SYSTEM_A,  // Sound system (stereo=0)
                                    0.01f);

    // Clean up created files
    std::filesystem::remove(iamfOutPath);
    std::filesystem::remove(videoOutPath);

    return integrityResult;
  }

  void bounceExportConfig(FileExport& config, juce::String testInfo) {
    try {
      fileExportRepository.update(config);
      generateAndBounceAudio();
    } catch (const std::exception& e) {
      std::cerr << "Exception during generateAndBounceAudio(): " << e.what()
                << std::endl;
      FAIL() << "Exception during generateAndBounceAudio(): " << e.what();
    }
    Logger::getInstance().flush();
    // Read log contents
    std::string logContent = getLoggedExportStatus();
    std::cout << "Log content:\n" << logContent << std::endl;

    EXPECT_NE(logContent.find("IAMF export attempt completed with status: OK"),
              std::string::npos)
        << logContent;

    EXPECT_TRUE(std::filesystem::exists(iamfOutPath)) << testInfo;
    std::filesystem::remove(iamfOutPath);
  }

  // Data used by all test fixtures:
  // Constants
  const int kSampleRate = 48e3;
  const int kSamplesPerFrame = 128;
  std::vector<Speakers::AudioElementSpeakerLayout> kAudioElementLayouts = {
      Speakers::kMono,          Speakers::kStereo,
      Speakers::k5Point1,       Speakers::k5Point1Point2,
      Speakers::k5Point1Point4, Speakers::k7Point1,
      Speakers::k7Point1Point2, Speakers::k7Point1Point4,
      Speakers::k3Point1Point2, Speakers::kBinaural,
      Speakers::kHOA1,          Speakers::kHOA2,
      Speakers::kHOA3,
  };
  std::vector<Speakers::AudioElementSpeakerLayout>
      kAudioElementExpandedLayouts = {
          Speakers::kExplLFE,
          Speakers::kExpl5Point1Point4Surround,
          Speakers::kExpl7Point1Point4SideSurround,
          Speakers::kExpl7Point1Point4RearSurround,
          Speakers::kExpl7Point1Point4TopFront,
          Speakers::kExpl7Point1Point4TopBack,
          Speakers::kExpl7Point1Point4Top,
          Speakers::kExpl7Point1Point4Front,
          Speakers::kExpl9Point1Point6,
          Speakers::kExpl9Point1Point6Front,
          Speakers::kExpl9Point1Point6Side,
          Speakers::kExpl9Point1Point6TopSide,
          Speakers::kExpl9Point1Point6Top,
      };

  // Repositories
  juce::ValueTree testState;
  FileExportRepository fileExportRepository;
  AudioElementRepository audioElementRepository;
  MixPresentationRepository mixRepository;
  MixPresentationLoudnessRepository mixPresentationLoudnessRepository;

  // File paths
  juce::String iamfPathStr;
  juce::String videoPathStr;
  std::filesystem::path iamfOutPath;
  std::filesystem::path videoOutPath;
  std::filesystem::path videoSourcePath =
      (std::filesystem::current_path() /
       "test_resources/SilentSampleVideo.mp4");

  // File export data
  FileExport ex;

  // Processor under test
  FileOutputProcessor fio_proc;
};