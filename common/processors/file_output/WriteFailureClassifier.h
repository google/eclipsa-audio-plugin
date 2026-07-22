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
#include <juce_audio_processors/juce_audio_processors.h>

#include "data_structures/src/FileExport.h"

// Classifies a file write failure by probing whether the parent directory of
// `path` is actually writable. The real write attempt has already failed by
// the time this runs, so this only distinguishes a permission problem from
// some other write failure (disk full, invalid filename, etc.). Shared by
// every writer path (IAMF, per-audio-element WAV, direct WAV export) so
// classification stays consistent across all of them.
//
// The probe filename is unique per call (rather than a fixed name) so a
// symlink cannot be pre-planted at a predictable path in a shared/multi-user
// export destination, and so two concurrent exports to the same folder
// cannot collide on the same probe file.
ExportError classifyWriteFailure(const juce::String& path);
