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

#include "IAMFExportUtil.h"

#include <gpac/filters.h>
#include <gpac/isomedia.h>
#include <gpac/tools.h>
#include <logger/logger.h>

#include "gpac/tools.h"

namespace IAMFExportHelper {

void writeIASeqHdr(FileProfile profileVersion,
                   iamf_tools_cli_proto::UserMetadata& userMetadata) {
  auto iaSeqHdr = userMetadata.add_ia_sequence_header_metadata();

  if (profileVersion == FileProfile::SIMPLE) {
    iaSeqHdr->set_primary_profile(iamf_tools_cli_proto::PROFILE_VERSION_SIMPLE);
    iaSeqHdr->set_additional_profile(
        iamf_tools_cli_proto::PROFILE_VERSION_SIMPLE);
  } else if (profileVersion == FileProfile::BASE) {
    iaSeqHdr->set_primary_profile(iamf_tools_cli_proto::PROFILE_VERSION_BASE);
    iaSeqHdr->set_additional_profile(
        iamf_tools_cli_proto::PROFILE_VERSION_BASE);
  } else if (profileVersion == FileProfile::BASE_ENHANCED) {
    iaSeqHdr->set_primary_profile(
        iamf_tools_cli_proto::PROFILE_VERSION_BASE_ENHANCED);
    iaSeqHdr->set_additional_profile(
        iamf_tools_cli_proto::PROFILE_VERSION_BASE_ENHANCED);
  } else {
    iaSeqHdr->set_primary_profile(
        iamf_tools_cli_proto::PROFILE_VERSION_INVALID);
    iaSeqHdr->set_additional_profile(
        iamf_tools_cli_proto::PROFILE_VERSION_INVALID);
  }
}

void writeLPCMConfigMD(const int samplesPerBlock, const int sampleRate,
                       const int sampleSize,
                       iamf_tools_cli_proto::UserMetadata& user_metadata) {
  auto codecMD = user_metadata.codec_config_metadata_size() == 0
                     ? user_metadata.add_codec_config_metadata()
                     : user_metadata.mutable_codec_config_metadata(0);
  codecMD->set_codec_config_id(200);

  auto codecConfigMD = codecMD->mutable_codec_config();
  codecConfigMD->set_num_samples_per_frame(samplesPerBlock);
  codecConfigMD->set_audio_roll_distance(0);
  codecConfigMD->set_codec_id(::iamf_tools_cli_proto::CodecId::CODEC_ID_LPCM);

  auto lpcmConfigMD = codecConfigMD->mutable_decoder_config_lpcm();
  lpcmConfigMD->set_sample_format_flags(
      iamf_tools_cli_proto::LPCM_LITTLE_ENDIAN);
  lpcmConfigMD->set_sample_size(sampleSize);
  lpcmConfigMD->set_sample_rate(sampleRate);
}

void writeFLACConfigMD(const int samplesPerBlock, const int samplesProcessed,
                       const int bitsPerSample, const int compressionLevel,
                       const int sampleRate,
                       iamf_tools_cli_proto::UserMetadata& user_metadata) {
  auto codec = user_metadata.add_codec_config_metadata();
  codec->set_codec_config_id(200);

  auto codecConfig = codec->mutable_codec_config();
  codecConfig->set_codec_id(::iamf_tools_cli_proto::CodecId::CODEC_ID_FLAC);
  codecConfig->set_num_samples_per_frame(samplesPerBlock);
  codecConfig->set_audio_roll_distance(0);

  auto flacConfig = codecConfig->mutable_decoder_config_flac();
  auto flacMD = flacConfig->add_metadata_blocks();
  flacMD->mutable_header()->set_last_metadata_block_flag(true);
  flacMD->mutable_header()->set_block_type(
      iamf_tools_cli_proto::FLAC_BLOCK_TYPE_STREAMINFO);
  flacMD->mutable_header()->set_metadata_data_block_length(34);
  flacMD->mutable_stream_info()->set_minimum_block_size(samplesPerBlock);
  flacMD->mutable_stream_info()->set_maximum_block_size(samplesPerBlock);
  flacMD->mutable_stream_info()->set_sample_rate(sampleRate);

  // Note that bits per sample seems to be 0 based, ie, we'd expect 16 to work
  // here but it doesn't and 15 does, we'd expect 24 to work but it doesn't and
  // 23 does, etc
  flacMD->mutable_stream_info()->set_bits_per_sample(bitsPerSample - 1);
  flacMD->mutable_stream_info()->set_total_samples_in_stream(samplesProcessed);
  flacConfig->mutable_flac_encoder_metadata()->set_compression_level(
      compressionLevel);
}

void writeOPUSConfigMD(const int sampleRate, const int bitratePerChannel,
                       iamf_tools_cli_proto::UserMetadata& user_metadata) {
  auto codec = user_metadata.add_codec_config_metadata();
  codec->set_codec_config_id(200);

  auto codecConfig = codec->mutable_codec_config();
  codecConfig->set_codec_id(::iamf_tools_cli_proto::CodecId::CODEC_ID_OPUS);

  // Set samples per frame based on sample rate
  // Valid frame sizes for Opus at different sample rates:
  int samplesPerFrame;
  int preSkip;
  int validatedBitrate;

  switch (sampleRate) {
    case 16000:
      samplesPerFrame = 320;  // 20ms frame at 16kHz
      preSkip = 104;          // Scaled pre-skip for 16kHz
      // Clamp bitrate for 16kHz: 8-64 kbps per channel
      validatedBitrate = std::max(8000, std::min(64000, bitratePerChannel));
      break;
    case 24000:
      samplesPerFrame = 480;  // 20ms frame at 24kHz
      preSkip = 156;          // Scaled pre-skip for 24kHz
      // Clamp bitrate for 24kHz: 16-96 kbps per channel
      validatedBitrate = std::max(16000, std::min(96000, bitratePerChannel));
      break;
    case 48000:
      samplesPerFrame = 960;  // 20ms frame at 48kHz
      preSkip = 312;          // Standard pre-skip for 48kHz
      // Clamp bitrate for 48kHz: 32-256 kbps per channel
      validatedBitrate = std::max(32000, std::min(256000, bitratePerChannel));
      break;
    default:
      // Default to 48kHz values for unsupported sample rates
      samplesPerFrame = 960;
      preSkip = 312;
      validatedBitrate = std::max(32000, std::min(256000, bitratePerChannel));
      break;
  }

  codecConfig->set_num_samples_per_frame(samplesPerFrame);
  codecConfig->set_automatically_override_audio_roll_distance(true);
  codecConfig->set_automatically_override_codec_delay(true);

  auto opusConfig = codecConfig->mutable_decoder_config_opus();
  opusConfig->set_input_sample_rate(sampleRate);
  opusConfig->set_pre_skip(preSkip);
  opusConfig->set_version(1);

  // Set the opus encoder metadata.
  // Data must be allocated (since set_allocated is used here). The
  // allocated data is owned by the protobuf and is deleted when the protobuf
  // is deleted.
  auto opusMD = new iamf_tools_cli_proto::OpusEncoderMetadata();
  opusMD->set_target_bitrate_per_channel(validatedBitrate);
  opusMD->set_application(
      ::iamf_tools_cli_proto::OpusApplicationFlag::APPLICATION_AUDIO);
  opusMD->set_use_float_api(false);
  opusConfig->set_allocated_opus_encoder_metadata(opusMD);
}

static bool validateMuxingPaths(const juce::String& inputAudioFile,
                                const juce::String& inputVideoFile,
                                const juce::String& outputMuxdFile) {
  if (!FileExport::validateFilePath(
          std::filesystem::path(inputAudioFile.toStdString()), true)) {
    LOG_ERROR(0, std::string("IAMFMuxing: Invalid input audio file path ") +
                     inputAudioFile.toStdString());
    return false;
  }
  if (!FileExport::validateFilePath(
          std::filesystem::path(inputVideoFile.toStdString()), true)) {
    LOG_ERROR(0, std::string("IAMFMuxing: Invalid input video file path ") +
                     inputVideoFile.toStdString());
    return false;
  }
  if (!FileExport::validateFilePath(
          std::filesystem::path(outputMuxdFile.toStdString()), false)) {
    LOG_ERROR(0, std::string("IAMFMuxing: Invalid output muxed file path ") +
                     outputMuxdFile.toStdString());
    return false;
  }
  return true;
}

// Writes IAMF audio to an MP4 file using a GPAC filter session
static bool muxIAMFAudio(const juce::String& inputAudioFile,
                         const juce::String& outputMp4File) {
#ifdef DEBUG
  gf_log_set_tool_level(GF_LOG_FILTER, GF_LOG_INFO);
  gf_log_set_tool_level(GF_LOG_CONTAINER, GF_LOG_INFO);
#endif

  GF_Err gf_err = GF_OK;
  GF_FilterSession* session = gf_fs_new_defaults(GF_FilterSessionFlags(0));
  if (session == NULL) {
    LOG_INFO(0, "IAMF Audio Muxing: Failed to create gpac session.");
    gf_fs_del(session);
    return false;
  }

  // Filter for input audio
  GF_Filter* src_audio = gf_fs_load_source(session, inputAudioFile.toRawUTF8(),
                                           NULL, NULL, &gf_err);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "IAMF Audio Muxing: Failed to load audio file " +
                     inputAudioFile.toStdString() +
                     " with error: " + std::string(gf_error_to_string(gf_err)));
    gf_fs_del(session);
    return false;
  }

  // Reframer for audio stream
  GF_Filter* reframer_filter = gf_fs_load_filter(session, "rfav1", &gf_err);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "IAMF Audio Muxing: Failed to load reframer filter.");
    gf_fs_del(session);
    return false;
  }

  // Filter for output mp4
  GF_Filter* dest_filter = gf_fs_load_destination(
      session, outputMp4File.toRawUTF8(), NULL, NULL, &gf_err);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "IAMF Audio Muxing: Failed to load output file filter.");
    gf_fs_del(session);
    return false;
  }

  gf_filter_set_source(reframer_filter, src_audio, NULL);
  gf_filter_set_source(dest_filter, reframer_filter, NULL);

  gf_err = gf_fs_run(session);

  if (gf_err >= GF_OK) {
    gf_err = gf_fs_get_last_connect_error(session);
    if (gf_err >= GF_OK) {
      gf_err = gf_fs_get_last_process_error(session);
    }
  }

  gf_fs_del(session);

  if (gf_err != GF_OK) {
    LOG_ERROR(0, "IAMF Audio Muxing: Failed with error: " +
                     std::string(gf_error_to_string(gf_err)));
    return false;
  }
  return true;
}

// Writes video to an existing MP4 file using GPAC ISO media APIs
static bool muxVideo(const juce::String& inputVideoFile,
                     const juce::String& outputMuxedFile) {
#ifdef DEBUG
  gf_log_set_tool_level(GF_LOG_CORE, GF_LOG_INFO);
  gf_log_set_tool_level(GF_LOG_CONTAINER, GF_LOG_INFO);
#endif

  GF_Err gf_err = GF_OK;

  // Open input video file for reading
  GF_ISOFile* src_video =
      gf_isom_open(inputVideoFile.toRawUTF8(), GF_ISOM_OPEN_READ, NULL);
  if (!src_video) {
    LOG_ERROR(0, "Video Muxing: Failed to open input video file " +
                     inputVideoFile.toStdString() + " for reading.");
    return false;
  }

  // Get the first video track from the source file
  u32 src_track_count = gf_isom_get_track_count(src_video);
  u32 src_video_track = 0;
  for (u32 i = 1; i <= src_track_count; i++) {
    u32 media_type = gf_isom_get_media_type(src_video, i);
    if (media_type == GF_ISOM_MEDIA_VISUAL) {
      src_video_track = i;
      break;
    }
  }

  if (src_video_track == 0) {
    LOG_ERROR(0, "Video Muxing: No video track found in input file.");
    gf_isom_close(src_video);
    return false;
  }

  // Open the existing MP4 file (with audio) for editing
  GF_ISOFile* dst_file =
      gf_isom_open(outputMuxedFile.toRawUTF8(), GF_ISOM_OPEN_EDIT, NULL);
  if (!dst_file) {
    LOG_ERROR(0, "Video Muxing: Failed to open output file " +
                     outputMuxedFile.toStdString() + " for editing.");
    gf_isom_close(src_video);
    return false;
  }

  // Get track count before adding video for verification later
  u32 tracks_before = gf_isom_get_track_count(dst_file);

  // Clone the video track structure (this only clones metadata, not samples)
  u32 dst_track = 0;
  gf_err = gf_isom_clone_track(src_video, src_video_track, dst_file,
                               GF_ISOTrackCloneFlags(0), &dst_track);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "Video Muxing: Failed to clone video track: " +
                     std::string(gf_error_to_string(gf_err)));
    gf_isom_close(dst_file);
    gf_isom_close(src_video);
    return false;
  }

  // Now manually copy all samples from the source track to the destination
  // track
  u32 sample_count = gf_isom_get_sample_count(src_video, src_video_track);
  for (u32 i = 1; i <= sample_count; i++) {
    u32 sample_desc_index = 0;
    GF_ISOSample* sample =
        gf_isom_get_sample(src_video, src_video_track, i, &sample_desc_index);
    if (!sample) {
      LOG_ERROR(0, "Video Muxing: Failed to get sample " + std::to_string(i) +
                       " from source track");
      gf_isom_close(dst_file);
      gf_isom_close(src_video);
      return false;
    }

    // Add the sample to the destination track
    gf_err = gf_isom_add_sample(dst_file, dst_track, sample_desc_index, sample);
    gf_isom_sample_del(&sample);

    if (gf_err != GF_OK) {
      LOG_ERROR(0, "Video Muxing: Failed to add sample " + std::to_string(i) +
                       " to destination track: " +
                       std::string(gf_error_to_string(gf_err)));
      gf_isom_close(dst_file);
      gf_isom_close(src_video);
      return false;
    }
  }

  // Close source file
  gf_isom_close(src_video);

  // Set storage mode to interleaved to mimic CLI behaviour
  gf_err = gf_isom_set_storage_mode(dst_file, GF_ISOM_STORE_INTERLEAVED);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "Video Muxing: Failed to set storage mode: " +
                     std::string(gf_error_to_string(gf_err)));
    gf_isom_close(dst_file);
    return false;
  }

  // Set final output filename (required for edit mode)
  // Create a temporary filename for the edited output
  juce::String tempOutputFile = outputMuxedFile + ".muxed";
  gf_err = gf_isom_set_final_name(
      dst_file, const_cast<char*>(tempOutputFile.toRawUTF8()));
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "Video Muxing: Failed to set final name: " +
                     std::string(gf_error_to_string(gf_err)));
    gf_isom_close(dst_file);
    return false;
  }

  // Close destination file (this writes the changes to the new filename)
  gf_err = gf_isom_close(dst_file);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "Video Muxing: Failed to close output file: " +
                     std::string(gf_error_to_string(gf_err)));
    return false;
  }

  // Replace the original file with the new file
  std::remove(outputMuxedFile.toRawUTF8());
  if (std::rename(tempOutputFile.toRawUTF8(), outputMuxedFile.toRawUTF8()) !=
      0) {
    LOG_ERROR(0, "Video Muxing: Failed to rename output file, error: " +
                     std::string(strerror(errno)));
    return false;
  }

  // Verify by reopening and checking track count
  GF_ISOFile* verify_file =
      gf_isom_open(outputMuxedFile.toRawUTF8(), GF_ISOM_OPEN_READ, NULL);
  if (!verify_file) {
    LOG_ERROR(0, "Video Muxing: Failed to reopen file for verification.");
    return false;
  }

  u32 tracks_after = gf_isom_get_track_count(verify_file);
  gf_isom_close(verify_file);

  if (tracks_after != tracks_before + 1) {
    LOG_ERROR(0, "Video Muxing: Track count verification failed. Expected " +
                     std::to_string(tracks_before + 1) + " tracks, got " +
                     std::to_string(tracks_after));
    return false;
  }

  return true;
}

bool muxIAMF(const FileExport& exportData) {
  const juce::String inputAudioFile = exportData.getExportFile();
  const juce::String inputVideoFile = exportData.getVideoSource();
  const juce::String outputMuxdFile = exportData.getVideoExportFolder();

  if (!validateMuxingPaths(inputAudioFile, inputVideoFile, outputMuxdFile)) {
    LOG_ERROR(0, "IAMF Muxing: One or more file paths are invalid.");
    return false;
  }

  // Initialize GPAC library before creating session
  GF_Err init_err = gf_sys_init(GF_MemTrackerNone, NULL);
  if (init_err != GF_OK) {
    LOG_ERROR(0, "IAMF Muxing: Failed to initialize GPAC system.");
    return false;
  }

  if (!muxIAMFAudio(inputAudioFile, outputMuxdFile)) {
    LOG_ERROR(0, "IAMF Muxing: Failed to mux IAMF audio into MP4.");
    gf_sys_close();
    return false;
  }
  if (!muxVideo(inputVideoFile, outputMuxdFile)) {
    LOG_ERROR(0, "IAMF Muxing: Failed to mux video into MP4.");
    gf_sys_close();
    return false;
  }

  gf_sys_close();
  return true;
}
}  // namespace IAMFExportHelper