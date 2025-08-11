// DVPCamera64 Stub Library for Linking
// This stub library allows compilation when DVPCamera64.lib is not available
// The actual implementation will be loaded from DVPCamera64.dll at runtime

// IMPORTANT: We need to prevent the header from declaring functions as dllimport
// We do this by defining a special macro that the header should check for
#define DVP_BUILDING_STUB

// Define dvp2api before including the header
// The header will override this, so we need to undef and redefine after
#ifdef _WIN32
    #define DVP_STUB_EXPORT __declspec(dllexport)
#else
    #define DVP_STUB_EXPORT __attribute__((visibility("default")))
#endif

// Include the header with its original definitions
#include "DVPCamera.h"

// Now undefine and redefine dvp2api for our stub implementations
#undef dvp2api
#ifdef _WIN32
    #ifdef __cplusplus
        #define dvp2api extern "C" DVP_STUB_EXPORT
    #else
        #define dvp2api DVP_STUB_EXPORT
    #endif
#else
    #ifdef __cplusplus
        #define dvp2api extern "C"
    #else
        #define dvp2api extern
    #endif
#endif

// For the version array - redefine it with export
#undef dvpVersion
dvp2api const unsigned int dvpVersion[4] = {2, 25, 43, 1332};

// Define stub functions that match the dvp2api calling convention
#define STUB_FUNCTION(ret, name, ...) \
    dvp2api ret name(__VA_ARGS__) { \
        return (ret)0; \
    }

// Initialize functions
STUB_FUNCTION(dvpStatus, dvpInit)
STUB_FUNCTION(dvpStatus, dvpRefresh, dvpUint32*)
STUB_FUNCTION(dvpStatus, dvpEnum, dvpUint32, dvpCameraInfo*)
STUB_FUNCTION(dvpStatus, dvpOpenByName, const char*, dvpOpenMode, dvpHandle*)
STUB_FUNCTION(dvpStatus, dvpClose, dvpHandle)
STUB_FUNCTION(dvpStatus, dvpIsValid, dvpHandle, bool*)
STUB_FUNCTION(dvpStatus, dvpStart, dvpHandle)
STUB_FUNCTION(dvpStatus, dvpStop, dvpHandle)
STUB_FUNCTION(dvpStatus, dvpGetStreamState, dvpHandle, dvpStreamState*)

// Camera info functions
STUB_FUNCTION(dvpStatus, dvpGetCameraInfo, dvpHandle, dvpCameraInfo*)
STUB_FUNCTION(dvpStatus, dvpGetSensorInfo, dvpHandle, dvpSensorInfo*)
STUB_FUNCTION(dvpStatus, dvpGetTemperatureInfo, dvpHandle, dvpTemperatureInfo*)
STUB_FUNCTION(dvpStatus, dvpGetFrameCount, dvpHandle, dvpFrameCount*)

// Frame functions
STUB_FUNCTION(dvpStatus, dvpGetFrame, dvpHandle, dvpFrame*, void**, dvpUint32)
STUB_FUNCTION(dvpStatus, dvpRegisterStreamCallback, dvpHandle, dvpStreamCallback, dvpStreamEvent, void*)
STUB_FUNCTION(dvpStatus, dvpUnregisterStreamCallback, dvpHandle, dvpStreamCallback, dvpStreamEvent, void*)

// Exposure and gain
STUB_FUNCTION(dvpStatus, dvpGetExposure, dvpHandle, double*)
STUB_FUNCTION(dvpStatus, dvpSetExposure, dvpHandle, double)
STUB_FUNCTION(dvpStatus, dvpGetExposureDescr, dvpHandle, dvpDoubleDescr*)
STUB_FUNCTION(dvpStatus, dvpGetAnalogGain, dvpHandle, float*)
STUB_FUNCTION(dvpStatus, dvpSetAnalogGain, dvpHandle, float)
STUB_FUNCTION(dvpStatus, dvpGetAnalogGainDescr, dvpHandle, dvpFloatDescr*)

// ROI functions
STUB_FUNCTION(dvpStatus, dvpGetRoi, dvpHandle, dvpRegion*)
STUB_FUNCTION(dvpStatus, dvpSetRoi, dvpHandle, dvpRegion)
STUB_FUNCTION(dvpStatus, dvpGetRoiDescr, dvpHandle, dvpRegionDescr*)

// Format functions
STUB_FUNCTION(dvpStatus, dvpGetTargetFormat, dvpHandle, dvpStreamFormat*)
STUB_FUNCTION(dvpStatus, dvpSetTargetFormat, dvpHandle, dvpStreamFormat)

// Trigger functions
STUB_FUNCTION(dvpStatus, dvpTriggerFire, dvpHandle)
STUB_FUNCTION(dvpStatus, dvpGetTriggerSource, dvpHandle, dvpTriggerSource*)
STUB_FUNCTION(dvpStatus, dvpSetTriggerSource, dvpHandle, dvpTriggerSource)
STUB_FUNCTION(dvpStatus, dvpGetTriggerState, dvpHandle, bool*)
STUB_FUNCTION(dvpStatus, dvpSetTriggerState, dvpHandle, bool)

// Color functions
STUB_FUNCTION(dvpStatus, dvpGetColorCorrection, dvpHandle, dvpColorCorrection*)
STUB_FUNCTION(dvpStatus, dvpSetColorCorrection, dvpHandle, dvpColorCorrection)
STUB_FUNCTION(dvpStatus, dvpGetRgbGain, dvpHandle, float*, float*, float*)
STUB_FUNCTION(dvpStatus, dvpSetRgbGain, dvpHandle, float, float, float)
STUB_FUNCTION(dvpStatus, dvpGetColorSolutionSel, dvpHandle, dvpUint32*)

// Gamma functions
STUB_FUNCTION(dvpStatus, dvpGetGamma, dvpHandle, dvpInt32*)
STUB_FUNCTION(dvpStatus, dvpSetGamma, dvpHandle, dvpInt32)

// Config functions
STUB_FUNCTION(dvpStatus, dvpSaveConfig, dvpHandle, const char*)
STUB_FUNCTION(dvpStatus, dvpLoadConfig, dvpHandle, const char*)

// User data functions
STUB_FUNCTION(dvpStatus, dvpReadUserData, dvpHandle, dvpUint32, void*, dvpUint32)
STUB_FUNCTION(dvpStatus, dvpWriteUserData, dvpHandle, dvpUint32, const void*, dvpUint32)

// Resolution functions
STUB_FUNCTION(dvpStatus, dvpGetResolutionModeSel, dvpHandle, dvpUint32*)
STUB_FUNCTION(dvpStatus, dvpSetResolutionModeSel, dvpHandle, dvpUint32)
STUB_FUNCTION(dvpStatus, dvpGetResolutionModeSelDescr, dvpHandle, dvpSelectionDescr*)
STUB_FUNCTION(dvpStatus, dvpGetResolutionModeSelDetail, dvpHandle, dvpUint32, dvpResolutionMode*)

// AE mode functions
STUB_FUNCTION(dvpStatus, dvpGetAeMode, dvpHandle, dvpAeMode*)
STUB_FUNCTION(dvpStatus, dvpSetAeMode, dvpHandle, dvpAeMode)

// Frame rate functions
STUB_FUNCTION(dvpStatus, dvpGetFrameRate, dvpHandle, double*)
STUB_FUNCTION(dvpStatus, dvpSetFrameRate, dvpHandle, double)

// Output functions
STUB_FUNCTION(dvpStatus, dvpGetOutputIoFunction, dvpHandle, dvpOutputIo, dvpOutputIoFunction*)
STUB_FUNCTION(dvpStatus, dvpSetOutputIoFunction, dvpHandle, dvpOutputIo, dvpOutputIoFunction)
STUB_FUNCTION(dvpStatus, dvpGetOutputIoLevel, dvpHandle, dvpOutputIo, bool*)
STUB_FUNCTION(dvpStatus, dvpSetOutputIoLevel, dvpHandle, dvpOutputIo, bool)
STUB_FUNCTION(dvpStatus, dvpGetInputIoLevel, dvpHandle, dvpInputIo, bool*)

// Additional functions needed for Do3ThinkCameraComponent
STUB_FUNCTION(dvpStatus, dvpSetColorSolutionSel, dvpHandle, dvpUint32)
STUB_FUNCTION(dvpStatus, dvpGetColorSolutionSelDescr, dvpHandle, dvpSelectionDescr*)
STUB_FUNCTION(dvpStatus, dvpGetHardwareIspState, dvpHandle, bool*)
STUB_FUNCTION(dvpStatus, dvpSetHardwareIspState, dvpHandle, bool)
STUB_FUNCTION(dvpStatus, dvpGetAeOperation, dvpHandle, dvpAeOperation*)
STUB_FUNCTION(dvpStatus, dvpGetAwbOperation, dvpHandle, dvpAwbOperation*)
STUB_FUNCTION(dvpStatus, dvpSetTriggerInputType, dvpHandle, dvpTriggerInputType)
STUB_FUNCTION(dvpStatus, dvpSetTriggerDelay, dvpHandle, double)
STUB_FUNCTION(dvpStatus, dvpSetPacketSizeSel, dvpHandle, dvpUint32)