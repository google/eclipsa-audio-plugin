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

#include "RendererVersionConverter.h"

void RendererVersionConverter::convertToLatestVersion(
    std::unique_ptr<juce::XmlElement>& xmlState) {
  // Check for version attribute, if missing, convert from no version to 1.1.1
  if (!xmlState->hasAttribute("version")) {
    convertFrom_NoVersion_To_1p1p1(xmlState);
  }

  // Add further version conversion logic for future versions here as necessary
  // Ensure version conversion logic is progressive (eg: check no version
  // --> 1.1.1, then 1.1.1 --> 1.2.0, etc.) This way, any version can be
  // converted to the latest version in a single pass.

  // Add a map for converting (eg: map 1.1.1 to 1.2.0, etc), associating the
  // various map entries with a source and destintation version and a function
  // to use for the conversion.
}

void RendererVersionConverter::convertFrom_NoVersion_To_1p1p1(
    std::unique_ptr<juce::XmlElement>& xmlState) {
  xmlState->setAttribute("version", "1.1.1");
}
