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

#pragma once
#include <juce_core/juce_core.h>

#include <atomic>
#include <functional>

#include "FilePlaybackProcessorTasks.h"

class FilePlaybackProcessor;

class FilePlaybackProcessorWorker {
 public:
  explicit FilePlaybackProcessorWorker(FilePlaybackProcessor& processor);

  void submitTask(const FilePlaybackProcessorEvents::TaskType newType,
                  FilePlaybackProcessorEvents::TaskData data = {});

  void postTask(const FilePlaybackProcessorEvents::TaskResult wayFinished,
                const uint64_t id);

 private:
  class LambdaJob : public juce::ThreadPoolJob {
   public:
    explicit LambdaJob(std::function<void()> func);
    juce::ThreadPoolJob::JobStatus runJob() override;

   private:
    std::function<void()> func_;
  };

  // Construct threadpool job
  juce::ThreadPoolJob* createJob(
      const FilePlaybackProcessorEvents::TaskType type,
      FilePlaybackProcessorEvents::TaskData data);

  juce::ThreadPool worker_{1};
  std::atomic_int currTaskId_{0};
  std::atomic<FilePlaybackProcessorEvents::TaskType> currentTask_{
      FilePlaybackProcessorEvents::TaskType::kNone};
  FilePlaybackProcessor& proc_;
};
