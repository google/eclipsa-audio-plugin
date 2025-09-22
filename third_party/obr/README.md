- Compiled from: https://github.com/google/obr
- Commit: https://github.com/google/obr/commit/212982959ef5fe0de36f666c0450e11c711c58e8

### Compile notes
- Compiled for ARM OSX (Sonoma 14.4.1, clang 15.0.0).

### To Compile
- Install the bazel build system if you haven't already  (brew install bazel)
- Add a compile target to create the dylib:
    - Open obr/renderer/build
    - Add the following to the end of the file:
        cc_binary(
            name = "obr",
            deps = [
                ":obr_impl",
            ],
            linkshared = True,
        )

    - Add the following to the obr_impl target:
        linkstatic = False,
        alwayslink = True
      
      eg:
        cc_library(
          name = "obr_impl",
          srcs = [
              "obr_impl.cc",
          ],
          hdrs = ["obr_impl.h"],
          visibility = ["//visibility:public"],
          deps = [
              ":audio_element_config",
              ":audio_element_type",
              "//obr/ambisonic_binaural_decoder",
              "//obr/ambisonic_binaural_decoder:fft_manager",
              "//obr/ambisonic_binaural_decoder:resampler",
              "//obr/ambisonic_binaural_decoder:sh_hrir_creator",
              "//obr/ambisonic_encoder",
              "//obr/audio_buffer",
              "//obr/common",
              "//obr/peak_limiter",
              "@com_google_absl//absl/log",
              "@com_google_absl//absl/log:check",
              "@com_google_absl//absl/status",
              "@com_google_absl//absl/synchronization",
          ],
          linkstatic = False,
          alwayslink = True,
      )

- Move the Abseil version forward to something compatible with this project:
  - At current time of compile we're using abseil version 20250512.1
  - Update the following in the obr/MODULE.bazel file to the necessary Abseil version
    
    bazel_dep(
        name = "abseil-cpp",
        version = "20250512.1",
        repo_name = "com_google_absl",
    )


- Compile the dylib by running the following from the top level directory:
  bazel build --compilation_mode=opt //obr/renderer:obr 

- Rename the library to obr.dylib and copy to third_party/obr/lib

- Update the dylib RPATH by running the following command:
  install_name_tool -id "@rpath/third_party/obr/lib/libobr.dylib" libobr.dylib

- Verify the RPATH was succesfully updated by running the following and ensuring the above path is returned:
  otool -l obr.dylib | grep obr.dylib

- Update the third party source headers:
  - Copy obr/obr to third_party/obr/obr
  - Run the clean_directory.sh script in obr/scripts to remove all non-header files
      ./clean_directory.sh ../obr