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

/**
 * Custom host detector that fixes JUCE 7.0.12 auval detection bug.
 * JUCE's PluginHostType().isAUVal() looks for "auvaltool" but the actual
 * executable is "auval", causing false negatives during AU validation.
 */
class CustomHostDetector {
public:
    CustomHostDetector() {
        detectHost();
    }
    
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
        hostPath_ = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName();
        hostFilename_ = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFileName();
        
        // Detect AU validation tools (fix for JUCE bug)
        isAUVal_ = isAUValidationTool();
    }
    
    bool isAUValidationTool() const {
        // Check for various AU validation tool names
        return hostFilename_.containsIgnoreCase("auval") ||
               hostFilename_.containsIgnoreCase("auvaltool") ||
               hostPath_.containsIgnoreCase("auval") ||
               hostPath_.containsIgnoreCase("AudioUnitHosting");
    }
    
    juce::PluginHostType hostType_;
    juce::String hostPath_;
    juce::String hostFilename_;
    bool isAUVal_;
};