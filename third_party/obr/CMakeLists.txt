add_library(obr INTERFACE)
target_compile_options(obr INTERFACE "-w")

# Disable unneeded build options for dependencies
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EIGEN_MPL2_ONLY ON CACHE BOOL "" FORCE)
set(EIGEN_BUILD_PKGCONFIG OFF CACHE BOOL "" FORCE)
set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)

# Fetch Eigen and pffft
include("${CMAKE_SOURCE_DIR}/cmake/eigen.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/pffft.cmake")

# Expose header locations
target_include_directories(obr INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/obr
        ${CMAKE_CURRENT_SOURCE_DIR}/obr/renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/obr/ambisonic_encoder
        ${CMAKE_BINARY_DIR}/_deps/pffft-src
)

# Platform-specific linking
if (OBR_USE_STATIC)
    # Windows: static libs with debug/release
    target_link_libraries(obr INTERFACE
            $<$<CONFIG:Debug>:${OBR_DEBUG_LIB}>
            $<$<CONFIG:Release>:${OBR_RELEASE_LIB}>
            $<$<CONFIG:Debug>:${PFFFT_DEBUG_LIB}>
            $<$<CONFIG:Release>:${PFFFT_RELEASE_LIB}>
            Eigen
    )
else ()
    # macOS: dynamic library
    add_library(obr_dylib SHARED IMPORTED)
    set_target_properties(obr_dylib PROPERTIES
            IMPORTED_LOCATION "${OBR_DYLIB}"
            IMPORTED_LOCATION_DEBUG "${OBR_DYLIB}"
            IMPORTED_LOCATION_RELEASE "${OBR_DYLIB}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${OBR_DYLIB}"
            IMPORTED_SONAME "obr.dylib"
            LINKER_LANGUAGE CXX
    )

    target_link_libraries(obr
            INTERFACE
            obr_dylib
            Eigen
            pffft
    )
endif ()

# Make include dirs available to parent
set(OBR_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/obr
        ${CMAKE_CURRENT_SOURCE_DIR}/obr/renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/obr/ambisonic_encoder
)
set(OBR_INCLUDE_DIRS "${OBR_INCLUDE_DIRS}" PARENT_SCOPE)