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
#include <juce_audio_processors/juce_audio_processors.h>

class TimeFormatConverter {
 public:
  enum TimeFormat { HoursMinutesSeconds = 0, BarsBeats = 1, Timecode = 2 };

  static juce::String secondsToHMS(int timeInSeconds);
  static juce::String secondsToBarsBeats(
      int timeInSeconds, double bpm,
      const juce::AudioPlayHead::TimeSignature& timeSig);
  static juce::String secondsToTimecode(
      int timeInSeconds, const juce::AudioPlayHead::FrameRate& frameRate);

  static int hmsToSeconds(const juce::String& val);
  static int barsBeatsToSeconds(
      const juce::String& val, double bpm,
      const juce::AudioPlayHead::TimeSignature& timeSig);
  static int timecodeToSeconds(const juce::String& val);

  static juce::String getFormatDescription(TimeFormat format);
};
