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

# After FetchContent_MakeAvailable(protobuf)
set(Protobuf_INCLUDE_DIRS "${protobuf_SOURCE_DIR}/src" CACHE PATH "" FORCE)
set(Protobuf_LIBRARIES "${protobuf_BINARY_DIR}/libprotobuf.lib" CACHE STRING "" FORCE)
set(Protobuf_PROTOC_EXECUTABLE "${protobuf_BINARY_DIR}/protoc" CACHE FILEPATH "" FORCE)
set(Protobuf_DIR "${protobuf_BINARY_DIR}/cmake" CACHE PATH "" FORCE)

# Force static linking for all third-party dependencies
set(BUILD_SHARED_LIBS OFF)
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

#Ensure IAMF library is found and named correctly
if (WIN32)
    # MSVC builds iamf.lib (no 'lib' prefix)
    set(IAMF_LIB_NAME iamf)
    set(IAMF_LIB_DIR "${CMAKE_BINARY_DIR}/_deps/libiamf-build/$<CONFIG>")
else()
    # UNIX builds libiamf.a
    set(IAMF_LIB_NAME libiamf)
    set(IAMF_LIB_DIR "${CMAKE_BINARY_DIR}/_deps/libiamf-build")
endif()

# Make sure all subprojects can find it
link_directories(${IAMF_LIB_DIR})

# Export variables globally so modules can use ${IAMF_LIB_NAME}
set(IAMF_LIB_NAME ${IAMF_LIB_NAME} CACHE STRING "IAMF library name")
set(IAMF_LIB_DIR ${IAMF_LIB_DIR} CACHE STRING "IAMF library directory")

#Override some of the SAF variables
#Unit tests for SAF have been tested and they pass, confirming the SAF library is working as expected.
# Manually set the SAF_PERFORMANCE_LIB variables for each platform
set(SAF_BUILD_TESTS 0 CACHE BOOL "" FORCE)
set(SAF_BUILD_EXAMPLES 1 CACHE BOOL "" FORCE)
set(saf_example_list ambi_dec CACHE STRING "" FORCE)

# Default performance library (fallback)
set(SAF_PERFORMANCE_LIB "SAF_USE_OPEN_BLAS_AND_LAPACKE")

if(APPLE)
    message(STATUS "SAF: Using Apple Accelerate")
    set(SAF_PERFORMANCE_LIB "SAF_USE_APPLE_ACCELERATE")

elseif(WIN32)
    # Use system Intel MKL directly
    message(STATUS "SAF: Attempting to use system Intel MKL")

    # Allow user to specify MKL root, or use environment variable
    set(MKL_ROOT "$ENV{MKLROOT}" CACHE PATH "Path to Intel MKL root directory")

    if(NOT MKL_ROOT OR NOT EXISTS "${MKL_ROOT}")
        message(WARNING "SAF: Intel MKL not found. MKLROOT is not set or invalid: '${MKL_ROOT}'")
        message(WARNING "      Set MKL_ROOT variable or source the Intel oneAPI setvars script")
        message(WARNING "      Falling back to OpenBLAS/LAPACKE")
        set(SAF_PERFORMANCE_LIB "SAF_USE_OPENBLAS_AND_LAPACKE")
    else()
        message(STATUS "SAF: Found Intel MKL at: ${MKL_ROOT}")
        set(SAF_PERFORMANCE_LIB "SAF_USE_INTEL_MKL_LP64")

        # Construct paths relative to MKL_ROOT
        set(INTEL_MKL_HEADER_PATH "${MKL_ROOT}/include" CACHE STRING "" FORCE)

        set(INTEL_MKL_LIB
                "${MKL_ROOT}/lib/mkl_intel_lp64.lib"
                "${MKL_ROOT}/lib/mkl_sequential.lib"
                "${MKL_ROOT}/lib/mkl_core.lib"
                CACHE STRING "" FORCE)

        # Validate that critical files exist
        if(NOT EXISTS "${INTEL_MKL_HEADER_PATH}/mkl.h")
            message(FATAL_ERROR "SAF: MKL header not found at ${INTEL_MKL_HEADER_PATH}/mkl.h")
        endif()

        foreach(lib_file ${INTEL_MKL_LIB})
            if(NOT EXISTS "${lib_file}")
                message(FATAL_ERROR "SAF: MKL library not found: ${lib_file}")
            endif()
        endforeach()
    endif()
else()
    message(SEND_ERROR "SAF: Using OpenBLAS/LAPACKE (default)")
endif()

# Reduce optimization level for SAF to avoid compiler crash
if(WIN32)
    set(CMAKE_C_FLAGS_RELEASE "/O1 /MD /DNDEBUG" CACHE STRING "" FORCE)
endif()

# Build SAF from source on all platforms
message(STATUS "SAF: Building from source with ${SAF_PERFORMANCE_LIB}")
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/Spatial_Audio_Framework)

# gpac for AV muxing
add_subdirectory(gpac)
# lib spatialaudio for panning
include("${CMAKE_SOURCE_DIR}/cmake/libspatialaudio.cmake")
# LUFSMeter
add_subdirectory(LUFSMeter)

# IAMF decoder
set(IAMF_DEC_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/libiamf/code/include")
if(EXISTS ${IAMF_DEC_INCLUDE_DIR})
    include_directories(${IAMF_DEC_INCLUDE_DIR})
else()
    message(FATAL_ERROR "IAMF decoder include directory not found: ${IAMF_DEC_INCLUDE_DIR}")
endif()

# SAF includes (optional)
set(SAF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/Spatial_Audio_Framework/examples/include")
if(EXISTS ${SAF_INCLUDE_DIR})
    include_directories(${SAF_INCLUDE_DIR})
else()
    message(WARNING "SAF include directory not found: ${SAF_INCLUDE_DIR}")
endif()

# Windows flags
if(WIN32)
    add_compile_definitions(_USE_MATH_DEFINES)
endif()

#Force all projects to use locally fetched Protobuf
if (DEFINED protobuf_SOURCE_DIR AND DEFINED protobuf_BINARY_DIR)
    message(STATUS "Overriding Protobuf include/lib paths with fetched version")
    set(Protobuf_INCLUDE_DIRS "${protobuf_SOURCE_DIR}/src" CACHE PATH "" FORCE)
    set(Protobuf_LIBRARIES "${protobuf_BINARY_DIR}/libprotobuf.lib" CACHE STRING "" FORCE)
    set(Protobuf_PROTOC_EXECUTABLE "${protobuf_BINARY_DIR}/protoc" CACHE FILEPATH "" FORCE)
    set(Protobuf_DIR "${protobuf_BINARY_DIR}/cmake" CACHE PATH "" FORCE)
else()
    message(WARNING "protobuf_SOURCE_DIR not defined yet ΓÇö ensure FetchContent_MakeAvailable(protobuf) ran")
endif()

