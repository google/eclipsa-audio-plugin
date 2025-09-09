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

#include "data_repository/implementation/AudioElementRepository.h"
#include "data_repository/implementation/MixPresentationRepository.h"
#include "data_structures/src/FileExport.h"
#include "user_metadata.pb.h"

namespace IAMFExportHelper {
void writeIASeqHdr(FileProfile profileVersion,
                   iamf_tools_cli_proto::UserMetadata& userMetadata);
void writeLPCMConfigMD(const int samplesPerBlock, const int sampleRate,
                       const int sampleSize,
                       iamf_tools_cli_proto::UserMetadata& user_metadata);
void writeFLACConfigMD(const int samplesPerBlock, const int samplesProcessed,
                       const int bitsPerSample, const int compressionLevel,
                       const int sampleRate,
                       iamf_tools_cli_proto::UserMetadata& user_metadata);
void writeOPUSConfigMD(const int sampleRate, const int bitratePerChannel,
                       iamf_tools_cli_proto::UserMetadata& user_metadata);
bool muxIAMF(const AudioElementRepository& aeRepo,
             const MixPresentationRepository& mpRepo,
             const FileExport& exportData);
}  // namespace IAMFExportHelper