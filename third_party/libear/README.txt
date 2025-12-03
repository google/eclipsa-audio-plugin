Compiled from: https://github.com/ebu/libear.git
Version: 0.9.0
Built: 2024-05-02T14:07:10

=== Compile Notes ===
- Compiled for ARM OSX

### Windows Build Instructions

=== Compile Notes ===
- Compiled with MSVC

1. install Boost headers via vcpkg

    cd C:\Dev\vcpkg  # Or your vcpkg path (C:\vcpkg\vcpkg, etc.)
     .\vcpkg install boost-headers:x64-windows

2. Clone libear and initialize submodules

    cd C:\Dev
    git clone https://github.com/ebu/libear.git
    cd libear
    git checkout v0.9.0
    git submodule update --init --recursive

3. Configure with cmake

    mkdir build
    cd build
    cmake .. -G "Visual Studio 17 2022" -A x64 `
        -DCMAKE_TOOLCHAIN_FILE="C:\Dev\vcpkg\scripts\buildsystems\vcpkg.cmake" `
        -DBOOST_ROOT="C:\Dev\vcpkg\installed\x64-windows" `
        -DBoost_INCLUDE_DIR="C:\Dev\vcpkg\installed\x64-windows\include" `
        -DCMAKE_BUILD_TYPE=Release

4. Build with cmake

    cmake --build . --config Release

5. Move generated lib file to third_party\libear\lib\Windows\Release


