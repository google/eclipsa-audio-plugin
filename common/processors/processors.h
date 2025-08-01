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

      ID:               Audio Processors
      vendor:           Google
      version:          0.0.1
      name:             Processors
      description:      Audio processors for building plugins
      license:          Apache License 2.0
      dependencies:     juce_audio_utils, juce_audio_processors, juce_dsp, juce_gui_basics, components

END_JUCE_MODULE_DECLARATION

#endif

#include "audioelementplugin_publisher/AudioElementPluginDataPublisher.h"
#include "audioelementplugin_publisher/MessagingThread.h"
#include "channel_monitor/ChannelMonitorProcessor.h"
#include "file_output/FileOutputProcessor.h"
#include "file_output/FileOutputProcessor_PremierePro.h"
#include "file_output/WavFileOutputProcessor.h"
#include "gain/GainEditor.h"
#include "gain/GainProcessor.h"
#include "gain/MSProcessor.h"
#include "loudness_export/LoudnessExportProcessor.h"
#include "loudness_export/LoudnessExportProcessor_PremierePro.h"
#include "loudness_export/MixPresentationLoudnessExportContainer.h"
#include "mix_monitoring/MixMonitorProcessor.h"
#include "mix_monitoring/TrackMonitorProcessor.h"
#include "panner/Panner3DProcessor.h"
#include "processor_base/ProcessorBase.h"
#include "remapping/RemappingProcessor.h"
#include "render/RenderProcessor.h"
#include "routing/RoutingProcessor.h"
#include "soundfield/SoundFieldProcessor.h"