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

#include "../processor_base/ProcessorBase.h"

#include <gtest/gtest.h>

const bool input = true;
const bool output = false;

class TestBaseProcessor : public ProcessorBase {
  void processBlock(juce::AudioBuffer<float>& buffer,
                    juce::MidiBuffer& midiMessages) override {
    juce::ignoreUnused(buffer);
    juce::ignoreUnused(midiMessages);
  }
};

TEST(test_base_processor, defaultbuses) {
  TestBaseProcessor proc;
  ASSERT_EQ(proc.getBusCount(input), 1);
  ASSERT_EQ(proc.getBusCount(output), 1);
}

TEST(test_base_processor, addbus) {
  TestBaseProcessor proc;
  ASSERT_TRUE(proc.canAddBus(input));
  ASSERT_TRUE(proc.canAddBus(output));
}

TEST(test_base_processor, removedefaultbus) {
  TestBaseProcessor proc;
  ASSERT_FALSE(proc.canRemoveBus(input));
  ASSERT_FALSE(proc.canRemoveBus(output));
}