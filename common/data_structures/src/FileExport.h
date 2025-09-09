/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <juce_data_structures/juce_data_structures.h>

#include "data_structures/src/RepositoryItem.h"

enum AudioFileFormat { IAMF = 0, WAV = 1, ADM = 2 };

enum AudioCodec { LPCM = 0, FLAC = 1, OPUS = 2 };

// Using a macro here to help minimize the amount of code
//  If we need custom getters/setters we can
//  break this macro up into variable creation and getter/setter creation
//  and then only define the getter/setter when default getter/setters are
//  needed
#define EXPORT_VALUE(type, x, y)          \
 private:                                 \
  type x##_;                              \
                                          \
 public:                                  \
  void set##y(const type x) { x##_ = x; } \
  type get##y() const { return x##_; }    \
  inline static const juce::Identifier k##y{#x};

enum FileProfile { SIMPLE, BASE, BASE_ENHANCED };

class FileProfileHelper {
 public:
  // Helper function to determine the number of channels in a profile
  static int profileChannels(FileProfile profile) {
    switch (profile) {
      case SIMPLE:
        return 16;
      case BASE:
        return 18;
      case BASE_ENHANCED:
        return 28;
    }
  }

  // Helper function to determine the number of audio elements in a profile
  static int profileAudioElements(FileProfile profile) {
    switch (profile) {
      case SIMPLE:
        return 1;
      case BASE:
        return 2;
      case BASE_ENHANCED:
        return 28;
    }
  }

  // Helper function to determine the minimum profile for a given number of
  // channels and audio elements
  static FileProfile minimumProfile(int numChannels, int numAudioElements) {
    if (numChannels <= 16 && numAudioElements <= 1) {
      return SIMPLE;
    } else if (numChannels <= 18 && numAudioElements <= 2) {
      return BASE;
    } else {
      return BASE_ENHANCED;
    }
  }
};

class FileExport final : public RepositoryItemBase {
 public:
  FileExport();
  FileExport(int startTime, int endTime, juce::String exportFile,
             juce::String exportFolder, AudioFileFormat audioFileFormat,
             AudioCodec audioCodec, int bitDepth, int sampleRate,
             bool exportAudioElements, bool exportAudio, bool exportVideo,
             juce::String videoSource, juce::String videoExportFolder,
             bool manualExport, FileProfile profile, int flac_compression_level,
             int opus_total_bitrate, int lpcm_sample_size);

  ~FileExport() = default;

  static FileExport fromTree(const juce::ValueTree tree);
  virtual juce::ValueTree toValueTree() const override;

  inline static const juce::Identifier kTreeType{"file_export"};

 private:
  EXPORT_VALUE(int, startTime, StartTime);
  EXPORT_VALUE(int, endTime, EndTime);
  EXPORT_VALUE(juce::String, exportFile, ExportFile);
  EXPORT_VALUE(juce::String, exportFolder, ExportFolder);
  EXPORT_VALUE(AudioFileFormat, audioFileFormat, AudioFileFormat);
  EXPORT_VALUE(AudioCodec, audioCodec, AudioCodec);
  EXPORT_VALUE(int, bitDepth, BitDepth);
  EXPORT_VALUE(int, sampleRate, SampleRate);
  EXPORT_VALUE(bool, exportAudioElements, ExportAudioElements);
  EXPORT_VALUE(bool, exportAudio, ExportAudio);
  EXPORT_VALUE(bool, exportVideo, ExportVideo);
  EXPORT_VALUE(juce::String, videoSource, VideoSource);
  EXPORT_VALUE(juce::String, videoExportFolder, VideoExportFolder);
  EXPORT_VALUE(bool, manualExport, ManualExport);
  EXPORT_VALUE(FileProfile, profile, Profile);
  EXPORT_VALUE(int, flac_compression_level, FlacCompressionLevel);
  EXPORT_VALUE(int, opus_total_bitrate, OpusTotalBitrate);
  EXPORT_VALUE(int, lpcm_sample_size, LPCMSampleSize);
  EXPORT_VALUE(long, sample_tally, SampleTally);
};
