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

#====================================================================
# Include Guard
#====================================================================
if (DEFINED _WINDOWS_TOOLCHAIN_INCLUDED)
    return()
endif ()
set(_WINDOWS_TOOLCHAIN_INCLUDED TRUE)

#====================================================================
# Platform Identifier
#====================================================================
set(ECLIPSA_PLATFORM "windows" CACHE STRING "" FORCE)

#====================================================================
# AAX SDK Path
#====================================================================
if (BUILD_AAX)
    set(AAX_SDK_VER "2-8-1" CACHE STRING "AAX SDK Version")
    set(AAX_SDK_SEARCH_HINT "C:/Code/Repos/aax-sdk-${AAX_SDK_VER}" CACHE INTERNAL "")
endif ()

#====================================================================
# Compiler Settings
#====================================================================
add_compile_definitions(_USE_MATH_DEFINES)
set(CMAKE_C_FLAGS_RELEASE "/O1 /MD /DNDEBUG" CACHE STRING "" FORCE)

#====================================================================
# Resource Compiler
#====================================================================
if (NOT CMAKE_RC_COMPILER)
    find_program(CMAKE_RC_COMPILER rc.exe
            HINTS "C:/Program Files (x86)/Windows Kits/10/bin/*/x64")
endif ()

#====================================================================
# VCPKG
#====================================================================
set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF CACHE BOOL "")

if (NOT DEFINED VCPKG_ROOT AND DEFINED ENV{VCPKG_ROOT})
    set(VCPKG_ROOT "$ENV{VCPKG_ROOT}" CACHE PATH "Path to vcpkg root")
endif ()

if (DEFINED VCPKG_ROOT)
    set(_VCPKG_TOOLCHAIN "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

    if (NOT EXISTS "${_VCPKG_TOOLCHAIN}")
        message(FATAL_ERROR "vcpkg toolchain not found at: ${_VCPKG_TOOLCHAIN}")
    endif ()

    if (NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg target triplet")
    endif ()

    message(STATUS "vcpkg toolchain: ${_VCPKG_TOOLCHAIN}")
    message(STATUS "vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")

    include("${_VCPKG_TOOLCHAIN}")
else ()
    message(WARNING "vcpkg not configured. Set VCPKG_ROOT environment variable or pass -DVCPKG_ROOT=<path>")
endif ()

#====================================================================
# Dependencies
#====================================================================
link_libraries(delayimp)

#====================================================================
# Build Settings
#====================================================================
set(IAMF_LIB_NAME "iamf" CACHE STRING "")
set(ECLIPSA_IAMF_LIB_DIR "${CMAKE_BINARY_DIR}/_deps/libiamf-build/$<CONFIG>" CACHE STRING "")
set(ECLIPSA_STATIC_LIB_SUFFIX ".lib" CACHE STRING "")
set(ECLIPSA_PLATFORM_LIBS
        vendored_gpac_crypto
        vendored_gpac_ssl
        ZLIB::ZLIB
        delayimp
        CACHE STRING ""
)

#====================================================================
# SAF Performance Library (Intel MKL)
#====================================================================
if (NOT DEFINED CACHE{SAF_PERFORMANCE_LIB})
    if (NOT DEFINED MKL_ROOT)
        set(MKL_ROOT "$ENV{MKLROOT}" CACHE PATH "Path to Intel MKL root directory")
    endif ()

    if (NOT MKL_ROOT OR NOT EXISTS "${MKL_ROOT}")
        message(STATUS "SAF: Intel MKL not found, falling back to OpenBLAS/LAPACKE")
        set(SAF_PERFORMANCE_LIB "SAF_USE_OPEN_BLAS_AND_LAPACKE" CACHE STRING "" FORCE)
    else ()
        message(STATUS "SAF: Found Intel MKL at: ${MKL_ROOT}")
        set(SAF_PERFORMANCE_LIB "SAF_USE_INTEL_MKL_LP64" CACHE STRING "" FORCE)
        set(INTEL_MKL_HEADER_PATH "${MKL_ROOT}/include" CACHE STRING "" FORCE)
        set(INTEL_MKL_LIB
                "${MKL_ROOT}/lib/mkl_intel_lp64.lib"
                "${MKL_ROOT}/lib/mkl_sequential.lib"
                "${MKL_ROOT}/lib/mkl_core.lib"
                CACHE STRING "" FORCE
        )

        if (NOT EXISTS "${INTEL_MKL_HEADER_PATH}/mkl.h")
            message(FATAL_ERROR "SAF: MKL header not found at ${INTEL_MKL_HEADER_PATH}/mkl.h")
        endif ()

        foreach (lib_file ${INTEL_MKL_LIB})
            if (NOT EXISTS "${lib_file}")
                message(FATAL_ERROR "SAF: MKL library not found: ${lib_file}")
            endif ()
        endforeach ()
    endif ()
endif ()

#====================================================================
# Boost Settings
#====================================================================
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "")
add_definitions(-DBOOST_ALL_NO_LIB)

#====================================================================
# Test DLLs
#====================================================================
set(ECLIPSA_TEST_DLLS
        "${CMAKE_BINARY_DIR}/_deps/zeromq-build/bin/$<CONFIG>/libzmq-v143-mt$<$<CONFIG:Debug>:-gd>-4_3_6.dll"
        "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/$<CONFIG>/libcryptoMD.dll"
        "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/$<CONFIG>/libsslMD.dll"
        CACHE STRING ""
)

#====================================================================
# Delay-load Libraries
#====================================================================
set(ECLIPSA_DELAYLOAD_LIBS
        vendored_gpac
        vendored_iamf_tools
        vendored_opensvc
        libzmq
        vendored_gpac_crypto
        vendored_gpac_ssl
        CACHE STRING ""
)

