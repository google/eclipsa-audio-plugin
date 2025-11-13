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

// This module encapsulate the IAMF audio player. The `IAMFPlaybackDevice`
// manages the physical playback device, as well as the layers that implement
// everything from background decoding to resampling to populating the playback
// device buffers.
// The playback device responds to state changes via listening to the
// `FilePlaybackRepository`. Currently, all updates are performed on the
// Message/UI thread. This means that interacting with the decoder will
// generate some latency. This is expected due to the nature of the decoder's
// serial design.

#if 0
BEGIN_JUCE_MODULE_DECLARATION

      ID:               Audio Processors
      vendor:           Google
      version:          0.0.1
      name:             player
      description:      Audio processors for building plugins
      license:          Apache License 2.0
      dependencies:     juce_audio_utils, juce_audio_processors, juce_dsp, juce_gui_basics, components

END_JUCE_MODULE_DECLARATION

#endif

#include "src/transport/IAMFDecoderSource.h"
#include "src/transport/IAMFPlaybackDevice.h"
