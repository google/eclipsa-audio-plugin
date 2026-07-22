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

#include "FileExport.h"

#include <filesystem>

FileExport::FileExport()
    : RepositoryItemBase({}),
      startSampleIdx_(0),
      endSampleIdx_(0),
      exportFile_(""),
      exportFolder_(""),
      audioFileFormat_(IAMF),
      audioCodec_(LPCM),
      bitDepth_(16),
      sampleRate_(16000),
      exportAudioElements_(false),
      exportAudio_(false),
      exportVideo_(false),
      videoSource_(""),
      videoExportFolder_(""),
      manualExport_(false),
      flac_compression_level_(8),
      opus_total_bitrate_(64000),
      lpcm_sample_size_(24),
      sample_tally_(0),
      profile_(FileProfile::BASE),
      exportCompleted_(true),
      securityBookmark_(""),
      exportError_(kNoError),
      videoLongerThanAudio_(false) {}

FileExport::FileExport(long startSampleIdx, long endSampleIdx,
                       juce::String exportFile, juce::String exportFolder,
                       AudioFileFormat audioFileFormat, AudioCodec audioCodec,
                       int bitDepth, int sampleRate, bool exportAudioElements,
                       bool exportAudio, bool exportVideo,
                       juce::String videoSource, juce::String videoExportFolder,
                       bool manualExport, FileProfile profile,
                       int flac_compression_level, int opus_total_bitrate,
                       int lpcm_sample_size, bool exportCompleted,
                       juce::String securityBookmark)
    : RepositoryItemBase(juce::Uuid()),
      startSampleIdx_(startSampleIdx),
      endSampleIdx_(endSampleIdx),
      exportFile_(exportFile),
      exportFolder_(exportFolder),
      audioFileFormat_(audioFileFormat),
      audioCodec_(audioCodec),
      bitDepth_(bitDepth),
      sampleRate_(sampleRate),
      exportAudioElements_(exportAudioElements),
      exportAudio_(exportAudio),
      exportVideo_(exportVideo),
      videoSource_(videoSource),
      videoExportFolder_(videoExportFolder),
      manualExport_(manualExport),
      profile_(profile),
      flac_compression_level_(flac_compression_level),
      opus_total_bitrate_(opus_total_bitrate),
      lpcm_sample_size_(lpcm_sample_size),
      sample_tally_(0),
      exportCompleted_(exportCompleted),
      securityBookmark_(securityBookmark),
      exportError_(kNoError),
      videoLongerThanAudio_(false) {}

FileExport FileExport::fromTree(const juce::ValueTree tree) {
  FileExport result(
      (juce::int64)tree[kStartSampleIdx], (juce::int64)tree[kEndSampleIdx],
      tree[kExportFile], tree[kExportFolder],
      (AudioFileFormat)(int)tree[kAudioFileFormat],
      (AudioCodec)(int)tree[kAudioCodec], tree[kBitDepth], tree[kSampleRate],
      tree[kExportAudioElements], tree[kExportAudio], tree[kExportVideo],
      tree[kVideoSource], tree[kVideoExportFolder], tree[kManualExport],
      (FileProfile)(int)tree[kProfile], tree[kFlacCompressionLevel],
      tree[kOpusTotalBitrate], tree[kLPCMSampleSize], tree[kExportCompleted],
      tree[kSecurityBookmark]);
  result.setExportError((ExportError)(int)tree[kExportError]);
  result.setVideoLongerThanAudio((bool)tree[kVideoLongerThanAudio]);
  return result;
}

juce::ValueTree FileExport::toValueTree() const {
  return {kTreeType,
          {{kStartSampleIdx, static_cast<juce::int64>(startSampleIdx_)},
           {kEndSampleIdx, static_cast<juce::int64>(endSampleIdx_)},
           {kExportFile, exportFile_},
           {kExportFolder, exportFolder_},
           {kAudioFileFormat, audioFileFormat_},
           {kAudioCodec, audioCodec_},
           {kBitDepth, bitDepth_},
           {kSampleRate, sampleRate_},
           {kExportAudioElements, exportAudioElements_},
           {kExportAudio, exportAudio_},
           {kExportVideo, exportVideo_},
           {kVideoSource, videoSource_},
           {kVideoExportFolder, videoExportFolder_},
           {kManualExport, manualExport_},
           {kProfile, static_cast<int>(profile_)},
           {kFlacCompressionLevel, flac_compression_level_},
           {kOpusTotalBitrate, opus_total_bitrate_},
           {kLPCMSampleSize, lpcm_sample_size_},
           {kSampleTally, static_cast<juce::int64>(sample_tally_)},
           {kExportCompleted, exportCompleted_},
           {kSecurityBookmark, securityBookmark_},
           {kExportError, static_cast<int>(exportError_)},
           {kVideoLongerThanAudio, videoLongerThanAudio_}}};
}

juce::String FileExport::expandTildePath(const juce::String& path) {
  if (path.startsWith("~")) {
    juce::File homeDir =
        juce::File::getSpecialLocation(juce::File::userHomeDirectory);
    return homeDir.getFullPathName() + path.substring(1);
  }
  return path;
}

bool FileExport::validateFilePath(const std::filesystem::path& path,
                                  const bool sourceFile) {
  if (path.empty()) {
    return false;
  }
  if (sourceFile) {
    return std::filesystem::exists(path);
  } else {
    return std::filesystem::exists(path.parent_path());
  }
}