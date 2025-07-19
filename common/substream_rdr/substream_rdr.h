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

      ID:               Substream Renderers
      vendor:           Google
      version:          0.0.1
      name:             Processors
      description:      Substream renderer for rendering Audio Element sources for final mixing
      license:          Apache License 2.0
      dependencies:     juce_audio_utils, components

END_JUCE_MODULE_DECLARATION

#endif

#include "bed2bed_rdr/BedToBedRdr.h"
#include "bin_rdr/BinauralRdr.h"
#include "hoa2bed_rdr/HOAToBedRdr.h"
#include "passthrough_rdr/PassthroughRdr.h"
#include "rdr_factory/RendererFactory.h"
#include "substream_rdr_utils/Speakers.h"
#include "surround_panner/AmbisonicPanner.h"
#include "surround_panner/AudioPanner.h"
#include "surround_panner/BinauralPanner.h"
#include "surround_panner/MonoToSpeakerPanner.h"
