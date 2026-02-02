# windows.cmake

if (DEFINED _WINDOWS_TOOLCHAIN_INCLUDED)
    return()
endif ()
set(_WINDOWS_TOOLCHAIN_INCLUDED TRUE)

set(CMAKE_SYSTEM_NAME Windows)

# Find resource compiler
if (NOT CMAKE_RC_COMPILER)
    find_program(CMAKE_RC_COMPILER rc.exe
            HINTS "C:/Program Files (x86)/Windows Kits/10/bin/*/x64")
endif ()

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

#Add math defines
add_compile_definitions(_USE_MATH_DEFINES)

# Reduce optimization to avoid MSVC compiler crash on SAF
set(CMAKE_C_FLAGS_RELEASE "/O1 /MD /DNDEBUG" CACHE STRING "" FORCE)
