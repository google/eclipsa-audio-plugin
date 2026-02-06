#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

class Worker {
 public:
  using CancelToken = std::shared_ptr<std::atomic<bool>>;
  using CancelFlag = std::atomic<bool>;

  Worker() { workerThread_ = std::thread(&Worker::runLoop, this); }

  ~Worker() {
    stop_ = true;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (cancelToken_) {
        *cancelToken_ = true;
      }
    }
    cv_.notify_one();
    if (workerThread_.joinable()) {
      workerThread_.join();
    }
  }

  template <typename Task, typename TaskCallback>
  void submit(Task task, TaskCallback callback) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (cancelToken_) {
        *cancelToken_ = true;
      }
      CancelToken newToken = std::make_shared<CancelFlag>(false);
      cancelToken_ = newToken;
      auto wrapper = [task, callback, newToken]() mutable {
        auto result = task(*newToken);
        callback(std::move(result));
      };
      nextTask_ = std::move(wrapper);
    }
    cv_.notify_one();
  }

 private:
  void runLoop() {
    while (!stop_) {
      std::function<void()> currTask;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return stop_ || nextTask_.has_value(); });
        if (stop_) {
          break;
        }

        currTask = std::move(*nextTask_);
        nextTask_ = std::nullopt;
      }
      currTask();
    }
  }

  // Data
  std::atomic<bool> stop_ = false;
  std::thread workerThread_;
  std::optional<std::function<void()>> nextTask_;
  // Synchronization
  std::mutex mutex_;
  std::condition_variable cv_;
  CancelToken cancelToken_;
};