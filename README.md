# Eclipsa Audio Plugins

## Building the Plugins

### Dependencies

- MacOS 14.7.1 or Windows 11
- CMake 3.29+.
- Git LFS
- For MacOS support:
	- Clang 14.0.3 or later
- For Windows support:
	- Intel OneAPI for MKL
	- vcpkg
	- MSVC (Visual Studio 2022)
- For AAX Plugin support:
	- Avid Developer Tools:
		1. AAX SDK. 
		2. ProToolsDev DAW. 

Please note: The AAX SDK is required to build the plug-in in AAX format and the development version of ProTools is required to load locally built, unsigned AAX plugins. 
Avid developer tools require an Avid account, and in the case of the ProTools development DAW, a developer license to use. Licenses can be acquired by emailing devauth@avid.com, which also provides details on configuring the license through iLok. The AAX SDK and ProToolsDev are available at [Avid Developer Tools](https://my.avid.com/products/cppsdk?toolkit=AAX).


### Windows Build Setup
Building on windows requires the following additional setup steps:

1. Install the Intel OneAPI for MKL
    - Download the Intel OneAPI Base Toolkit here - https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl-download.html
    - During building, pass the MKL root to CMake
    ```
    -DMKL_ROOT="C:/Program Files (x86)/Intel/oneAPI/mkl/2025.2"
    ```

2. Clone vcpkg
    - Clone the vcpkg repository with
    ```
    git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg\vcpkg
    ```
    - Set the vcpkg information when running CMake
    ```
    -DVCPKG_ROOT=C:/vcpkg/vcpkg
    -DVCPKG_TARGET_TRIPLET=x64-windows
    ```


### Building Plugins Locally with CMake

To start, install git-lfs from: https://git-lfs.com/

Clone the repository and populate submodules.
```
git clone --recurse-submodules <Repository URL>
```
Currently the default plugin format that is built is 'Standalone' - a plugin format specific to JUCE which does not require external libraries nor a DAW to host it. This format is useful for quick builds, and is the format built by our CI. On MacOS, the Standalone version of the plugin is AU. Additional formats can be added using the CMake flags.

The CMake build supports the following flags:
| Flag                         | Configured Parameter                                       |
|------------------------------|------------------------------------------------------------|
| -DBUILD_VST3=ON              | Compile the plugin for VST3 format                         |
| -DBUILD_AAX=ON               | Compiled the plugin for AAX format                         |
| -DINTERNAL_TEST=ON           | Compile unit tests                                         |
| -DECLIPSA_VERSION=0.0.1      | Add the specified version information to the build         |
| -DECLIPSA_LOGIC_PRO_BUILD=ON | Compile the AU plugin for LogicPro (reduces channel width) |

#### Building For MacOS

On MacOS, run the following command to configure the CMake build, adding any desired flags as needed
```
cmake -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE --no-warn-unused-cli -S ./ -B ./build -G "Unix Makefiles"
```

Or using Ninja
```
cmake -B ./build -DBUILD_AAX=ON -DCMAKE_BUILD_TYPE=Release -G Ninja
```

Finally, after running the generate command above, the plugin can be built with the following command
```
cmake --build ./build
```

Alternatively use the Cmake extension for Visual Studio Code, adding the following to settings.json:
```
    "cmake.configureSettings": {
        "BUILD_AAX": "ON", # Add flags as needed
    }

```
Then execute the Configure and Build commands via the CMake extension.


#### Building For Windows

On Windows, run the following command to configure the CMake build:
```
cmake.exe -DVCPKG_ROOT:STRING=C:\Code\Repos\vcpkg "-DMKL_ROOT:STRING=C:\Program Files (x86)\Intel\oneAPI\mkl\2025.3" -DVCPKG_TARGET_TRIPLET:STRING=x64-windows -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE --no-warn-unused-cli -S ./ -B ./build -G "Visual Studio 17 2022" -T host=x64 -A x64
```

The plugin can then be built with the following command:
```
cmake --build ./build --config=Release
```

Alternativel, use the CMake extension for Visual Studio Code adding the following to settings.json:
```
    "cmake.configureSettings": {
        "VCPKG_ROOT": "C:\\Code\\Repos\\vcpkg",
        "MKL_ROOT": "C:\\Program Files (x86)\\Intel\\oneAPI\\mkl\\2025.3",
        "VCPKG_TARGET_TRIPLET": "x64-windows",
        "BUILD_AAX": "ON", # Add flags as needed
    }
```
Then execute the Configure and Build commands via the CMake extension.

After building, unit tests can be run by running the ```ctest``` command in the build folder of the project.


### Debugging the Plugin Locally

The Eclipsa Audio Plugins are built on top of the JUCE framework making the JUCE Audio Plugin Host an ideal way to perform local debugging.

To configure the JUCE Audio Plugin Host:
- Copy the JUCE repository from /External to a new location
- Run the following commands to install JUCE to your workspace directory
```
cmake -B cmake-build-install -DCMAKE_INSTALL_PREFIX=/path/to/vscode/workspace/lib/JUCE_Install -DJUCE_BUILD_EXTRAS=ON
cmake --build cmake-build-install --target install --config Debug
```
- Launch the Audio Plugin Host either directly or via Visual Studio Configuration
```
/JUCE/cmake-build-install/extras/AudioPluginHost/AudioPluginHost_artefacts/AudioPluginHost.app
```

Alternatively, the plugin can be loaded into ProTools developer version and the debugger attached to it.

### Running Unit Tests

After compilation with unit testing enabled, the test runner executable will be located in the `build/` folder. To filter for specific tests, run the test executable through `ctest` with the test string e.g./ for rendering specific tests, `ctest -R rdr`.

For convenient development in VSCode, a reference debug configuration is provided.

```
    "configurations": [
        {
            "name": "Run Filtered Tests",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/eclipsa_tests",
            "args": [
                "--gtest_filter=*${input:testFilter}*"
            ],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}/build",
            "environment": [],
            "externalConsole": false,
            "MIMode": "lldb"
        }
    ],
    "inputs": [
        {
            "id": "testFilter",
            "type": "promptString",
            "description": "Enter test name or pattern to filter (e.g. TestSuite.TestName or TestSuite.*)"
        }
    ]
```

### Loading Plugin to a DAW

The JUCE CMake API has a flag to copy the plugin in its various formats to the default locations for DAW plugins i.e./ on OSX, copying RendererPlugin.component to ```/Library/Audio/Plug-Ins/Components```. We're developing on OSX and haven't found any problems with it. 

It's possible that the plugin, though successfully copied, may not be visible within the DAW. In this case, restarting the DAW typically rescans plugin folders and the plugin should then be available for application on a track.

Please note: For AAX plugins, the plugin will not be available in non developer versions of ProTools without first being signed. Signing AAX plugins is beyond the scope of this document

### Diagnosing Pro Tools Crashes

Should a crash occur with the plugin during Pro Tools runtime, collecting crash data can aid in identifying the issue. The most useful data in diagnosing the cause of a crash is in the Pro Tools core dump. This can be located at `~/Library/Application\ Support/Avid/Pro\ Tools/Crashpad/completed`, and the most recent .dmp file collected. In addition to the core dump data, recent logs can be collected from `/tmp/Eclipsa_Audio_Plugin`. Logs and core dump files can then be attached to issues opened in Github.

### Running scripts to get installer:

The project provides an installer for the AAX plugin which can be generated using the "package.sh" script in the scripts folder.

1. Build the Plugins

	Run the build command to generate release .aaxplugin files for both the Eclipsa Audio Renderer Plugin and Eclipsa Audio Element Plugin. 
	```
	cmake -B ./build -DBUILD_AAX=ON -DCMAKE_BUILD_TYPE=Release -DECLIPSA_VERSION=0.0.1 -G Ninja
	cmake --build ./build
	```
	During the build, necessary libraries will be copied to each plugin’s Resources folder.

	- Renderer Plugin Resources Path: `build/rendererplugin/RendererPlugin_artefacts/Release/AAX/Eclipsa Audio Renderer.aaxplugin/Contents/Resources`
	- Audio Element Plugin Resources Path: `build/audioelementplugin/AudioElementPlugin/Release/AAX/Eclipsa Audio Element Plugin.aaxplugin/Contents/Resources`

2. Verify Libraries and Symbolic Links

	To confirm that libraries are correctly copied and symbolic links are in place:
	1. Navigate to each plugin’s `Resources` folder:
		```
	 	cd build/rendererplugin/RendererPlugin_artefacts/Release/AAX/Eclipsa Audio Renderer.aaxplugin/Contents/Resources
	 	cd build/audioelementplugin/AudioElementPlugin/Release/AAX/Eclipsa Audio Element Plugin.aaxplugin/Contents/Resources
	 	```
	2. Run the following command to check for symbolic links:
		```
		ls -l
		```
	    Expected output should include symbolic links for `libzmq`:
		```
		libzmq.5.2.6.dylib
		libzmq.5.dylib -> libzmq.5.2.6.dylib
		libzmq.dylib -> libzmq.5.dylib
		```

3. Create the Installer:
		```
		sudo ./scripts/package.sh
		```
		This script will:
		•	Copy `.aaxplugin` files and required resources to a temporary directory.
		•	Set permissions correctly.
		•	Use `pkgbuild` and `productbuild` to generate the final installer.
		•	Perform ad-hoc or release signing as needed

	The installer is output to the scripts folder, and should be named Eclipsa_BranchName.pkg
	
### Generating Signed AAX Plugins

In order to use the AAX plugin in a non developer version of the ProTools DAW, the plugin must be signed with a certificate.

Signing the binaries is beyond the scope of this document, however Avid provides documentation for understanding this process and it's requirements.

During installer creation, if you wish to perform the necessary signing steps, create a script called "wraptool_helper.sh" and add the script to the /scripts folder. This script must then call the signing tool, pointing it at the AAX plugins. Note that this tool must perform both the AAX and Apple developer signing steps.