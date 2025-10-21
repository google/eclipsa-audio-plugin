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

      ID:               Data Structures
      vendor:           Google
      version:          0.0.1
      name:             Data Structures
      description:      Data structures used throughout the project
      license:          Apache License 2.0
      dependencies:     juce_audio_utils, juce_data_structures

END_JUCE_MODULE_DECLARATION

#endif

#include "src/ActiveMixPresentation.h"
#include "src/AmbisonicsData.h"
#include "src/AudioElement.h"
#include "src/AudioElementCommunication.h"
#include "src/AudioElementSpatialLayout.h"
#include "src/ChannelGains.h"
#include "src/FileExport.h"
#include "src/LanguageCodeMetaData.h"
#include "src/LoudnessExportData.h"
#include "src/MixPresentation.h"
#include "src/MixPresentationLoudness.h"
#include "src/MixPresentationSoloMute.h"
#include "src/ParameterMetaData.h"
#include "src/PlaybackMS.h"
#include "src/RepositoryItem.h"
#include "src/RoomSetup.h"
#include "src/SpeakerMonitorData.h"
#include "src/TimeFormatConverter.h"