add_library(gpac INTERFACE)
target_include_directories(gpac INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include/")

if (DEFINED GPAC_DYLIB)
    # macOS: single dylib
    add_library(gpac_impl SHARED IMPORTED GLOBAL)
    set_target_properties(gpac_impl PROPERTIES
            IMPORTED_LOCATION "${GPAC_DYLIB}"
            LINKER_LANGUAGE CXX
    )
else ()
    # Windows: dll + lib with debug/release
    foreach (_file ${GPAC_DEBUG_DLL} ${GPAC_DEBUG_LIB} ${GPAC_RELEASE_DLL} ${GPAC_RELEASE_LIB})
        if (NOT EXISTS "${_file}")
            message(FATAL_ERROR "GPAC file not found: ${_file}")
        endif ()
    endforeach ()

    message(STATUS "GPAC Debug DLL: ${GPAC_DEBUG_DLL}")
    message(STATUS "GPAC Release DLL: ${GPAC_RELEASE_DLL}")

    add_library(gpac_impl SHARED IMPORTED GLOBAL)
    set_target_properties(gpac_impl PROPERTIES
            IMPORTED_LOCATION "${GPAC_RELEASE_DLL}"
            IMPORTED_LOCATION_DEBUG "${GPAC_DEBUG_DLL}"
            IMPORTED_LOCATION_RELEASE "${GPAC_RELEASE_DLL}"
            IMPORTED_IMPLIB "${GPAC_RELEASE_LIB}"
            IMPORTED_IMPLIB_DEBUG "${GPAC_DEBUG_LIB}"
            IMPORTED_IMPLIB_RELEASE "${GPAC_RELEASE_LIB}"
            LINKER_LANGUAGE CXX
    )
endif ()

target_link_libraries(gpac INTERFACE gpac_impl ${GPAC_EXTRA_LIBS})

if (GPAC_COPY_DLL)
    add_custom_target(copy_gpac_dll ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${GPAC_DEBUG_DLL},${GPAC_RELEASE_DLL}>"
            "${CMAKE_BINARY_DIR}/$<CONFIG>/libgpac.dll"
            COMMENT "Copying GPAC runtime DLL to build directory"
    )
endif ()