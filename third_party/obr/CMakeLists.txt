add_library(obr INTERFACE)
target_compile_options(obr INTERFACE "-w") # Silence is golden

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

# Platform-specific impl defined in cmake/prebuiltLibs/*.cmake
target_link_libraries(obr INTERFACE obr_impl Eigen pffft)

# Make include dirs available to parent
set(OBR_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/obr
        ${CMAKE_CURRENT_SOURCE_DIR}/obr/renderer
        ${CMAKE_CURRENT_SOURCE_DIR}/obr/ambisonic_encoder
)
set(OBR_INCLUDE_DIRS "${OBR_INCLUDE_DIRS}" PARENT_SCOPE)