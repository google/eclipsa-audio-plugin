Windows Build Setup

-MSVC (Visual Studio 2022)

-CMake 3.29+

-vcpkg for dependency management (see vcpkg.json)

-Intel OneAPI for MKL (Currently used in SAF, so potentially optional)

1. Clone vcpkg: 

-git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg\vcpkg C:\vcpkg\vcpkg\bootstrap-vcpkg.bat

2. Set your vcpkg root folder in your ide or pass to Cmake:

-DVCPKG_ROOT=C:/vcpkg/vcpkg
-DVCPKG_TARGET_TRIPLET=x64-windows

3. For SAF, download the Intel OneAPI Base Toolkit here - https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl-download.html

4. Pass the MKL root to Cmake - DMKL_ROOT="C:/Program Files (x86)/Intel/oneAPI/mkl/2025.2"

5. For AAX, pass your AAX root to Cmake - DAAX_SDK_ROOT=C:/SDKs/aax-sdk-2-7-0

6. Config and build: 

cmake -G "Visual Studio 17 2022" `
  -A x64 `
   -DVCPKG_ROOT=C:/vcpkg/vcpkg `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
   -DVCPKG_APPLOCAL_DEPS=OFF `
  -DBUILD_AAX=ON `
   -DMKL_ROOT="C:/Program Files (x86)/Intel/oneAPI/mkl/2025.2" `
  -DAAX_SDK_ROOT=C:/SDKs/aax-sdk-2-7-0 `
   -B cmake-build-release-visual-studio

cmake --build cmake-build-release-visual-studio --config Release
