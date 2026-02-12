# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# You may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

#====================================================================
# Core module setup
#====================================================================
juce_add_modules(
        components
        data_repository
        data_structures
        logger
        processors
        substream_rdr)

#====================================================================
# Optional test targets
#====================================================================
if (CI_TEST OR INTERNAL_TEST)
    add_subdirectory(processors/tests)
    add_subdirectory(data_repository/tests)
    add_subdirectory(logger/tests)
    add_subdirectory(data_structures/tests)
    add_subdirectory(substream_rdr/tests)
endif ()

#====================================================================
# DAW-specific build flags
#====================================================================
if (ECLIPSA_LOGIC_PRO_BUILD)
    target_compile_definitions(processors INTERFACE ECLIPSA_LOGIC_PRO_BUILD=1)
endif ()

#====================================================================
# Icon resources
#====================================================================
add_subdirectory(components/icons)
target_link_libraries(components INTERFACE binary_data)
# Link components against Eigen (for AmbisonicsVisualizer)
target_link_libraries(components INTERFACE Eigen)

#====================================================================
# Common include directories
#====================================================================
target_include_directories(data_structures INTERFACE
        "${BOOST_LIBRARY_INCLUDES}/include"
        "${zeromq_SOURCE_DIR}/include"
        "${cppzmq_SOURCE_DIR}"
)
target_include_directories(substream_rdr INTERFACE
        "${spatialaudio_SOURCE_DIR}/include"
        "${spatialaudio_SOURCE_DIR}"
        "${spatialaudio_SOURCE_DIR}/include/dsp"
        "${spatialaudio_SOURCE_DIR}/include/hrtf"
        "${spatialaudio_SOURCE_DIR}/include/normal"
        "${spatialaudio_SOURCE_DIR}/source"
        "${spatialaudio_SOURCE_DIR}/source/kiss_fft"
        "${CMAKE_BINARY_DIR}/_deps/libspatialaudio-build"
        "${CMAKE_SOURCE_DIR}/third_party/libiamf/code/include"
)

#====================================================================
# IAMF and Proto Integration
#====================================================================
target_link_libraries(data_structures INTERFACE iamftools substream_rdr)

# Make sure our regenerated proto headers come first
target_include_directories(data_structures INTERFACE
        "${CMAKE_SOURCE_DIR}/third_party/iamftools/iamf/cli/proto"
)

#====================================================================
# Substream and Processor Linking
#====================================================================
target_link_libraries(substream_rdr INTERFACE
        libspatialaudio
        obr
        libear
        data_structures
)
target_link_libraries(processors INTERFACE
        gpac
        libear
        iamftools
        logger
        substream_rdr
        libzmq
        saf
        saf_example_ambi_dec
        lufs_meter
)
target_include_directories(substream_rdr INTERFACE ${OBR_INCLUDE_DIRS})
target_include_directories(processors INTERFACE ${OBR_INCLUDE_DIRS})

#====================================================================
# Components and Logging
#====================================================================
target_link_libraries(components INTERFACE binary_data libear substream_rdr processors data_structures data_repository)
target_link_libraries(logger INTERFACE
        Boost::log
        Boost::log_setup
        Boost::filesystem
        Boost::thread
        juce::juce_core
)

get_target_property(SUBSTREAM_INCLUDES substream_rdr INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "substream_rdr include paths: ${SUBSTREAM_INCLUDES}")

#====================================================================
# Unified plugin dependency target
#====================================================================
add_library(eclipsa_common INTERFACE)
target_link_libraries(eclipsa_common INTERFACE
        substream_rdr
        processors
        components
        logger
        data_repository
        data_structures
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
