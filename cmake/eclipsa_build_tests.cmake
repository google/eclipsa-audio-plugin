# Function to build a single test executable from all the collected test sources
function(eclipsa_build_tests)
    get_property(test_sources GLOBAL PROPERTY ECLIPSA_TEST_SOURCES)
    get_property(test_link_libs GLOBAL PROPERTY ECLIPSA_TEST_LINK_LIBS)

    # Remove duplicates from the list of libraries
    list(REMOVE_DUPLICATES test_link_libs)

    add_executable(unit_tests ${test_sources})
    target_compile_definitions(unit_tests
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JUCE_SILENCE_XCODE_15_LINKER_WARNING)
    target_link_options(unit_tests
        PUBLIC
            "-Wl"
            "-ld_classic")

    if(APPLE)
        set(VENDOR_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/libiamf/third_party/lib/macos")
        target_link_directories(unit_tests PRIVATE ${VENDOR_LIB_PATH})
    endif()

    if(DEFINED LIBIAMF_INCLUDE_DIRS)
        target_include_directories(unit_tests PRIVATE ${LIBIAMF_INCLUDE_DIRS})
    endif()
    
    target_link_libraries(unit_tests
        PRIVATE
            ${test_link_libs}
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags
            GTest::gtest_main)

    gtest_discover_tests(unit_tests
    DISCOVERY_MODE PRE_TEST)
endfunction()