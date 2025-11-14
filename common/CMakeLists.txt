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

# -------------------------------
#  Core module setup
# -------------------------------

juce_add_modules(
    components
    data_repository
    data_structures
    logger
    player
    processors
    substream_rdr)

# -------------------------------
#  Optional test targets
# -------------------------------

if(CI_TEST OR INTERNAL_TEST)
    add_subdirectory(processors/tests)
    add_subdirectory(data_repository/tests)
    add_subdirectory(logger/tests)
    add_subdirectory(data_structures/tests)
    add_subdirectory(substream_rdr/tests)
    add_subdirectory(player/test)
    
endif()

# -------------------------------
#  Platform-specific processor defs
# -------------------------------

# DAW-specific build flags
if(ECLIPSA_LOGIC_PRO_BUILD)
    target_compile_definitions(processors INTERFACE ECLIPSA_LOGIC_PRO_BUILD=1)
endif()

# -------------------------------
#  Icon resources
# -------------------------------

add_subdirectory(components/icons)
target_link_libraries(components INTERFACE binary_data)

# Link components against Eigen (for AmbisonicsVisualizer)
target_link_libraries(components INTERFACE Eigen)

# -------------------------------
#  Common include directories
# -------------------------------

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

# -------------------------------
#  IAMF and Proto Integration
# -------------------------------

# Unified cross-platform IAMF linkage (no Bazel paths)
target_link_libraries(data_structures INTERFACE iamftools substream_rdr)

# Make sure our regenerated proto headers come first
target_include_directories(data_structures INTERFACE
        "${CMAKE_SOURCE_DIR}/third_party/iamftools/iamf/cli/proto"
)

# -------------------------------
#  Substream and Processor Linking
# -------------------------------

# Both Windows and macOS now link full audio stack (OBR + SAF)
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

target_link_libraries(player INTERFACE processors substream_rdr)

# -------------------------------
#  Components and Logging
# -------------------------------

target_link_libraries(components INTERFACE binary_data libear substream_rdr processors data_structures data_repository player)

if(WIN32)
    message(STATUS "Linking logger against Boost via CMake targets")

#    find_package(Boost 1.70.0 REQUIRED COMPONENTS log log_setup thread filesystem)

    target_link_libraries(logger INTERFACE
            Boost::log
            Boost::log_setup
            Boost::filesystem
            Boost::thread
            Boost::system
            juce::juce_core
    )

#    target_include_directories(logger INTERFACE ${Boost_INCLUDE_DIRS})
else()
    target_link_libraries(logger INTERFACE
            Boost::log
            Boost::log_setup
            Boost::thread
            Boost::filesystem
            juce::juce_core
    )
endif()

get_target_property(SUBSTREAM_INCLUDES substream_rdr INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "substream_rdr include paths: ${SUBSTREAM_INCLUDES}")
