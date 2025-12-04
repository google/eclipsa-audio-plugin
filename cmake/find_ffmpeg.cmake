# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Find FFmpeg package with platform-independent detection
# Supports standard installations and homebrew on macOS
#
# Sets:
#   FFMPEG_TOOLS_FOUND - whether FFmpeg was found
#   FFMPEG_EXECUTABLE - path to ffmpeg executable
#   FFPROBE_EXECUTABLE - path to ffprobe executable

# Find ffmpeg and ffprobe executables
find_program(FFMPEG_EXECUTABLE ffmpeg)
find_program(FFPROBE_EXECUTABLE ffprobe)

# Check if executables were found
if(FFMPEG_EXECUTABLE AND FFPROBE_EXECUTABLE)
    set(FFMPEG_TOOLS_FOUND TRUE)
else()
    set(FFMPEG_TOOLS_FOUND FALSE)
    if(NOT FFMPEG_EXECUTABLE)
        message(WARNING "ffmpeg executable not found")
    endif()
    if(NOT FFPROBE_EXECUTABLE)
        message(WARNING "ffprobe executable not found")
    endif()
endif()
