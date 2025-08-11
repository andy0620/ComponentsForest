# DVP Camera Wrapper

## Overview
This wrapper provides a unified interface to the Do3Think DVP Camera SDK that solves the dllimport/dllexport problem on Windows and supports both stub mode and dynamic loading.

## Problem Solved
The original DVPCamera.h header forces `__declspec(dllimport)` on Windows, which causes compilation issues when:
- Building a stub library for testing without the SDK
- Using dynamic loading instead of static linking
- Cross-compiling or building on systems without the SDK installed

## Solution Architecture

### Files Created
- `dvp_wrapper.h` - Complete type definitions and function declarations
- `dvp_wrapper.cpp` - Implementation supporting both stub and dynamic loading modes
- `test_wrapper.cpp` - Test program to verify wrapper functionality

### Key Features

1. **Complete Type Definitions**
   - All DVP types defined directly (no dependency on DVPCamera.h)
   - Enums: dvpStatus, dvpImageFormat, dvpStreamFormat, etc.
   - Structures: dvpCameraInfo, dvpFrame, dvpRegion, etc.
   - Callbacks: dvpStreamCallback, dvpEventCallback

2. **Dual Mode Support**
   - **Stub Mode** (default): Returns safe dummy values for testing
   - **Dynamic Loading Mode** (with USE_DYNAMIC_LOADING): Loads DVPCamera64.dll at runtime

3. **Platform Independence**
   - Works on Windows (LoadLibrary/GetProcAddress)
   - Works on Linux (dlopen/dlsym)
   - No dllimport/dllexport issues

## Usage

### In CMakeLists.txt
```cmake
add_library(Do3ThinkCameraComponent SHARED
    Do3ThinkCamera/dothink_camera.cpp
    Do3ThinkCamera/dvp_wrapper.cpp  # Add this
    # ... other files
)

if(USE_DYNAMIC_LOADING)
    target_compile_definitions(Do3ThinkCameraComponent PRIVATE USE_DYNAMIC_LOADING)
endif()
```

### In Your Code
```cpp
// Simply include the wrapper instead of DVPCamera.h
#include "dvp_wrapper.h"

// Use DVP functions normally
dvpStatus status = dvpInit();
dvpHandle handle;
status = dvpOpenByName("Camera1", OPEN_NORMAL, &handle);
```

## Build Modes

### 1. Stub Mode (for development/testing)
```bash
g++ -c dvp_wrapper.cpp
# Returns safe dummy values, no SDK required
```

### 2. Dynamic Loading Mode (for production)
```bash
g++ -DUSE_DYNAMIC_LOADING -c dvp_wrapper.cpp
# Loads DVPCamera64.dll at runtime
```

### 3. Static Linking (if you have the SDK .lib file)
```bash
# Not using the wrapper, link directly to DVPCamera64.lib
# But this requires the problematic DVPCamera.h
```

## Testing

Run the test program to verify the wrapper:
```bash
# Compile test
g++ test_wrapper.cpp dvp_wrapper.cpp -o test_wrapper

# Run test
./test_wrapper
```

Expected output in stub mode:
```
Testing DVP Wrapper...
dvpInit() returned: 0 (DVP_STATUS_OK = 0)
dvpRefresh() returned: 0, device count: 0
...
Wrapper mode: STUB IMPLEMENTATION
Test completed successfully!
```

## Benefits

1. **No SDK Required for Development** - Can build and test without installing Do3Think SDK
2. **Clean Compilation** - No dllimport/dllexport conflicts
3. **Runtime Flexibility** - Can ship without DVPCamera64.dll and load it if available
4. **Cross-Platform** - Same code works on Windows and Linux
5. **Maintainable** - All DVP types in one place, easy to update

## Implementation Notes

### Dynamic Loading Search Paths (Windows)
```c
const char* libraryNames[] = {
    "DVPCamera64.dll",
    "DVPCamera.dll",
    nullptr
};
```

### Dynamic Loading Search Paths (Linux)
```c
const char* libraryNames[] = {
    "libDVPCamera.so",
    "./libDVPCamera.so",
    "/usr/lib/libDVPCamera.so",
    "/usr/local/lib/libDVPCamera.so",
    nullptr
};
```

### Function Pointer Loading
All DVP functions are loaded dynamically:
```c
pfn_dvpInit = (decltype(pfn_dvpInit))GET_FUNCTION(g_dvpLibrary, "dvpInit");
```

## Migration from Old Code

### Before (problematic)
```cpp
#ifdef USE_DYNAMIC_LOADING
#include "dvp_minimal_loader.h"
#else
#include "SDK/DVPCamera_wrapper.h"  // Has dllimport issues
#endif
```

### After (clean)
```cpp
#include "dvp_wrapper.h"  // Works in all modes
```

## Future Enhancements

1. Add more comprehensive error messages in dynamic loading mode
2. Support for additional DVP functions as needed
3. Optional logging of SDK function calls for debugging
4. Automatic SDK path detection from registry (Windows)