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

# Add libraries with internal JUCE dependencies
juce_add_modules(
    components
    data_repository
    data_structures
    logger
    processors
    substream_rdr)


# Discover test targets if testing enabled
if(CI_TEST OR INTERNAL_TEST)
    add_subdirectory(processors/tests)
    add_subdirectory(data_repository/tests)
    add_subdirectory(logger/tests)
    add_subdirectory(data_structures/tests)
    add_subdirectory(substream_rdr/tests)
    
endif()

target_compile_definitions(processors INTERFACE MUXER_PATH="${CMAKE_SOURCE_DIR}/external/FFmpeg/bin/ffmpeg")

# Add DAW-specific build flags to processors module when enabled
if(ECLIPSA_LOGIC_PRO_BUILD)
    target_compile_definitions(processors INTERFACE ECLIPSA_LOGIC_PRO_BUILD=1)
endif()

add_subdirectory(components/icons)
target_link_libraries(components INTERFACE binary_data)

target_include_directories(data_structures INTERFACE "${BOOST_LIBRARY_INCLUDES}/include")
target_include_directories(data_structures INTERFACE "${zeromq_SOURCE_DIR}/include")
target_include_directories(data_structures INTERFACE "${cppzmq_SOURCE_DIR}")
target_include_directories(substream_rdr INTERFACE  "${spatialaudio_SOURCE_DIR}/include"
                                                    "${spatialaudio_SOURCE_DIR}"
                                                    "${spatialaudio_SOURCE_DIR}/include/dsp" 
                                                    "${spatialaudio_SOURCE_DIR}/include/hrtf"
                                                    "${spatialaudio_SOURCE_DIR}/include/normal" 
                                                    "${spatialaudio_SOURCE_DIR}/source" 
                                                    "${spatialaudio_SOURCE_DIR}/source/kiss_fft"
                                                    "${CMAKE_BINARY_DIR}/_deps/libspatialaudio-build"
                                                    "${CMAKE_SOURCE_DIR}/third_party/libiamf/code/include")

# Link libraries with internal dependencies on external libs
target_link_libraries(substream_rdr INTERFACE libspatialaudio obr libear data_structures)
target_link_libraries(processors INTERFACE gpac libear iamftools logger substream_rdr libzmq saf saf_example_ambi_dec lufs_meter)
target_link_libraries(components INTERFACE binary_data libear substream_rdr processors)
target_link_libraries(data_structures INTERFACE iamftools substream_rdr Boost::lockfree)
target_link_libraries(logger INTERFACE Boost::log Boost::log_setup Boost::thread Boost::filesystem juce::juce_core) 
