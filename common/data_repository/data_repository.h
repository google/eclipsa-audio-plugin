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

#if 0

BEGIN_JUCE_MODULE_DECLARATION

      ID:               Data Repository
      vendor:           Google
      version:          0.0.1
      name:             Data Repository
      description:      Classes used to manage persistent state
      license:          Apache License 2.0
      dependencies:     juce_data_structures, data_structures

END_JUCE_MODULE_DECLARATION

#endif

#include "implementation/ActiveMixPresentationRepository.h"
#include "implementation/AudioElementRepository.h"
#include "implementation/AudioElementSpatialLayoutRepository.h"
#include "implementation/FileExportRepository.h"
#include "implementation/FilePlaybackRepository.h"
#include "implementation/MSPlaybackRepository.h"
#include "implementation/MixPresentationLoudnessRepository.h"
#include "implementation/MixPresentationRepository.h"
#include "implementation/MixPresentationSoloMuteRepository.h"
#include "implementation/MultiChannelGainRepository.h"
#include "implementation/RoomSetupRepository.h"