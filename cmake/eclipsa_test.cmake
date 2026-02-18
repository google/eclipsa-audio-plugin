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

# Function to add a test executable for CTest.
# test_name:    Test executable name.
# test_source:  Test source file name.
# test_libs:    Test link libraries as ';' separated list.
function(eclipsa_add_test test_name test_source test_libs)
    set(absolute_test_source "${CMAKE_CURRENT_SOURCE_DIR}/${test_source}")

    get_property(test_sources GLOBAL PROPERTY ECLIPSA_TEST_SOURCES)
    set_property(GLOBAL PROPERTY ECLIPSA_TEST_SOURCES "${test_sources};${absolute_test_source}")

    # Only add codec libs when a test needs IAMF decode/encode stack
    set(_needs_iamf_codecs FALSE)
    if ("${test_libs}" MATCHES "(^|;)iamf(;|$)" OR "${test_libs}" MATCHES "(^|;)iamfdec_utils(;|$)")
        set(_needs_iamf_codecs TRUE)
    endif ()

    if (_needs_iamf_codecs)
        if (APPLE)
            set(LIBIAMF_CODEC_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/macos")
            list(APPEND test_libs
                    "${LIBIAMF_CODEC_PATH}/libopus.a"
                    "${LIBIAMF_CODEC_PATH}/libogg.a"
            )
        elseif (WIN32)
            set(LIBIAMF_CODEC_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/Windows")
            list(APPEND test_libs
                    $<IF:$<CONFIG:Debug>,${LIBIAMF_CODEC_PATH}/Debug/opus.lib,${LIBIAMF_CODEC_PATH}/Release/opus.lib>
                    $<IF:$<CONFIG:Debug>,${LIBIAMF_CODEC_PATH}/Debug/ogg.lib,${LIBIAMF_CODEC_PATH}/Release/ogg.lib>
            )
        endif ()
    endif ()

    # IAMF include dirs warning
    if (_needs_iamf_codecs AND NOT DEFINED LIBIAMF_INCLUDE_DIRS)
        message(WARNING "LIBIAMF_INCLUDE_DIRS not defined, but required by ${test_name}")
    endif ()

    # If iamfdec_utils requested, ensure iamf is also linked
    if (TARGET iamf AND "${test_libs}" MATCHES "(^|;)iamfdec_utils(;|$)")
        list(FIND test_libs "iamf" iamf_already_linked)
        if (iamf_already_linked EQUAL -1)
            list(APPEND test_libs iamf)
        endif ()
    endif ()

    get_property(test_link_libs GLOBAL PROPERTY ECLIPSA_TEST_LINK_LIBS)
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_resources")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/test_resources"
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endif ()

    set_property(GLOBAL PROPERTY ECLIPSA_TEST_LINK_LIBS "${test_link_libs};${test_libs}")
endfunction()
