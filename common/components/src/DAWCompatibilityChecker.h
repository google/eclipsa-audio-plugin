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
#include <juce_core/juce_core.h>

class DAWCompatibilityChecker {
 public:
  static bool isDAWSupported() {
    juce::PluginHostType hostType_;

    switch (hostType_.type) {
      // Supported DAWs
      case juce::PluginHostType::AvidProTools:
      case juce::PluginHostType::Reaper:
      case juce::PluginHostType::AdobePremierePro:
      case juce::PluginHostType::AppleLogic:
        return true;

      // All other DAWs are unsupported
      default:
        return false;
    }
  }
};
