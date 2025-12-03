add_library(libear INTERFACE)
target_include_directories(libear INTERFACE "include/")
target_compile_options(libear INTERFACE "-w") # Silence is golden

#Cross-platform setup
if(WIN32)
    set(LIBEAR_DEBUG_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/Windows/Debug/libear.lib")
    set(LIBEAR_RELEASE_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/Windows/Release/libear.lib")
else()
    set(LIBEAR_DEBUG_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/libear.a")
    set(LIBEAR_RELEASE_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/libear.a")
endif()

# Check that the file actually exists
if(NOT EXISTS "${LIBEAR_DEBUG_PATH}")
    message(FATAL_ERROR "libear Debug library not found at: ${LIBEAR_DEBUG_PATH}")
endif()

if(NOT EXISTS "${LIBEAR_RELEASE_PATH}")
    message(FATAL_ERROR "libear Release library not found at: ${LIBEAR_RELEASE_PATH}")
endif()

# Import prebuilt static library
add_library(libear_archive STATIC IMPORTED)
set_target_properties(libear_archive PROPERTIES
        IMPORTED_LOCATION "${LIBEAR_RELEASE_PATH}"
        IMPORTED_LOCATION_DEBUG "${LIBEAR_DEBUG_PATH}"
        IMPORTED_LOCATION_RELEASE "${LIBEAR_RELEASE_PATH}"
        IMPORTED_LOCATION_RELWITHDEBINFO "${LIBEAR_RELEASE_PATH}"
        IMPORTED_LOCATION_MINSIZEREL "${LIBEAR_RELEASE_PATH}"
)

target_link_libraries(libear 
  INTERFACE 
    libear_archive
    Boost::optional 
    Boost::variant 
    Boost::math 
    Boost::algorithm)