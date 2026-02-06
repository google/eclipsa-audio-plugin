# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

include(FetchContent)

# Force static linking for all third-party dependencies
set(BUILD_SHARED_LIBS OFF)

#====================================================================
# Prebuilt Libraries
#====================================================================
include("${CMAKE_SOURCE_DIR}/cmake/prebuiltLibs/${ECLIPSA_PLATFORM}.cmake")

#====================================================================
# Add Dependencies
#====================================================================
#JUCE for plugin development.
add_subdirectory(JUCE)
#IAMF for IAMF file export.
add_subdirectory(iamftools)
# lib open binaural renderer for panning via ambisonics and binaural rendering.
add_subdirectory(obr)
# Libear for loudness rendering.
include("${CMAKE_SOURCE_DIR}/cmake/boost.cmake")
# Configure libear-build to use bundled dependencies and disable tests
add_subdirectory(libear)
#ZeroMQ for communication between processors.
include("${CMAKE_SOURCE_DIR}/cmake/zeromq.cmake")
#LibIAMF for audio file import.
add_subdirectory(libiamf)
# gpac for AV muxing
add_subdirectory(gpac)
# lib spatialaudio for panning
include("${CMAKE_SOURCE_DIR}/cmake/libspatialaudio.cmake")
# LUFSMeter
add_subdirectory(LUFSMeter)
#====================================================================
# IAMF library directory
#====================================================================
set(IAMF_LIB_DIR ${ECLIPSA_IAMF_LIB_DIR})

# Make sure all subprojects can find it
link_directories(${IAMF_LIB_DIR})
set(IAMF_LIB_DIR ${IAMF_LIB_DIR} CACHE STRING "IAMF library directory")

# IAMF decoder
set(IAMF_DEC_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/libiamf/code/include")
if (EXISTS ${IAMF_DEC_INCLUDE_DIR})
    include_directories(${IAMF_DEC_INCLUDE_DIR})
else ()
    message(FATAL_ERROR "IAMF decoder include directory not found: ${IAMF_DEC_INCLUDE_DIR}")
endif ()

#====================================================================
#Spatial Audio Framework
#====================================================================
set(SAF_BUILD_TESTS 0 CACHE BOOL "" FORCE)
set(SAF_BUILD_EXAMPLES 1 CACHE BOOL "" FORCE)
set(saf_example_list ambi_dec CACHE STRING "" FORCE)

# Manually set the SAF_PERFORMANCE_LIB variables for each platform
message(STATUS "SAF: Building from source with ${SAF_PERFORMANCE_LIB}")
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/Spatial_Audio_Framework)

# SAF includes
set(SAF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/Spatial_Audio_Framework/examples/include")
if (EXISTS ${SAF_INCLUDE_DIR})
    include_directories(${SAF_INCLUDE_DIR})
else ()
    message(WARNING "SAF include directory not found: ${SAF_INCLUDE_DIR}")
endif ()

#====================================================================
# Protobuf
#====================================================================
if (DEFINED protobuf_SOURCE_DIR AND DEFINED protobuf_BINARY_DIR)
    message(STATUS "Overriding Protobuf include/lib paths with fetched version")
    set(Protobuf_INCLUDE_DIRS "${protobuf_SOURCE_DIR}/src" CACHE PATH "" FORCE)
    set(Protobuf_LIBRARIES "${protobuf_BINARY_DIR}/libprotobuf${ECLIPSA_STATIC_LIB_SUFFIX}" CACHE STRING "" FORCE)
    set(Protobuf_PROTOC_EXECUTABLE "${protobuf_BINARY_DIR}/protoc" CACHE FILEPATH "" FORCE)
    set(Protobuf_DIR "${protobuf_BINARY_DIR}/cmake" CACHE PATH "" FORCE)
else ()
    message(WARNING "protobuf_SOURCE_DIR not defined yet — ensure FetchContent_MakeAvailable(protobuf) ran")
endif ()

