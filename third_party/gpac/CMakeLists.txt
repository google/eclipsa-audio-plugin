add_library(gpac INTERFACE)
target_include_directories(gpac INTERFACE "include/")

if(WIN32)
    # Platform-specific setup for Windows
    set(GPAC_PLATFORM "Windows")

    # Set paths for each configuration
    set(GPAC_DEBUG_DLL "${CMAKE_CURRENT_LIST_DIR}/lib/${GPAC_PLATFORM}/Debug/libgpac.dll")
    set(GPAC_DEBUG_LIB "${CMAKE_CURRENT_LIST_DIR}/lib/${GPAC_PLATFORM}/Debug/libgpac.lib")
    set(GPAC_RELEASE_DLL "${CMAKE_CURRENT_LIST_DIR}/lib/${GPAC_PLATFORM}/Release/libgpac.dll")
    set(GPAC_RELEASE_LIB "${CMAKE_CURRENT_LIST_DIR}/lib/${GPAC_PLATFORM}/Release/libgpac.lib")

    # Validate that the libraries exist
    if(NOT EXISTS "${GPAC_DEBUG_DLL}")
        message(FATAL_ERROR "GPAC Debug DLL not found at: ${GPAC_DEBUG_DLL}")
    endif()
    if(NOT EXISTS "${GPAC_DEBUG_LIB}")
        message(FATAL_ERROR "GPAC Debug LIB not found at: ${GPAC_DEBUG_LIB}")
    endif()
    if(NOT EXISTS "${GPAC_RELEASE_DLL}")
        message(FATAL_ERROR "GPAC Release DLL not found at: ${GPAC_RELEASE_DLL}")
    endif()
    if(NOT EXISTS "${GPAC_RELEASE_LIB}")
        message(FATAL_ERROR "GPAC Release LIB not found at: ${GPAC_RELEASE_LIB}")
    endif()

    message(STATUS "GPAC Debug DLL: ${GPAC_DEBUG_DLL}")
    message(STATUS "GPAC Release DLL: ${GPAC_RELEASE_DLL}")

    # Windows: dynamic library (.dll + .lib)
    add_library(gpac_archive SHARED IMPORTED GLOBAL)

    set_target_properties(gpac_archive PROPERTIES
            IMPORTED_LOCATION "${GPAC_RELEASE_DLL}"
            IMPORTED_LOCATION_DEBUG "${GPAC_DEBUG_DLL}"
            IMPORTED_LOCATION_RELEASE "${GPAC_RELEASE_DLL}"
            IMPORTED_IMPLIB "${GPAC_RELEASE_LIB}"
            IMPORTED_IMPLIB_DEBUG "${GPAC_DEBUG_LIB}"
            IMPORTED_IMPLIB_RELEASE "${GPAC_RELEASE_LIB}"
            LINKER_LANGUAGE CXX
    )

    # Copy DLL to build directory (configuration-aware)
    add_custom_target(copy_gpac_dll ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${GPAC_DEBUG_DLL},${GPAC_RELEASE_DLL}>"
            "${CMAKE_BINARY_DIR}/$<CONFIG>/libgpac.dll"
            COMMENT "Copying GPAC runtime DLL to build directory"
    )

    target_link_libraries(gpac INTERFACE gpac_archive ws2_32 shlwapi winmm)

else()
    add_library(gpac_shared SHARED IMPORTED)
    set_target_properties(gpac_shared PROPERTIES
            IMPORTED_LOCATION         "${CMAKE_CURRENT_LIST_DIR}/lib/libgpac.dylib"
            IMPORTED_LOCATION_DEBUG   "${CMAKE_CURRENT_LIST_DIR}/lib/libgpac.dylib"
            IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/lib/libgpac.dylib"
            IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}/lib/libgpac.dylib"
            LINKER_LANGUAGE CXX)
    target_link_libraries(gpac INTERFACE gpac_shared)
endif()
