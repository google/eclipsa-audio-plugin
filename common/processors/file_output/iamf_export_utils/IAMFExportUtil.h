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
#include <data_structures/src/AudioElement.h>

#include "data_structures/src/FileExport.h"
#include "data_structures/src/MixPresentation.h"
#include "user_metadata.pb.h"

namespace IAMFExportHelper {
void writeIASeqHdr(FileProfile profileVersion,
                   iamf_tools_cli_proto::UserMetadata& userMetadata);
void writeLPCMConfigMD(const int samplesPerBlock, const int sampleRate,
                       const int sampleSize,
                       iamf_tools_cli_proto::UserMetadata& user_metadata);
void writeFLACConfigMD(const int samplesPerBlock,
                       const juce::int64 samplesProcessed,
                       const int bitsPerSample, const int compressionLevel,
                       const int sampleRate,
                       iamf_tools_cli_proto::UserMetadata& user_metadata);
void writeOPUSConfigMD(const int samplesPerBlock, const int sampleRate,
                       const int bitratePerChannel,
                       iamf_tools_cli_proto::UserMetadata& user_metadata);
std::vector<const AudioElement*> filterFreeAudioElements(
    const juce::OwnedArray<AudioElement>& audioElements,
    const juce::OwnedArray<MixPresentation>& mixPresentations);
// Muxes the exported IAMF audio and configured video source together.
// When non-null, `outVideoDurationSec` is set to the video track's own
// duration (in seconds) as already computed during muxing -- callers that
// need the video's duration should read it from here instead of a second,
// independent parse of the video file (see getMediaDurationSeconds below).
// Left untouched if muxing fails before the video file is opened.
bool muxIAMF(const FileExport& exportData,
             double* outVideoDurationSec = nullptr);

// Returns the media file's duration in seconds, or -1.0 if the file cannot
// be opened or has no timescale. Prefers the first video track's own
// duration over the movie/container-level duration, falling back to the
// container duration if the file has no visual track.
[[nodiscard]] double getMediaDurationSeconds(const juce::String& mediaFilePath);
}  // namespace IAMFExportHelper
