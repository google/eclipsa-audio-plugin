# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# You may not use this file except in compliance with the License.
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

# Suppress CMake 3.31+ warning about FindBoost being deprecated
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

set(BOOST_COMPONENTS log log_setup thread filesystem math)

message(STATUS "Fetching Boost")
include(FetchContent)
set(BOOST_ENABLE_CMAKE ON)

if(WIN32)
    set(Boost_USE_STATIC_RUNTIME OFF)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    add_definitions(-DBOOST_ALL_NO_LIB)

endif()

set(BOOST_LOCALE_ENABLE_ICU OFF)

FetchContent_Declare(
        Boost
        USES_TERMINAL_DOWNLOAD TRUE
        DOWNLOAD_NO_EXTRACT FALSE
        OVERRIDE_FIND_PACKAGE
        URL "https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.gz"
        CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF)
FetchContent_MakeAvailable(Boost)
find_package(Boost 1.86.0 COMPONENTS ${BOOST_COMPONENTS})


