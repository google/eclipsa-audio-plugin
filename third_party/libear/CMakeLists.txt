add_library(libear INTERFACE)
target_include_directories(libear INTERFACE "include/")
target_compile_options(libear INTERFACE "-w") # Silence is golden

#Cross-platform setup
if (WIN32)
    set(LIBEAR_FILE "libear.lib")
else()
    set(LIBEAR_FILE "libear.a")
endif()

# Check that the file actually exists
set(LIBEAR_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/${LIBEAR_FILE}")
if (NOT EXISTS "${LIBEAR_PATH}")
    message(FATAL_ERROR "libear prebuilt library not found at: ${LIBEAR_PATH}")
endif()

# Import prebuilt static library
add_library(libear_archive STATIC IMPORTED)
set_target_properties(libear_archive PROPERTIES
        IMPORTED_LOCATION_RELEASE "${LIBEAR_PATH}"
        IMPORTED_LOCATION_DEBUG   "${LIBEAR_PATH}"
        IMPORTED_LOCATION_RELWITHDEBINFO "${LIBEAR_PATH}"
)

target_link_libraries(libear 
  INTERFACE 
    libear_archive
    Boost::optional 
    Boost::variant 
    Boost::math 
    Boost::algorithm)