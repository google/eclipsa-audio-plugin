- Compiled from: https://github.com/AOMediaCodec/iamf-tools.git
- Commit: https://github.com/AOMediaCodec/iamf-tools/commit/93471884b25e8a5bb7ef43c1330aa90b37a574b0
- Version: 1.0.0

### Update notes
- The commit hash on line 2 must be updated to reflect the commit used for compiling the .dylib file.
- Ensure the format of line 2 remains unchanged, as the commit hash is required for logging purposes.
### Compile notes
- Compiled for ARM OSX (Sonoma 14.4.1, clang 15.0.0).
### Steps taken to integrate library:
1. In the `iamf-tools` repository, modified `iamf-tools/iamf/cli/build`. Replaced the declaration of a final binary target (`cc_binary`) with a shared library target (`cc_shared_library`) and removed sources from the modified target.
2. Ran the Bazel build with `bazel build --copt="-g" --strip="never"  //iamf/cli:encoder_main --macos_minimum_os=14 --spawn_strategy=standalone --cxxopt="-std=c++20"`.
3. Copied the resulting .dylib file to the repository, the header (`encoder_main_lib.h`) declaring the desired entry points into the library (`TestMain(...)`), and the protocol buffers used by the library.
4. The .dylib file contains an internal reference to where it was built. This path must be updated for the library to be linkable/loadable on OSX. This path was updated via an OSX command `install_name_tool @rpath/third_party/iamftools/lib/lib_encoder_main.dylib /path/to/.dylib` to use a relative path internally. *A CMake command setting the RPath to the repository root was also added.*
5. Added the `protobuf` library as a submodule to build the protoc compiler to generate files required by `iamf-tools`, and to provide other non-generated required headers.
### Summary of steps taken to integrate library:
- In the `iamf-tools` repo: replace the binary target with a shared library target in the iamf/cli/build file. Build the repo.
- Copy the shared library, headerfile for library entrypoint, and .proto files over.
- Update the internal path of the .dylib via the OSX command line utility.
- Add protobufs as a submodule, as its CMake API is necessary for generating proto files at buildtime (and we need a couple headers). 
