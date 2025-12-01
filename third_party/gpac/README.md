- Compiled from: https://github.com/gpac/gpac/tree/945c0e4b1bbf3b0278e94f8623a14e6bb061ea91
- Commit: https://github.com/gpac/gpac/commit/945c0e4b1bbf3b0278e94f8623a14e6bb061ea91
- Version: 1.0.0 (main branch - 945c0e4b1bbf3b0278e94f8623a14e6bb061ea91)

### Compile notes on MacOs
- Compiled for ARM OSX (Sonoma 14.4.1, clang 15.0.0).
### Steps taken to build the library on macOS:
1. Ran the configure script with the following options: ``` ./configure --prefix=<install_path> --static-bin --use-curl=no ```. Building with these options is important as gpac tries to use available external libs, so we explicitly disable them (except for zlib currently).
2. Built and installed with `make -jxx` and `make install-lib`.

### Compile notes on Windows
- Compiled with MSVC for x64 on Windows 10

### Steps taken to build the library on Windows:
1. Clone repository: https://github.com/gpac/gpac
```
   git clone https://github.com/gpac/gpac.git 
   cd gpac
   git submodule update --init --recursive --force --checkout
   cd ..
```

2. Clone windows dependencies in root folder (outside gpac clone): https://github.com/gpac/deps_windows
```
   git clone https://github.com/gpac/deps_windows
   cd deps_windows
   git submodule update --init --recursive --force --checkout
```

3. Build dependencies (in Developer Command Prompt for VS):
```
   cd build\msvc
   msbuild.exe BuildAll_vc10.sln /maxcpucount /t:Build /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0
```
Note: xvid will fail without NASM installed, but is optional.

4. Copy built libraries to gpac_public:
```
   cd ..\..
   .\CopyLibs2Public.bat all
```

5. Build GPAC static library:
```
   cd ..\gpac\build\msvc14
   msbuild.exe libgpac.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0
```
Output: `gpac\bin\x64\Release\libgpac_static.lib`


