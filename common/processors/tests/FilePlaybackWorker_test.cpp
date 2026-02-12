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

#include "../file_playback/FilePlaybackWorker.h"

#include <gtest/gtest.h>
#include <juce_events/juce_events.h>

class PlaybackTasksTest : public ::testing::Test {
 protected:
  Worker worker;
  juce::WaitableEvent event;
};

TEST_F(PlaybackTasksTest, basic_task) {
  int taskResult = 0;
  worker.submit([](Worker::CancelFlag& stop) { return 42; },
                [&](const int res) {
                  taskResult = res;
                  event.signal();
                });

  event.wait();
  EXPECT_EQ(taskResult, 42);
}

TEST_F(PlaybackTasksTest, preempt_task) {
  int taskResult = 0;

  // Submit a long-running task
  worker.submit(
      [](Worker::CancelFlag& stop) {
        while (!stop) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return -1;
      },
      [&](const int res) {
        taskResult = res;
        event.signal();
      });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Submit a second task that should preempt the first
  worker.submit([](Worker::CancelFlag& stop) { return 74; },
                [&](const int res) {
                  taskResult = res;
                  event.signal();
                });

  event.wait();
  EXPECT_EQ(taskResult, 74);
}