/*
 * Copyright 2025 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

#include "logger/logger.h"
#if JUCE_MAC
#include <libproc.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/**
 * Custom host detector that fixes JUCE 7.0.12 auval detection bug.
 * JUCE's PluginHostType().isAUVal() looks for "auvaltool" but the actual
 * executable is "auval", causing false negatives during AU validation.
 */
class CustomHostDetector {
 public:
  CustomHostDetector() { detectHost(); }

  // Main method for AU validation context detection
  bool isAUValidationContext() const {
    return isAUVal_ || hostType_.isAUVal() || hostType_.isLogic();
  }

  // Standard JUCE host detection methods (delegate to JUCE)
  bool isLogic() const { return hostType_.isLogic(); }
  bool isGarageBand() const { return hostType_.isGarageBand(); }
  bool isPremiere() const { return hostType_.isPremiere(); }
  bool isReaper() const { return hostType_.isReaper(); }

  // Our custom auval detection
  bool isAUVal() const { return isAUVal_; }

 private:
  void detectHost() {
    // Get the host executable path/name
    hostPath_ =
        juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getFullPathName();
    hostFilename_ =
        juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getFileName();

    // Debug logging to see what we're detecting
    LOG_INFO(0, "CustomHostDetector: hostPath = " + hostPath_.toStdString());
    LOG_INFO(
        0, "CustomHostDetector: hostFilename = " + hostFilename_.toStdString());

    // Detect AU validation tools (fix for JUCE bug)
    isAUVal_ = isAUValidationTool();

    LOG_INFO(0, "CustomHostDetector: isAUVal = " +
                    std::string(isAUVal_ ? "true" : "false"));
    LOG_INFO(0, "CustomHostDetector: JUCE isAUVal = " +
                    std::string(hostType_.isAUVal() ? "true" : "false"));
    LOG_INFO(0, "CustomHostDetector: isLogic = " +
                    std::string(hostType_.isLogic() ? "true" : "false"));
  }

  bool isAUValidationTool() const {
    // Direct executable name/path checks
    if (hostFilename_.containsIgnoreCase("auval") ||
        hostFilename_.containsIgnoreCase("auvaltool") ||
        hostPath_.containsIgnoreCase("auval") ||
        hostPath_.containsIgnoreCase("AudioUnitHosting"))
      return true;

#if JUCE_MAC
    // Walk up the parent process chain a few levels to see if launched by auval
    pid_t pid = getpid();
    for (int depth = 0; depth < 4; ++depth) {
      struct proc_bsdinfo procInfo;
      if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &procInfo, sizeof(procInfo)) !=
          sizeof(procInfo))
        break;
      juce::String parentName(procInfo.pbi_name);
      if (parentName.containsIgnoreCase("auval")) {
        return true;
      }
      pid = procInfo.pbi_ppid;
      if (pid <= 1) break;
    }
#endif
    return false;
  }

  juce::PluginHostType hostType_;
  juce::String hostPath_;
  juce::String hostFilename_;
  bool isAUVal_;
};