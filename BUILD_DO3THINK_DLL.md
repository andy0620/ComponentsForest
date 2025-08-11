# Building Do3ThinkCameraComponent.dll

## Overview
The Do3ThinkCameraComponent.dll is a Qt6-based camera component that interfaces with Do3Think industrial cameras using their DVPCamera SDK.

## Prerequisites
1. **Visual Studio 2022** (Community, Professional, or Enterprise)
2. **Qt 6.9.1** installed at `C:\Qt\6.9.1\msvc2022_64`
3. **CMake 3.27+** for generating project files

## Build Instructions

### Method 1: Using PowerShell Script (Recommended)
```powershell
# Open PowerShell as Administrator
cd C:\Users\g4user\Desktop\ComponentsForest

# Run the build script
.\build_do3think_dll.ps1
```

### Method 2: Using Visual Studio
1. Open `build\ComponentsForest.sln` in Visual Studio 2022
2. Set configuration to **Release** and platform to **x64**
3. Build the **DVPCamera_stub** project first (Right-click → Build)
4. Build the **Do3ThinkCameraComponent** project (Right-click → Build)

### Method 3: Using Command Line with MSBuild
```cmd
# Open Developer Command Prompt for VS 2022
cd C:\Users\g4user\Desktop\ComponentsForest\build

# Build DVPCamera stub library
msbuild DVPCamera_stub.vcxproj /p:Configuration=Release /p:Platform=x64

# Build Do3ThinkCameraComponent
msbuild Do3ThinkCameraComponent.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Output Files
After successful build, you'll find:
- **Do3ThinkCameraComponent.dll** - The main camera component library
- **Do3ThinkCameraComponent.lib** - Import library for linking
- **DVPCamera_stub.lib** - Stub library for DVPCamera SDK

Location: `build\Release\`

## Troubleshooting

### Missing DVPCamera64.lib
The project includes a stub library (`DVPCamera_stub`) that provides the necessary symbols for compilation. The actual DVPCamera64.dll from Do3Think SDK will be loaded at runtime.

### Qt Not Found
Ensure Qt 6.9.1 is installed at `C:\Qt\6.9.1\msvc2022_64`. If installed elsewhere, update the Qt6_DIR in CMake:
```cmd
cmake .. -DQt6_DIR="C:\path\to\Qt\6.9.1\msvc2022_64\lib\cmake\Qt6"
```

### Build Errors
1. Clean the build directory: Delete all files in `build\` except the .sln file
2. Re-run CMake: `cmake ..`
3. Try building again

## Using the DLL
To use Do3ThinkCameraComponent.dll in your application:

1. Copy these files to your application directory:
   - Do3ThinkCameraComponent.dll
   - Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll (from Qt installation)
   - DVPCamera64.dll (from Do3Think SDK, when available)

2. Link against Do3ThinkCameraComponent.lib in your project

3. Include the headers:
   ```cpp
   #include "Do3ThinkCamera/dothink_camera.h"
   ```

## SDK Integration
The component expects the Do3Think DVPCamera64.dll to be present at runtime. Place it in:
- The same directory as your executable, OR
- A directory in the system PATH

## Status
✅ **DVPCamera_stub library created** - Provides linking symbols
✅ **CMake configuration complete** - Visual Studio projects generated
✅ **Build scripts created** - PowerShell and batch files available
⏳ **Ready to build** - Run the PowerShell script to compile the DLL