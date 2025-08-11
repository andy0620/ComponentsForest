// DVPCamera64 Stub Library for Linking
// This stub library allows compilation when DVPCamera64.lib is not available
// The actual implementation will be loaded from DVPCamera64.dll at runtime

#include <cstdint>

// Define basic types
typedef void* dvpHandle;
typedef int32_t dvpStatus;
typedef uint32_t dvpUint32;
typedef int32_t dvpInt32;
typedef float dvpFloat32;

// Define enums as int32_t
typedef int32_t dvpOpenMode;
typedef int32_t dvpStreamState;
typedef int32_t dvpStreamFormat;
typedef int32_t dvpTriggerSource;
typedef int32_t dvpColorCorrection;
typedef int32_t dvpAeMode;
typedef int32_t dvpOutputIo;
typedef int32_t dvpOutputIoFunction;
typedef int32_t dvpStreamEvent;

// Define struct pointers as void*
typedef void* dvpCameraInfo;
typedef void* dvpSensorInfo;
typedef void* dvpTemperatureInfo;
typedef void* dvpFrameCount;
typedef void* dvpFrame;
typedef void* dvpDoubleDescr;
typedef void* dvpFloatDescr;
typedef void* dvpRegion;
typedef void* dvpRegionDescr;
typedef void* dvpSelectionDescr;
typedef void* dvpResolutionMode;

// Define callback type
typedef int32_t (*dvpStreamCallback)(dvpHandle handle, dvpStreamEvent event, void* pContext, dvpFrame* pFrame, void* pBuffer);

#ifdef _WIN32
    #define EXPORT_FUNC __declspec(dllexport)
#else
    #define EXPORT_FUNC __attribute__((visibility("default")))
#endif

#define STUB_FUNCTION(ret, name, ...) \
    extern "C" EXPORT_FUNC ret name(__VA_ARGS__) { \
        return (ret)0; \
    }

// Initialize functions
STUB_FUNCTION(dvpStatus, dvpInit)
STUB_FUNCTION(dvpStatus, dvpRefresh, dvpUint32*)
STUB_FUNCTION(dvpStatus, dvpEnum, dvpUint32, dvpCameraInfo)
STUB_FUNCTION(dvpStatus, dvpOpenByName, const char*, dvpOpenMode, dvpHandle*)
STUB_FUNCTION(dvpStatus, dvpClose, dvpHandle)
STUB_FUNCTION(dvpStatus, dvpIsValid, dvpHandle, bool*)
STUB_FUNCTION(dvpStatus, dvpStart, dvpHandle)
STUB_FUNCTION(dvpStatus, dvpStop, dvpHandle)
STUB_FUNCTION(dvpStatus, dvpGetStreamState, dvpHandle, dvpStreamState*)

// Camera info functions
STUB_FUNCTION(dvpStatus, dvpGetCameraInfo, dvpHandle, dvpCameraInfo)
STUB_FUNCTION(dvpStatus, dvpGetSensorInfo, dvpHandle, dvpSensorInfo)
STUB_FUNCTION(dvpStatus, dvpGetTemperatureInfo, dvpHandle, dvpTemperatureInfo)
STUB_FUNCTION(dvpStatus, dvpGetFrameCount, dvpHandle, dvpFrameCount)

// Frame functions
STUB_FUNCTION(dvpStatus, dvpGetFrame, dvpHandle, dvpFrame, void**, dvpUint32)
STUB_FUNCTION(dvpStatus, dvpRegisterStreamCallback, dvpHandle, dvpStreamCallback, dvpStreamEvent, void*)
STUB_FUNCTION(dvpStatus, dvpUnregisterStreamCallback, dvpHandle, dvpStreamCallback, dvpStreamEvent, void*)

// Exposure and gain
STUB_FUNCTION(dvpStatus, dvpGetExposure, dvpHandle, double*)
STUB_FUNCTION(dvpStatus, dvpSetExposure, dvpHandle, double)
STUB_FUNCTION(dvpStatus, dvpGetExposureDescr, dvpHandle, dvpDoubleDescr)
STUB_FUNCTION(dvpStatus, dvpGetAnalogGain, dvpHandle, float*)
STUB_FUNCTION(dvpStatus, dvpSetAnalogGain, dvpHandle, float)
STUB_FUNCTION(dvpStatus, dvpGetAnalogGainDescr, dvpHandle, dvpFloatDescr)

// ROI functions
STUB_FUNCTION(dvpStatus, dvpGetRoi, dvpHandle, dvpRegion)
STUB_FUNCTION(dvpStatus, dvpSetRoi, dvpHandle, dvpRegion)
STUB_FUNCTION(dvpStatus, dvpGetRoiDescr, dvpHandle, dvpRegionDescr)

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
STUB_FUNCTION(dvpStatus, dvpGetResolutionModeSelDescr, dvpHandle, dvpSelectionDescr)
STUB_FUNCTION(dvpStatus, dvpGetResolutionModeSelDetail, dvpHandle, dvpUint32, dvpResolutionMode)

// AE mode functions
STUB_FUNCTION(dvpStatus, dvpGetAeMode, dvpHandle, dvpAeMode*)
STUB_FUNCTION(dvpStatus, dvpSetAeMode, dvpHandle, dvpAeMode)

// Frame rate functions
STUB_FUNCTION(dvpStatus, dvpGetFrameRate, dvpHandle, double*)
STUB_FUNCTION(dvpStatus, dvpSetFrameRate, dvpHandle, double)

// Output functions
STUB_FUNCTION(dvpStatus, dvpGetOutputIoFunction, dvpHandle, dvpOutputIo, dvpOutputIoFunction*)
STUB_FUNCTION(dvpStatus, dvpSetOutputIoFunction, dvpHandle, dvpOutputIo, dvpOutputIoFunction)