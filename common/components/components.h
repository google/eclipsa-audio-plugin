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

      ID:               GUI Components
      vendor:           Google
      version:          0.0.1
      name:             components
      description:      Components for plugin GUI
      license:          Apache License 2.0
      dependencies:     juce_audio_processors, juce_gui_basics, data_structures, data_repository

END_JUCE_MODULE_DECLARATION

#endif

#include "src/ChannelListComponent.h"
#include "src/DAWCompatibilityChecker.h"
#include "src/DAWWarningBanner.h"
#include "src/DialIndicator.h"
#include "src/EclipsaColours.h"
#include "src/HeaderBar.h"
#include "src/Icons.h"
#include "src/MainEditor.h"
#include "src/RoomSetupScreen.h"
#include "src/RoundedRectangle.h"
#include "src/SegmentedToggleButton.h"
#include "src/SegmentedToggleImageButton.h"
#include "src/SliderLabelAttachment.h"
#include "src/TimeFormatSegmentSelector.h"
#include "src/ambisonics_visualizers/VisualizerPair.h"
#include "src/loudness_meter/LoudnessMeter.h"
#include "src/loudness_meter/LoudnessScale.h"
#include "src/loudness_meter/LoudnessStats.h"
#include "src/room_views/Coordinates.h"
#include "src/room_views/PerspectiveRoomViews.h"