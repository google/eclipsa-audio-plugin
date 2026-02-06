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