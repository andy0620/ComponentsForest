#ifndef DVPCAMERA_WRAPPER_H
#define DVPCAMERA_WRAPPER_H

// This wrapper header ensures that DVP functions are not imported when using the stub library

#ifdef DVP_USE_STUB_LIBRARY
    // When using the stub library, we need to trick the SDK header
    // We temporarily undefine _WIN32 to make it use the non-Windows path
    // which doesn't use dllimport
    #ifdef _WIN32
        #define _WIN32_WAS_DEFINED
        #undef _WIN32
    #endif
#endif

// Now include the actual SDK header
#include "DVPCamera.h"

#ifdef DVP_USE_STUB_LIBRARY
    // Restore _WIN32 if it was defined
    #ifdef _WIN32_WAS_DEFINED
        #define _WIN32
        #undef _WIN32_WAS_DEFINED
    #endif
#endif

#endif // DVPCAMERA_WRAPPER_H