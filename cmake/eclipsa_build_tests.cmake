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
        set(VENDOR_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/macos")
        target_link_directories(eclipsa_tests PRIVATE ${VENDOR_LIB_PATH})
    elseif(WIN32)
        set(VENDOR_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/lib/Windows/${CMAKE_BUILD_TYPE}")
        target_link_directories(eclipsa_tests PRIVATE ${VENDOR_LIB_PATH})

        # Move the dlls required for the unit tests as well
        # This can be done in the post build, since we'll run the tests after the build is complete
        set(ZMQ_DLL         "${CMAKE_BINARY_DIR}/_deps/zeromq-build/bin/${BUILD_LIB_DIR}/libzmq-v143-mt$<$<CONFIG:Debug>:-gd>-4_3_6.dll")
        set(CRYPTOMD_DLL    "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/${CMAKE_BUILD_TYPE}/libcryptoMD.dll")
        set(LIBSSLMD_DLL    "${CMAKE_SOURCE_DIR}/third_party/gpac/lib/Windows/${CMAKE_BUILD_TYPE}/libsslMD.dll")  
        
        set(TEST_DIR "${CMAKE_BINARY_DIR}/${BUILD_LIB_DIR}")
        add_custom_command(TARGET eclipsa_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${TEST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ZMQ_DLL} "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CRYPTOMD_DLL} "${TEST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LIBSSLMD_DLL} "${TEST_DIR}/"
        COMMENT "Copied Test DLLs to ${TEST_DIR}"
        )
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