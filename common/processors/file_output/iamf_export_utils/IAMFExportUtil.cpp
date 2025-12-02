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

// Checks for errors in the muxed file using ffmpeg and verifies presence of an
// IAMF audio stream and video stream
bool validateMuxedFile(const juce::String& path) {
#ifndef ECLIPSA_FFMPEG_AVAILABLE
  LOG_WARNING(0, "FFmpeg validation skipped: FFmpeg is not available");
  return true;
#endif

  // Check for errors in the file using ffmpeg
  juce::String ffmpegCommand =
      "ffmpeg -v error -i \"" + path + "\" -f null - 2>&1";

  FILE* pipe = popen(ffmpegCommand.toRawUTF8(), "r");
  if (!pipe) {
    std::cout << "Failed to execute ffmpeg command" << std::endl;
    return false;
  }

  char buffer[128];
  juce::String ffmpegOutput;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    ffmpegOutput += buffer;
  }

  int ffmpegExitCode = pclose(pipe);

  if (!ffmpegOutput.isEmpty() || ffmpegExitCode != 0) {
    std::cout << "FFmpeg validation errors:" << std::endl;
    // Only print the first 1000 characters to avoid excessive output
    if (ffmpegOutput.length() > 1000) {
      ffmpegOutput = ffmpegOutput.substring(0, 1000) + "... (truncated)";
    }
    std::cout << ffmpegOutput.toRawUTF8() << std::endl;
    return false;
  }

  // Check for IAMF Audio Element stream group using ffprobe
  juce::String ffprobeAudioElementCommand =
      "ffprobe \"" + path + "\" 2>&1 | grep -q 'IAMF Audio Element'";

  int audioElementCheckCode = system(ffprobeAudioElementCommand.toRawUTF8());
  if (audioElementCheckCode != 0) {
    std::cout << "FFmpeg validation: IAMF Audio Element stream group not found"
              << std::endl;
    return false;
  }

  // Check for IAMF Mix Presentation stream group using ffprobe
  juce::String ffprobeMixPresentationCommand =
      "ffprobe \"" + path + "\" 2>&1 | grep -q 'IAMF Mix Presentation'";

  int mixPresentationCheckCode =
      system(ffprobeMixPresentationCommand.toRawUTF8());
  if (mixPresentationCheckCode != 0) {
    std::cout << "FFmpeg validation: IAMF Mix Presentation stream group not "
                 "found"
              << std::endl;
    return false;
  }

  // Check for video stream using ffprobe
  juce::String ffprobeVideoCommand =
      "ffprobe -select_streams v -show_entries stream=codec_name -of "
      "default=noprint_wrappers=1 \"" +
      path + "\" 2>&1 | grep -q .";

  int videoCheckCode = system(ffprobeVideoCommand.toRawUTF8());
  if (videoCheckCode != 0) {
    std::cout << "FFmpeg validation: Video stream not found" << std::endl;
    return false;
  }

  return true;
}

bool muxIamfAudio(const juce::String& inputAudioFile,
                  const juce::String& outputMp4File) {
#ifdef DEBUG
  gf_log_set_tool_level(GF_LOG_FILTER, GF_LOG_INFO);
  gf_log_set_tool_level(GF_LOG_CONTAINER, GF_LOG_INFO);
#endif

  // Initialize GPAC library before creating session
  GF_Err init_err = gf_sys_init(GF_MemTrackerNone, NULL);
  if (init_err != GF_OK) {
    LOG_ERROR(0, "IAMF Muxing: Failed to initialize GPAC system.");
    return false;
  }

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
    LOG_ERROR(0, "IAMF Audio Muxing: Failed to load audio file.");
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

  // Connect the filters: audio source -> reframer -> destination
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
    LOG_ERROR(0, "IAMF Audio Muxing: Failed with error code " + gf_err);
    return false;
  }

  return true;
}

bool muxVideo(const juce::String& inputVideoFile,
              const juce::String& audioMp4File,
              const juce::String& outputMuxedFile) {
#ifdef DEBUG
  gf_log_set_tool_level(GF_LOG_FILTER, GF_LOG_INFO);
  gf_log_set_tool_level(GF_LOG_CONTAINER, GF_LOG_INFO);
#endif

  // Initialize GPAC library before creating session
  GF_Err init_err = gf_sys_init(GF_MemTrackerNone, NULL);
  if (init_err != GF_OK) {
    LOG_ERROR(0, "IAMF Muxing: Failed to initialize GPAC system.");
    return false;
  }

  GF_Err gf_err = GF_OK;
  GF_FilterSession* session = gf_fs_new_defaults(GF_FilterSessionFlags(0));
  if (session == NULL) {
    LOG_INFO(0, "Video Interleaving: Failed to create gpac session.");
    gf_fs_del(session);
    return false;
  }

  // Filter for input video
  GF_Filter* src_video = gf_fs_load_source(session, inputVideoFile.toRawUTF8(),
                                           NULL, NULL, &gf_err);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "Video Interleaving: Failed to load video file.");
    gf_fs_del(session);
    return false;
  }

  // Filter for input mp4 containing iamf audio track
  GF_Filter* src_audio_mp4 =
      gf_fs_load_source(session, audioMp4File.toRawUTF8(), NULL, NULL, &gf_err);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "Video Interleaving: Failed to load audio MP4 file.");
    gf_fs_del(session);
    return false;
  }

  // Filter for output mp4
  GF_Filter* dest_filter = gf_fs_load_destination(
      session, outputMuxedFile.toRawUTF8(), NULL, NULL, &gf_err);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "Video Interleaving: Failed to load output file filter.");
    gf_fs_del(session);
    return false;
  }

  // Filter for muxing audio and video
  GF_Filter* mux_filter = gf_fs_load_filter(session, "mp4mx", &gf_err);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "Video Interleaving: Failed to load muxing filter.");
    gf_fs_del(session);
    return false;
  }

  // Filter for removing audio from video
  GF_Filter* audio_remover = gf_fs_load_filter(session, "mp4dmx", &gf_err);
  if (gf_err != GF_OK) {
    LOG_ERROR(0, "Video Interleaving: Failed to load audio remover filter.");
    gf_fs_del(session);
    return false;
  }

  gf_filter_set_source(audio_remover, src_video, NULL);
  gf_filter_set_source(mux_filter, audio_remover, NULL);
  gf_filter_set_source(mux_filter, src_audio_mp4, NULL);
  gf_filter_set_source(dest_filter, mux_filter, NULL);

  gf_err = gf_fs_run(session);

  if (gf_err >= GF_OK) {
    gf_err = gf_fs_get_last_connect_error(session);
    if (gf_err >= GF_OK) {
      gf_err = gf_fs_get_last_process_error(session);
    }
  }

  gf_fs_del(session);

  if (gf_err != GF_OK) {
    LOG_ERROR(0, "Video Interleaving: Failed with error code " + gf_err);
    return false;
  }

  return true;
}

bool muxIAMF(const AudioElementRepository& aeRepo,
             const MixPresentationRepository& mpRepo,
             const FileExport& exportData) {
  const juce::String inputAudioFile = exportData.getExportFile();
  const juce::String inputVideoFile = exportData.getVideoSource();
  const juce::String outputMuxdFile = exportData.getVideoExportFolder();

  // Pre-muxing checks
  if (!std::filesystem::exists(inputAudioFile.toStdString())) {
    LOG_ERROR(0, "IAMF Muxing: Input audio file" +
                     inputAudioFile.toStdString() + " does not exist.");
    return false;
  }
  if (!std::filesystem::exists(inputVideoFile.toStdString())) {
    LOG_ERROR(0, "IAMF Muxing: Input video file" +
                     inputVideoFile.toStdString() + " does not exist.");
    std::cout << "IAMF Muxing: Input video file" +
                     inputVideoFile.toStdString() + " does not exist."
              << std::endl;
    return false;
  }
  if (outputMuxdFile.isEmpty()) {
    LOG_ERROR(0, "IAMF Muxing: Output muxed file path is empty.");
    return false;
  }

  // Step 1: Mux IAMF audio into an MP4 container
  if (!muxIamfAudio(inputAudioFile, outputMuxdFile)) {
    LOG_ERROR(0, "IAMF Muxing: Failed to mux IAMF audio.");
    return false;
  }

  // Step 2: Interleave video with the audio MP4
  if (!muxVideo(inputVideoFile, outputMuxdFile, outputMuxdFile)) {
    LOG_ERROR(0, "IAMF Muxing: Failed to interleave video with audio.");
    return false;
  }

  return true;
}
}  // namespace IAMFExportHelper