add_library(gpac INTERFACE)
target_include_directories(gpac INTERFACE "include/")

if(WIN32)
    # Windows: dynamic library (.dll + .lib)
    add_library(gpac_archive SHARED IMPORTED)
    set_target_properties(gpac_archive PROPERTIES
            IMPORTED_LOCATION         "${CMAKE_CURRENT_LIST_DIR}/lib/Windows/${BUILD_LIB_DIR}/libgpac.dll"
            IMPORTED_LOCATION_DEBUG   "${CMAKE_CURRENT_LIST_DIR}/lib/Windows/${BUILD_LIB_DIR}/libgpac.dll"
            IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/lib/Windows/${BUILD_LIB_DIR}/libgpac.dll"
            IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}/lib/Windows/${BUILD_LIB_DIR}/libgpac.dll"
            IMPORTED_IMPLIB           "${CMAKE_CURRENT_LIST_DIR}/lib/Windows/${BUILD_LIB_DIR}/libgpac.lib"
            LINKER_LANGUAGE CXX)

    # Copy DLL to build directory
    add_custom_target(copy_gpac_dll ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_LIST_DIR}/lib/Windows/${BUILD_LIB_DIR}/libgpac.dll"
            "${CMAKE_BINARY_DIR}/libgpac.dll"
            COMMENT "Copying GPAC runtime DLL to build directory"
    )

    target_link_libraries(gpac INTERFACE gpac_archive ws2_32 shlwapi winmm)

else()
    # macOS: static library (.a)
    add_library(gpac_archive STATIC IMPORTED)
    set_target_properties(gpac_archive PROPERTIES
            IMPORTED_LOCATION         "${CMAKE_CURRENT_LIST_DIR}/lib/libgpac_static.a"
            IMPORTED_LOCATION_DEBUG   "${CMAKE_CURRENT_LIST_DIR}/lib/libgpac_static.a"
            IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/lib/libgpac_static.a"
            IMPORTED_LOCATION_RELWITHDEBINFO "${CMAKE_CURRENT_LIST_DIR}/lib/libgpac_static.a"
            LINKER_LANGUAGE CXX)

    # z-lib is an OSX system library
    target_link_libraries(gpac INTERFACE gpac_archive z)
endif()
