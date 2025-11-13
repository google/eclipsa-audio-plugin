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
#include <data_repository/data_repository.h>

#include "data_repository/implementation/MixPresentationLoudnessRepository.h"
#include "data_structures/src/MixPresentationSoloMute.h"

// Collection of references to persistent plugin state data.
struct RepositoryCollection {
  RoomSetupRepository& roomSetupRepo_;
  AudioElementRepository& aeRepo_;
  MultiChannelRepository& chGainRepo_;
  FileExportRepository& fioRepo_;
  MixPresentationRepository& mpRepo_;
  MixPresentationSoloMuteRepository& mpSMRepo_;
  MixPresentationLoudnessRepository& mpLoudnessRepo_;
  MultibaseAudioElementSpatialLayoutRepository& audioElementSpatialLayoutRepo_;
  MSPlaybackRepository& playbackMSRepo_;
  AudioElementSubscriber& audioElementSubscriber_;
  ActiveMixRepository& activeMPRepo_;
  FilePlaybackRepository& playbackRepo_;
};