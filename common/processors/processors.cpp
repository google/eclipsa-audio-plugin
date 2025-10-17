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

#include "processors.h"

#include "audioelementplugin_publisher/AudioElementPluginDataPublisher.cpp"
#include "audioelementplugin_publisher/MessagingThread.cpp"
#include "channel_monitor/ChannelMonitorProcessor.cpp"
#include "file_output/FileOutputProcessor.cpp"
#include "file_output/FileOutputProcessor_PremierePro.cpp"
#include "file_output/WavFileOutputProcessor.cpp"
#include "file_output/iamf_export_utils/IAMFExportUtil.cpp"
#include "file_output/iamf_export_utils/IAMFFileReader.cpp"
#include "file_output/iamf_export_utils/IAMFFileWriter.cpp"
#include "gain/GainEditor.cpp"
#include "gain/GainProcessor.cpp"
#include "gain/MSProcessor.cpp"
#include "loudness_export/LoudnessExportProcessor.cpp"
#include "loudness_export/LoudnessExportProcessor_PremierePro.cpp"
#include "loudness_export/MixPresentationLoudnessExportContainer.cpp"
#include "mix_monitoring/MixMonitorProcessor.cpp"
#include "mix_monitoring/TrackMonitorProcessor.cpp"
#include "mix_monitoring/loudness_standards/MeasureEBU128.cpp"
#include "panner/Panner3DProcessor.cpp"
#include "remapping/RemappingProcessor.cpp"
#include "render/RenderProcessor.cpp"
#include "routing/RoutingProcessor.cpp"
#include "soundfield/SoundFieldProcessor.cpp"
#include "soundfield/SoundFieldReconstructor.cpp"