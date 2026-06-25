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

juce::String TimeFormatConverter::secondsToHMS(double timeInSeconds) {
  int totalSeconds = static_cast<int>(timeInSeconds);
  int seconds = totalSeconds % 60;
  int minutes = (totalSeconds / 60) % 60;
  int hours = totalSeconds / 3600;

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
    double timeInSeconds, double bpm,
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

double TimeFormatConverter::hmsToSeconds(const juce::String& val) {
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

double TimeFormatConverter::barsBeatsToSeconds(
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

  return totalBeats * secondsPerBeat;
}

double TimeFormatConverter::timecodeToMs(const juce::String& val, double fps) {
  auto parts = juce::StringArray::fromTokens(val, ":", "");
  if (parts.size() != 4) return -1;

  for (auto& p : parts)
    if (!p.containsOnly("0123456789")) return -1;

  int hours = parts[0].getIntValue();
  int minutes = parts[1].getIntValue();
  int seconds = parts[2].getIntValue();
  int frames = parts[3].getIntValue();

  if (minutes > 59 || seconds > 59 || hours < 0 || minutes < 0 || seconds < 0 ||
      frames < 0 || frames >= (int)fps)
    return -1;

  double totalSeconds = hours * 3600 + minutes * 60 + seconds + (frames / fps);
  return totalSeconds * 1000.0;
}

juce::String TimeFormatConverter::msToTimecode(double ms, const double fps) {
  double totalSeconds = ms / 1000.0;
  int hh = (int)(totalSeconds / 3600);
  int mm = (int)(std::fmod(totalSeconds, 3600.0) / 60.0);
  int ss = (int)(std::fmod(totalSeconds, 60.0));
  int ff = (int)(std::fmod(totalSeconds, 1.0) * fps);
  return juce::String::formatted("%02d:%02d:%02d:%02d", hh, mm, ss, ff);
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
