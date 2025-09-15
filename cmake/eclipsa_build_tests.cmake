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

# Function to build a single test executable from all the collected test sources
function(eclipsa_build_tests)
    get_property(test_sources GLOBAL PROPERTY ECLIPSA_TEST_SOURCES)
    get_property(test_link_libs GLOBAL PROPERTY ECLIPSA_TEST_LINK_LIBS)

    # Remove duplicates from the list of libraries
    list(REMOVE_DUPLICATES test_link_libs)

    add_executable(eclipsa_tests ${test_sources})
    target_compile_definitions(eclipsa_tests
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_SILENCE_XCODE_15_LINKER_WARNING)

    if(APPLE)
        set(VENDOR_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/third_party/lib/macos")
        target_link_directories(eclipsa_tests PRIVATE ${VENDOR_LIB_PATH})
    endif()

    if(DEFINED LIBIAMF_INCLUDE_DIRS)
        target_include_directories(eclipsa_tests PRIVATE ${LIBIAMF_INCLUDE_DIRS})
    endif()
    
    target_link_libraries(eclipsa_tests
        PRIVATE
            ${test_link_libs}
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
            GTest::gtest_main)

    gtest_discover_tests(eclipsa_tests
    DISCOVERY_MODE PRE_TEST)
endfunction()