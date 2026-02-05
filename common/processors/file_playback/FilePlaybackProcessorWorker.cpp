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

#include "FilePlaybackProcessorWorker.h"

#include "FilePlaybackProcessor.h"
#include "logger/logger.h"

FilePlaybackProcessorWorker::FilePlaybackProcessorWorker(
    FilePlaybackProcessor& processor)
    : proc_(processor) {}

void FilePlaybackProcessorWorker::submitTask(
    const FilePlaybackProcessorEvents::TaskType newType,
    FilePlaybackProcessorEvents::TaskData data) {
  LOG_INFO(0, "FilePlaybackProcessorWorker::submitTask: Submitting task: " +
                  std::string(
                      FilePlaybackProcessorEvents::taskTypeToString(newType)));

  if (FilePlaybackProcessorEvents::getEventPriority(newType) <
      FilePlaybackProcessorEvents::getEventPriority(currentTask_.load())) {
    LOG_INFO(0, "FilePlaybackProcessorWorker::submitTask: Task " +
                    std::string(FilePlaybackProcessorEvents::taskTypeToString(
                        newType)) +
                    " skipped due to lower priority");
    return;
  }

  ++currTaskId_;

  // Join any existing task
  proc_.updateProcessorState(FilePlaybackProcessor::State::kBuffering);
  proc_.abortTask(true);
  const bool kShutdown = worker_.removeAllJobs(true, 2000);
  proc_.abortTask(false);

  if (!kShutdown) {
    LOG_ERROR(0,
              "FilePlaybackProcessorWorker: Failed to shutdown existing task");
    return;
  }

  currentTask_ = newType;
  data.id = currTaskId_;
  juce::ThreadPoolJob* job = createJob(newType, data);
  worker_.addJob(job, true);
}

void FilePlaybackProcessorWorker::postTask(
    const FilePlaybackProcessorEvents::TaskResult wayFinished,
    const uint64_t id) {
  if (id != currTaskId_) {
    LOG_INFO(0,
             "FilePlaybackProcessorWorker: Discarding stale "
             "task completion notification");
    return;
  }

  currentTask_ = FilePlaybackProcessorEvents::TaskType::kNone;
  proc_.handleTaskCompletion(wayFinished);
}

FilePlaybackProcessorWorker::LambdaJob::LambdaJob(std::function<void()> func)
    : juce::ThreadPoolJob("FilePlaybackProcessorWorkerJob"),
      func_(std::move(func)) {}

juce::ThreadPoolJob::JobStatus
FilePlaybackProcessorWorker::LambdaJob::runJob() {
  func_();
  return juce::ThreadPoolJob::JobStatus::jobHasFinished;
}

// Create a job from the requested task type
juce::ThreadPoolJob* FilePlaybackProcessorWorker::createJob(
    const FilePlaybackProcessorEvents::TaskType type,
    FilePlaybackProcessorEvents::TaskData data) {
  return new LambdaJob([this, type, data]() {
    // Handle the task based on its type
    FilePlaybackProcessorEvents::TaskResult res;
    switch (type) {
      case FilePlaybackProcessorEvents::TaskType::kLoad:
        res = proc_.loadFileForPlayback(data.filePath);
        postTask(res, data.id);
        break;
      case FilePlaybackProcessorEvents::TaskType::kSeek: {
        res = proc_.seekTo(
            data.val, data.prevState == FilePlaybackProcessor::State::kPlaying);
        postTask(res, data.id);
      } break;
      case FilePlaybackProcessorEvents::TaskType::kLayout:
        res = proc_.changeLayout(
            data.layout,
            data.prevState == FilePlaybackProcessor::State::kPlaying);
        postTask(res, data.id);
        break;
      default:
        break;
    }
  });
}