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

#include "components.h"

#include "src/AEStripComponent.cpp"
#include "src/AudioFilePlayer.cpp"
#include "src/ChannelListComponent.cpp"
#include "src/ControlKnob.cpp"
#include "src/ControlKnobSkewed.cpp"
#include "src/RoomSetupScreen.cpp"
#include "src/SegmentedToggleButton.cpp"
#include "src/SegmentedToggleImageButton.cpp"
#include "src/SliderLabelAttachment.cpp"
#include "src/TextEditorControlledDial.cpp"
#include "src/ambisonics_visualizers/AmbisonicsVisualizer.cpp"
#include "src/ambisonics_visualizers/ColourLegend.cpp"
#include "src/loudness_meter/LoudnessLevelBar.cpp"
#include "src/loudness_meter/LoudnessMeter.cpp"
#include "src/loudness_meter/LoudnessScale.cpp"
#include "src/loudness_meter/LoudnessStats.cpp"
#include "src/room_views/Coordinates.cpp"
#include "src/room_views/PerspectiveRoomView.cpp"
#include "src/room_views/PerspectiveRoomViews.cpp"
#include "src/track_monitor_visuals/AmbisonicsVisualizerSet.cpp"
#include "src/track_monitor_visuals/LoudnessMetersSet.cpp"
#include "src/track_monitor_visuals/TrackMonitorViewPort.cpp"
