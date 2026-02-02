# windows.cmake

if (DEFINED _WINDOWS_TOOLCHAIN_INCLUDED)
    return()
endif ()
set(_WINDOWS_TOOLCHAIN_INCLUDED TRUE)

set(CMAKE_SYSTEM_NAME Windows)

# Stack size
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:16777216")

# Disable vcpkg applocal copy
set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF CACHE BOOL "")

# Try environment variable first (same behavior as before)
if (NOT DEFINED VCPKG_ROOT AND DEFINED ENV{VCPKG_ROOT})
    set(VCPKG_ROOT "$ENV{VCPKG_ROOT}" CACHE PATH "Path to vcpkg root")
endif ()

if (DEFINED VCPKG_ROOT)
    set(_VCPKG_TOOLCHAIN "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

    if (NOT EXISTS "${_VCPKG_TOOLCHAIN}")
        message(FATAL_ERROR "vcpkg toolchain file not found at: ${_VCPKG_TOOLCHAIN}")
    endif ()

    if (NOT DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg target triplet")
    endif ()

    message(STATUS "vcpkg toolchain: ${_VCPKG_TOOLCHAIN}")
    message(STATUS "vcpkg triplet: ${VCPKG_TARGET_TRIPLET}")

    # Chain load (this replaces setting CMAKE_TOOLCHAIN_FILE)
    include("${_VCPKG_TOOLCHAIN}")
else ()
    message(WARNING "vcpkg not configured. Set VCPKG_ROOT or pass -DVCPKG_ROOT.")
endif ()
