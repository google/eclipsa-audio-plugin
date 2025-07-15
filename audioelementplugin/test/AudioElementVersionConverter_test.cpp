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

#include "../src/AudioElementVersionConverter.h"

#include <gtest/gtest.h>
#include <juce_core/juce_core.h>

TEST(AudioElementVersionConverterTest, AddsVersionAttributeWhenMissing) {
  // Create an XmlElement without a version attribute
  auto xml = std::make_unique<juce::XmlElement>("State");
  ASSERT_FALSE(xml->hasAttribute("version"));

  AudioElementVersionConverter::convertToLatestVersion(xml);
  EXPECT_EQ(xml->getStringAttribute("version"), "1.1.1");
}

TEST(AudioElementVersionConverterTest, DoesNotOverwriteExistingVersion) {
  auto xml = std::make_unique<juce::XmlElement>("State");
  xml->setAttribute("version", "1.1.1");

  AudioElementVersionConverter::convertToLatestVersion(xml);
  EXPECT_EQ(xml->getStringAttribute("version"), "1.1.1");
}