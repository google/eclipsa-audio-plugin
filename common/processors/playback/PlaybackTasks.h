#pragma once

#include <atomic>
#include <functional>
#include <thread>

class Task {
 public:
  // All tasks should return a status enum and a pointer?
  enum CompletionType {
    kPreempted,
    kCompleted,
    kCompletedWithErr,
  };

  struct Result {
    CompletionType ctype;
  };

  virtual Result run() = 0;
  void abort() { aborted_ = true; }
  bool wasAborted() const { return aborted_; }

 private:
  std::atomic_bool aborted_ = false;
};

class Worker {
 public:
  Worker(std::function<Task::Result()> tfc) {}
  ~Worker() {
    shouldExit_ = true;
    if (td_.joinable()) {
      td_.join();
    }
  }

  void submitTask() {}

  void run() {
    while (!shouldExit_) {
      Task* task = currTask_;

      if (task) {
        const auto kRes = task->run();

        if (!task->wasAborted()) {
        }

        delete task;
      }
    }
  }

  void postTask() {}

 private:
  std::atomic_bool shouldExit_;
  std::thread td_;
  std::atomic<Task*> currTask_;
};