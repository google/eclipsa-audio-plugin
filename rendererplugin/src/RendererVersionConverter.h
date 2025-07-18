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

#include <juce_data_structures/juce_data_structures.h>

class RendererVersionConverter {
 public:
  // Converts the version of the plugin to the latest version.
  // This function should be called during the plugin initialization.
  static void convertToLatestVersion(
      std::unique_ptr<juce::XmlElement>& xmlState);

 private:
  // Helper function to convert from an older version to a newer version.
  static void convertFrom_NoVersion_To_1p1p1(
      std::unique_ptr<juce::XmlElement>& xmlState);
};