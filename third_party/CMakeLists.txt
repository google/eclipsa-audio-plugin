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

#====================================================================
# IAMF library directory (platform-specific structure)
#====================================================================
if (WIN32)
    set(IAMF_LIB_DIR "${CMAKE_BINARY_DIR}/_deps/libiamf-build/$<CONFIG>")
else ()
    set(IAMF_LIB_DIR "${CMAKE_BINARY_DIR}/_deps/libiamf-build")
endif ()
# Make sure all subprojects can find it
link_directories(${IAMF_LIB_DIR})
set(IAMF_LIB_DIR ${IAMF_LIB_DIR} CACHE STRING "IAMF library directory")

#====================================================================
#Spatial Audio Framework
#Override some of the SAF variables
#Unit tests for SAF have been tested and they pass, confirming the SAF library is working as expected.
# Manually set the SAF_PERFORMANCE_LIB variables for each platform
#====================================================================
set(SAF_BUILD_TESTS 0 CACHE BOOL "" FORCE)
set(SAF_BUILD_EXAMPLES 1 CACHE BOOL "" FORCE)
set(saf_example_list ambi_dec CACHE STRING "" FORCE)
# Default performance library (fallback)
set(SAF_PERFORMANCE_LIB "SAF_USE_OPEN_BLAS_AND_LAPACKE")

if (APPLE)
    message(STATUS "SAF: Using Apple Accelerate")
    set(SAF_PERFORMANCE_LIB "SAF_USE_APPLE_ACCELERATE")

elseif (WIN32)
    # Use system Intel MKL directly
    message(STATUS "SAF: Attempting to use system Intel MKL")

    # Allow user to specify MKL root, or use environment variable
    set(MKL_ROOT "$ENV{MKLROOT}" CACHE PATH "Path to Intel MKL root directory")

    if (NOT MKL_ROOT OR NOT EXISTS "${MKL_ROOT}")
        message(WARNING "SAF: Intel MKL not found. MKLROOT is not set or invalid: '${MKL_ROOT}'")
        message(WARNING "      Set MKL_ROOT variable or source the Intel oneAPI setvars script")
        message(WARNING "      Falling back to OpenBLAS/LAPACKE")
        set(SAF_PERFORMANCE_LIB "SAF_USE_OPENBLAS_AND_LAPACKE")
    else ()
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
        if (NOT EXISTS "${INTEL_MKL_HEADER_PATH}/mkl.h")
            message(FATAL_ERROR "SAF: MKL header not found at ${INTEL_MKL_HEADER_PATH}/mkl.h")
        endif ()

        foreach (lib_file ${INTEL_MKL_LIB})
            if (NOT EXISTS "${lib_file}")
                message(FATAL_ERROR "SAF: MKL library not found: ${lib_file}")
            endif ()
        endforeach ()
    endif ()
else ()
    message(SEND_ERROR "SAF: Using OpenBLAS/LAPACKE (default)")
endif ()

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
if (EXISTS ${IAMF_DEC_INCLUDE_DIR})
    include_directories(${IAMF_DEC_INCLUDE_DIR})
else ()
    message(FATAL_ERROR "IAMF decoder include directory not found: ${IAMF_DEC_INCLUDE_DIR}")
endif ()

# SAF includes (optional)
set(SAF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/Spatial_Audio_Framework/examples/include")
if (EXISTS ${SAF_INCLUDE_DIR})
    include_directories(${SAF_INCLUDE_DIR})
else ()
    message(WARNING "SAF include directory not found: ${SAF_INCLUDE_DIR}")
endif ()

#Force all projects to use locally fetched Protobuf
if (DEFINED protobuf_SOURCE_DIR AND DEFINED protobuf_BINARY_DIR)
    message(STATUS "Overriding Protobuf include/lib paths with fetched version")
    set(Protobuf_INCLUDE_DIRS "${protobuf_SOURCE_DIR}/src" CACHE PATH "" FORCE)
    if (WIN32)
        set(Protobuf_LIBRARIES "${protobuf_BINARY_DIR}/libprotobuf.lib" CACHE STRING "" FORCE)
    else ()
        set(Protobuf_LIBRARIES "${protobuf_BINARY_DIR}/libprotobuf.a" CACHE STRING "" FORCE)
    endif ()
    set(Protobuf_PROTOC_EXECUTABLE "${protobuf_BINARY_DIR}/protoc" CACHE FILEPATH "" FORCE)
    set(Protobuf_DIR "${protobuf_BINARY_DIR}/cmake" CACHE PATH "" FORCE)
else ()
    message(WARNING "protobuf_SOURCE_DIR not defined yet — ensure FetchContent_MakeAvailable(protobuf) ran")
endif ()

#====================================================================
# Imported Targets
#====================================================================
# 1. GPAC (renamed to vendored_gpac)
if (NOT TARGET vendored_gpac)
    add_library(vendored_gpac SHARED IMPORTED GLOBAL)
    if (APPLE)
        set_target_properties(vendored_gpac PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/gpac/lib/libgpac.dylib")
    elseif (WIN32)
        set_target_properties(vendored_gpac PROPERTIES
                IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/gpac/lib/Windows/Release/libgpac.dll"
                IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/gpac/lib/Windows/Debug/libgpac.dll"
                IMPORTED_IMPLIB_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/gpac/lib/Windows/Release/libgpac.lib"
                IMPORTED_IMPLIB_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/gpac/lib/Windows/Debug/libgpac.lib"
        )
    endif ()
endif ()

# 2. IAMF Tools (renamed to vendored_iamf_tools)
if (NOT TARGET vendored_iamf_tools)
    add_library(vendored_iamf_tools SHARED IMPORTED GLOBAL)
    if (APPLE)
        set_target_properties(vendored_iamf_tools PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/iamftools/lib/libiamf_tools.dylib")
    elseif (WIN32)
        set_target_properties(vendored_iamf_tools PROPERTIES
                IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/iamftools/lib/Windows/Release/iamf_tools.dll"
                IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/iamftools/lib/Windows/Debug/iamf_tools.dll"
                IMPORTED_IMPLIB_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/iamftools/lib/Windows/Release/iamf_tools.lib"
                IMPORTED_IMPLIB_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/iamftools/lib/Windows/Debug/iamf_tools.lib"
        )
    endif ()
endif ()

# 3. OBR (Mac Only - renamed to vendored_obr)
if (APPLE AND NOT TARGET vendored_obr)
    add_library(vendored_obr SHARED IMPORTED GLOBAL)
    set_target_properties(vendored_obr PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/obr/lib/obr.dylib")
endif ()

# 4. Windows-Specific Helpers
if (WIN32)
    if (NOT TARGET vendored_opensvc)
        add_library(vendored_opensvc SHARED IMPORTED GLOBAL)
        set_target_properties(vendored_opensvc PROPERTIES
                IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/OpenSVC/lib/Windows/Release/OpenSVCDecoder.dll"
                IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/OpenSVC/lib/Windows/Debug/OpenSVCDecoder.dll"
        )
    endif ()

    if (NOT TARGET vendored_gpac_crypto)
        add_library(vendored_gpac_crypto SHARED IMPORTED GLOBAL)
        set_target_properties(vendored_gpac_crypto PROPERTIES
                IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/gpac/lib/Windows/Release/libcryptoMD.dll"
                IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/gpac/lib/Windows/Debug/libcryptoMD.dll"
        )
    endif ()

    if (NOT TARGET vendored_gpac_ssl)
        add_library(vendored_gpac_ssl SHARED IMPORTED GLOBAL)
        set_target_properties(vendored_gpac_ssl PROPERTIES
                IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/gpac/lib/Windows/Release/libsslMD.dll"
                IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/gpac/lib/Windows/Debug/libsslMD.dll"
        )
    endif ()
endif ()