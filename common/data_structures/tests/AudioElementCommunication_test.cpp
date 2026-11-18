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

#include "../src/AudioElementCommunication.h"

#include <gtest/gtest.h>

#include <cstring>
#include <vector>

// A message of the struct's exact size decodes to the published values.
TEST(AudioElementUpdateData, decodes_message_of_exact_size) {
  AudioElementUpdateData sent;
  sent.x = 1.f;
  sent.loudness = -23.f;
  strcpy(sent.name, "Dialog");

  AudioElementUpdateData received;
  ASSERT_TRUE(
      AudioElementUpdateData::fromMessage(&sent, sizeof(sent), received));
  EXPECT_EQ(received.x, 1.f);
  EXPECT_EQ(received.loudness, -23.f);
  EXPECT_EQ(received.uuid, sent.uuid);
  EXPECT_STREQ(received.name, "Dialog");
}

// Messages shorter or longer than the struct are rejected.
TEST(AudioElementUpdateData, rejects_message_of_wrong_size) {
  const std::vector<char> bytes(sizeof(AudioElementUpdateData) + 1, 'x');
  AudioElementUpdateData received;
  EXPECT_FALSE(AudioElementUpdateData::fromMessage(bytes.data(), 1, received));
  EXPECT_FALSE(AudioElementUpdateData::fromMessage(
      bytes.data(), sizeof(AudioElementUpdateData) - 1, received));
  EXPECT_FALSE(AudioElementUpdateData::fromMessage(bytes.data(), bytes.size(),
                                                   received));
}

// A name that fills its field without a terminator is terminated on receipt.
TEST(AudioElementUpdateData, terminates_unterminated_name) {
  AudioElementUpdateData sent;
  memset(sent.name, 'a', sizeof(sent.name));

  AudioElementUpdateData received;
  ASSERT_TRUE(
      AudioElementUpdateData::fromMessage(&sent, sizeof(sent), received));
  EXPECT_EQ(strlen(received.name), sizeof(received.name) - 1);
}
