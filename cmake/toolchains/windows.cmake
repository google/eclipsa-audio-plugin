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

# Include guard
if (DEFINED _WINDOWS_TOOLCHAIN_INCLUDED)
    return()
endif ()

set(_WINDOWS_TOOLCHAIN_INCLUDED TRUE)

# Math constants
add_compile_definitions(_USE_MATH_DEFINES)

# Reduce optimization to avoid MSVC compiler crash on SAF
set(CMAKE_C_FLAGS_RELEASE "/O1 /MD /DNDEBUG" CACHE STRING "" FORCE)

# Resource compiler fallback
if (NOT CMAKE_RC_COMPILER)
    find_program(CMAKE_RC_COMPILER rc.exe
            HINTS "C:/Program Files (x86)/Windows Kits/10/bin/*/x64")
endif ()

# Disable automatic DLL copying (we handle this manually)
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

# Initialize the base formats
set(_DEFAULT_FORMATS "Standalone")

if (BUILD_AAX)
    set(AAX_SDK_VER "2-8-1" CACHE STRING "AAX SDK Version")
    list(APPEND _DEFAULT_FORMATS "AAX")
    set(AAX_SDK_SEARCH_HINT "C:/Code/Repos/aax-sdk-${AAX_SDK_VER}" CACHE INTERNAL "")
endif ()

# Finalize the list into the CACHE so the project can see it
set(PLUGIN_FORMATS "${_DEFAULT_FORMATS}" CACHE STRING "Target plugin formats")

# --- ZLIB Configuration ---
find_package(ZLIB REQUIRED)

# We use a CACHE variable to store the library path/target
# so the rest of the project can access it easily.
set(ZLIB_LIBRARIES ${ZLIB_LIBRARIES} CACHE INTERNAL "ZLIB libraries for Windows")

# If you REALLY want every single target to link ZLIB automatically:
link_libraries(ZLIB::ZLIB)
link_libraries(delayimp)

set(IAMF_LIB_NAME "iamf" CACHE STRING "")

if (BUILD_VST3)
    list(APPEND _DEFAULT_FORMATS "VST3")
endif ()
