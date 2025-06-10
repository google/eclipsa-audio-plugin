- Compiled from: https://github.com/gpac/gpac/tree/945c0e4b1bbf3b0278e94f8623a14e6bb061ea91
- Commit: https://github.com/gpac/gpac/commit/945c0e4b1bbf3b0278e94f8623a14e6bb061ea91
- Version: 1.0.0 (main branch - 945c0e4b1bbf3b0278e94f8623a14e6bb061ea91)

### Compile notes
- Compiled for ARM OSX (Sonoma 14.4.1, clang 15.0.0).
### Steps taken to build the library:
1. Ran the configure script with the following options: ``` ./configure --prefix=<install_path> --static-bin --use-curl=no ```. Building with these options is important as gpac tries to use available external libs, so we explicitly disable them (except for zlib currently).
2. Built and installed with `make -jxx` and `make install-lib`.