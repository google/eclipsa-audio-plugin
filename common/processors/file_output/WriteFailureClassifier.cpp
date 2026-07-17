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

#include "WriteFailureClassifier.h"

#include <logger/logger.h>

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <string>

ExportError classifyWriteFailure(const juce::String& path) {
  try {
    const std::filesystem::path kParentDir =
        std::filesystem::path(path.toStdString()).parent_path();
    if (kParentDir.empty()) {
      // No parent directory to probe -- don't fall back to probing the
      // process's current working directory.
      return kFileWriteFailed;
    }
    const std::filesystem::path kProbePath =
        kParentDir /
        (".acx_write_probe_" + juce::Uuid().toString().toStdString() + ".tmp");
    std::ofstream probe(kProbePath);
    if (!probe.is_open()) {
      const int kProbeErrno = errno;
      if (kProbeErrno == EACCES || kProbeErrno == EPERM ||
          kProbeErrno == EROFS) {
        return kPermissionDenied;
      }
      return kFileWriteFailed;
    }
    probe.close();
    std::error_code removeEc;
    std::filesystem::remove(kProbePath, removeEc);
    if (removeEc) {
      LOG_WARNING(0,
                  "classifyWriteFailure: Failed to remove write-probe file: " +
                      removeEc.message());
    }
    // The probe succeeded, so the parent directory is actually writable --
    // the real failure must be something else (e.g. disk full, invalid
    // filename).
    return kFileWriteFailed;
  } catch (const std::filesystem::filesystem_error&) {
    return kFileWriteFailed;
  }
}
