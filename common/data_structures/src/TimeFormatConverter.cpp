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

#include "TimeFormatConverter.h"

#include <cmath>

juce::String TimeFormatConverter::secondsToHMS(int timeInSeconds) {
  int seconds = timeInSeconds;
  int minutes = seconds / 60;
  seconds = seconds % 60;
  int hours = minutes / 60;
  minutes = minutes % 60;

  // Pad with zeros
  juce::String hourString =
      (hours < 10) ? "0" + juce::String(hours) : juce::String(hours);
  juce::String minuteString =
      (minutes < 10) ? "0" + juce::String(minutes) : juce::String(minutes);
  juce::String secondString =
      (seconds < 10) ? "0" + juce::String(seconds) : juce::String(seconds);

  return hourString + ":" + minuteString + ":" + secondString;
}

juce::String TimeFormatConverter::secondsToBarsBeats(
    int timeInSeconds, double bpm,
    const juce::AudioPlayHead::TimeSignature& timeSig) {
  if (bpm <= 0.0) {
    return "1.1.000";  // Default fallback
  }

  // Calculate bars and beats from time in seconds
  double beatsPerBar = timeSig.numerator;
  double secondsPerBeat = 60.0 / bpm;
  double totalBeats = timeInSeconds / secondsPerBeat;

  int bars = static_cast<int>(totalBeats / beatsPerBar) + 1;  // Bars start at 1
  double beatsInCurrentBar = std::fmod(totalBeats, beatsPerBar);
  int beat = static_cast<int>(beatsInCurrentBar) + 1;  // Beats start at 1
  int ticks = static_cast<int>(
      (beatsInCurrentBar - std::floor(beatsInCurrentBar)) * 960.0);

  // Format: BBB.B.TTT
  return juce::String(bars) + "." + juce::String(beat) + "." +
         juce::String(ticks).paddedLeft('0', 3);
}

juce::String TimeFormatConverter::secondsToTimecode(
    int timeInSeconds, const juce::AudioPlayHead::FrameRate& frameRate) {
  int hours = timeInSeconds / 3600;
  int minutes = (timeInSeconds % 3600) / 60;
  int seconds = timeInSeconds % 60;
  int frames = 0;  // For integer seconds, frames = 0

  // Format: HH:MM:SS:FF
  juce::String hourStr = juce::String(hours).paddedLeft('0', 2);
  juce::String minStr = juce::String(minutes).paddedLeft('0', 2);
  juce::String secStr = juce::String(seconds).paddedLeft('0', 2);
  juce::String frameStr = juce::String(frames).paddedLeft('0', 2);

  return hourStr + ":" + minStr + ":" + secStr + ":" + frameStr;
}

int TimeFormatConverter::hmsToSeconds(const juce::String& val) {
  auto parts = juce::StringArray::fromTokens(val, ":", "");
  if (parts.size() != 3) {
    return -1;
  }
  if (!parts[0].containsOnly("0123456789") ||
      !parts[1].containsOnly("0123456789") ||
      !parts[2].containsOnly("0123456789")) {
    return -1;
  }

  int hours = parts[0].getIntValue();
  int minutes = parts[1].getIntValue();
  int seconds = parts[2].getIntValue();

  // Validate ranges
  if (minutes > 59 || seconds > 59 || hours < 0 || minutes < 0 || seconds < 0) {
    return -1;
  }

  return (hours * 3600 + minutes * 60 + seconds);
}

int TimeFormatConverter::barsBeatsToSeconds(
    const juce::String& val, double bpm,
    const juce::AudioPlayHead::TimeSignature& timeSig) {
  auto parts = juce::StringArray::fromTokens(val, ".", "");
  if (parts.size() != 3) {
    return -1;
  }
  if (!parts[0].containsOnly("0123456789") ||
      !parts[1].containsOnly("0123456789") ||
      !parts[2].containsOnly("0123456789")) {
    return -1;
  }

  int bars = parts[0].getIntValue();
  int beat = parts[1].getIntValue();
  int ticks = parts[2].getIntValue();

  double beatsPerBar = timeSig.numerator;
  double secondsPerBeat = 60.0 / bpm;

  // Validate ranges
  if (bars < 1 || beat < 1 || beat > static_cast<int>(beatsPerBar) ||
      ticks < 0 || ticks >= 960 || bpm <= 0) {
    return -1;
  }

  // Convert to total beats
  double totalBeats = (bars - 1) * beatsPerBar +  // Bars start at 1
                      (beat - 1) +                // Beats start at 1
                      (ticks / 960.0);            // 960 ticks per beat

  return static_cast<int>(totalBeats * secondsPerBeat);
}

int TimeFormatConverter::timecodeToSeconds(const juce::String& val) {
  auto parts = juce::StringArray::fromTokens(val, ":", "");
  if (parts.size() != 4) {
    return -1;
  }
  if (!parts[0].containsOnly("0123456789") ||
      !parts[1].containsOnly("0123456789") ||
      !parts[2].containsOnly("0123456789") ||
      !parts[3].containsOnly("0123456789")) {
    return -1;
  }

  int hours = parts[0].getIntValue();
  int minutes = parts[1].getIntValue();
  int seconds = parts[2].getIntValue();
  int frames = parts[3].getIntValue();

  // Validate ranges (frames validation would require frame rate)
  if (minutes > 59 || seconds > 59 || hours < 0 || minutes < 0 || seconds < 0 ||
      frames < 0) {
    return -1;
  }

  // Note: Frames are ignored for now as we work with integer seconds
  // This means 00:00:05:00 and 00:00:05:29 both become 5 seconds
  // This is acceptable for the current use case but creates lossy conversion
  return (hours * 3600 + minutes * 60 + seconds);
}

juce::String TimeFormatConverter::getFormatDescription(TimeFormat format) {
  switch (format) {
    case TimeFormat::HoursMinutesSeconds:
      return "Format: Hours:Minutes:Seconds";
    case TimeFormat::BarsBeats:
      return "Format: Bars.Beats.Ticks";
    case TimeFormat::Timecode:
      return "Format: Timecode (HH:MM:SS:FF)";
    default:
      return "";
  }
}
