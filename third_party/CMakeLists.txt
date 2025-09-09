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

include(FetchContent)

# Force static linking for all third-party dependencies
set(BUILD_SHARED_LIBS OFF)

# JUCE for plugin development
add_subdirectory(JUCE)
# IAMF for IAMF file export
add_subdirectory(iamftools)
# lib open binaural renderer for panning via ambisonics and binaural rendering.
add_subdirectory(obr)
# Libear for loudspeaker rendering
include("${CMAKE_SOURCE_DIR}/cmake/boost.cmake")
add_subdirectory(libear)
# ZeroMQ for communication between processors
include("${CMAKE_SOURCE_DIR}/cmake/zeromq.cmake")
# LibIAMF for audio file import
add_subdirectory(libiamf)
# Override some of the SAF variables
# Units tests for SAF have been tested and they pass, confirming that the SAF library is working as expected.
set(SAF_BUILD_TESTS 0)
set(saf_example_list ambi_dec) #the ambisonic decoder example
set(SAF_BUILD_EXAMPLES 1) # only builds the examples in saf_example_list
# Manually set the SAF_PERFORMANCE_LIB variable
set(SAF_PERFORMANCE_LIB "SAF_USE_APPLE_ACCELERATE")
add_subdirectory(Spatial_Audio_Framework)
# gpac for AV muxing
add_subdirectory(gpac)
# lib spatialaudio for panning
include("${CMAKE_SOURCE_DIR}/cmake/libspatialaudio.cmake")

add_subdirectory(LUFSMeter)