/**
 * @file dvp_wrapper.cpp
 * @brief DVP Camera API Wrapper Implementation
 * 
 * This file provides the implementation for DVP functions.
 * It supports both stub mode (returns dummy values) and dynamic loading mode.
 */

#include "dvp_wrapper.h"
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

// Simple debug output that doesn't rely on external functions
#define DEBUG_LOG(msg) do { \
    std::cerr << "[DVP_WRAPPER] " << msg << std::endl; \
    std::cerr.flush(); \
} while(0)

#ifdef USE_DYNAMIC_LOADING

// ============================================================================
// Dynamic Loading Implementation
// ============================================================================

#ifdef _WIN32
    #include <windows.h>
    typedef HMODULE LibraryHandle;
    #define LOAD_LIBRARY(name) LoadLibraryA(name)
    #define GET_FUNCTION(lib, name) GetProcAddress(lib, name)
    #define FREE_LIBRARY(lib) FreeLibrary(lib)
#else
    #include <dlfcn.h>
    typedef void* LibraryHandle;
    #define LOAD_LIBRARY(name) dlopen(name, RTLD_LAZY)
    #define GET_FUNCTION(lib, name) dlsym(lib, name)
    #define FREE_LIBRARY(lib) dlclose(lib)
#endif

// Global library handle
static LibraryHandle g_dvpLibrary = nullptr;

// Function pointers
static dvpStatus (*pfn_dvpInit)() = nullptr;
static dvpStatus (*pfn_dvpExit)() = nullptr;
static dvpStatus (*pfn_dvpRefresh)(dvpUint32*) = nullptr;
static dvpStatus (*pfn_dvpEnum)(dvpUint32, dvpCameraInfo*) = nullptr;
static dvpStatus (*pfn_dvpOpen)(dvpUint32, dvpOpenMode, dvpHandle*) = nullptr;
static dvpStatus (*pfn_dvpOpenByName)(const char*, dvpOpenMode, dvpHandle*) = nullptr;
static dvpStatus (*pfn_dvpClose)(dvpHandle) = nullptr;
static dvpStatus (*pfn_dvpIsValid)(dvpHandle, bool*) = nullptr;
static dvpStatus (*pfn_dvpStart)(dvpHandle) = nullptr;
static dvpStatus (*pfn_dvpStop)(dvpHandle) = nullptr;
static dvpStatus (*pfn_dvpGetStreamState)(dvpHandle, dvpStreamState*) = nullptr;
static dvpStatus (*pfn_dvpGetFrame)(dvpHandle, dvpFrame*, void**, dvpUint32) = nullptr;
static dvpStatus (*pfn_dvpGetFrameBuffer)(dvpHandle, dvpFrameBuffer*, dvpUint32) = nullptr;
static dvpStatus (*pfn_dvpRegisterStreamCallback)(dvpHandle, dvpStreamCallback, dvpStreamEvent, void*) = nullptr;
static dvpStatus (*pfn_dvpUnregisterStreamCallback)(dvpHandle, dvpStreamCallback, dvpStreamEvent, void*) = nullptr;
static dvpStatus (*pfn_dvpGetCameraInfo)(dvpHandle, dvpCameraInfo*) = nullptr;
static dvpStatus (*pfn_dvpGetSensorInfo)(dvpHandle, dvpSensorInfo*) = nullptr;
static dvpStatus (*pfn_dvpGetExposure)(dvpHandle, dvpFloat64*) = nullptr;
static dvpStatus (*pfn_dvpSetExposure)(dvpHandle, dvpFloat64) = nullptr;
static dvpStatus (*pfn_dvpGetExposureDescr)(dvpHandle, dvpDoubleDescr*) = nullptr;
static dvpStatus (*pfn_dvpGetAnalogGain)(dvpHandle, float*) = nullptr;
static dvpStatus (*pfn_dvpSetAnalogGain)(dvpHandle, float) = nullptr;
static dvpStatus (*pfn_dvpGetAnalogGainDescr)(dvpHandle, dvpFloatDescr*) = nullptr;
static dvpStatus (*pfn_dvpGetRoi)(dvpHandle, dvpRegion*) = nullptr;
static dvpStatus (*pfn_dvpSetRoi)(dvpHandle, dvpRegion) = nullptr;
static dvpStatus (*pfn_dvpGetRoiDescr)(dvpHandle, dvpRegionDescr*) = nullptr;
static dvpStatus (*pfn_dvpGetAeMode)(dvpHandle, dvpAeMode*) = nullptr;
static dvpStatus (*pfn_dvpSetAeMode)(dvpHandle, dvpAeMode) = nullptr;
static dvpStatus (*pfn_dvpGetAeOperation)(dvpHandle, dvpAeOperation*) = nullptr;
static dvpStatus (*pfn_dvpSetAeOperation)(dvpHandle, dvpAeOperation) = nullptr;
static dvpStatus (*pfn_dvpGetAwbOperation)(dvpHandle, dvpAwbOperation*) = nullptr;
static dvpStatus (*pfn_dvpSetAwbOperation)(dvpHandle, dvpAwbOperation) = nullptr;
static dvpStatus (*pfn_dvpGetRgbGain)(dvpHandle, float*, float*, float*) = nullptr;
static dvpStatus (*pfn_dvpSetRgbGain)(dvpHandle, float, float, float) = nullptr;
static dvpStatus (*pfn_dvpGetGamma)(dvpHandle, dvpInt32*) = nullptr;
static dvpStatus (*pfn_dvpSetGamma)(dvpHandle, dvpInt32) = nullptr;
static dvpStatus (*pfn_dvpGetTargetFormat)(dvpHandle, dvpStreamFormat*) = nullptr;
static dvpStatus (*pfn_dvpSetTargetFormat)(dvpHandle, dvpStreamFormat) = nullptr;
static dvpStatus (*pfn_dvpGetResolutionModeSel)(dvpHandle, dvpUint32*) = nullptr;
static dvpStatus (*pfn_dvpSetResolutionModeSel)(dvpHandle, dvpUint32) = nullptr;
static dvpStatus (*pfn_dvpGetResolutionModeSelDescr)(dvpHandle, dvpSelectionDescr*) = nullptr;
static dvpStatus (*pfn_dvpGetResolutionModeSelDetail)(dvpHandle, dvpUint32, dvpResolutionMode*) = nullptr;
static dvpStatus (*pfn_dvpGetTriggerSource)(dvpHandle, dvpTriggerSource*) = nullptr;
static dvpStatus (*pfn_dvpSetTriggerSource)(dvpHandle, dvpTriggerSource) = nullptr;
static dvpStatus (*pfn_dvpGetTriggerInputType)(dvpHandle, dvpLine, dvpTriggerInputType*) = nullptr;
static dvpStatus (*pfn_dvpSetTriggerInputType)(dvpHandle, dvpTriggerInputType) = nullptr;
static dvpStatus (*pfn_dvpTriggerFire)(dvpHandle) = nullptr;
static dvpStatus (*pfn_dvpSetTriggerDelay)(dvpHandle, dvpFloat64) = nullptr;
static dvpStatus (*pfn_dvpGetBufferQueueSize)(dvpHandle, dvpUint32*) = nullptr;
static dvpStatus (*pfn_dvpSetBufferQueueSize)(dvpHandle, dvpUint32) = nullptr;
static dvpStatus (*pfn_dvpGetFrameCount)(dvpHandle, dvpFrameCount*) = nullptr;
static dvpStatus (*pfn_dvpGetTemperatureInfo)(dvpHandle, dvpTemperatureInfo*) = nullptr;
static dvpStatus (*pfn_dvpGetColorCorrection)(dvpHandle, dvpColorCorrection*) = nullptr;
static dvpStatus (*pfn_dvpSetColorCorrection)(dvpHandle, dvpColorCorrection) = nullptr;
static dvpStatus (*pfn_dvpGetColorSolutionSel)(dvpHandle, dvpUint32*) = nullptr;
static dvpStatus (*pfn_dvpSetColorSolutionSel)(dvpHandle, dvpUint32) = nullptr;
static dvpStatus (*pfn_dvpGetColorSolutionSelDescr)(dvpHandle, dvpSelectionDescr*) = nullptr;
static dvpStatus (*pfn_dvpGetHardwareIspState)(dvpHandle, bool*) = nullptr;
static dvpStatus (*pfn_dvpSetHardwareIspState)(dvpHandle, bool) = nullptr;
static dvpStatus (*pfn_dvpSaveConfig)(dvpHandle, const char*) = nullptr;
static dvpStatus (*pfn_dvpLoadConfig)(dvpHandle, const char*) = nullptr;
static dvpStatus (*pfn_dvpReadUserData)(dvpHandle, dvpUint32, void*, dvpUint32) = nullptr;
static dvpStatus (*pfn_dvpWriteUserData)(dvpHandle, dvpUint32, const void*, dvpUint32) = nullptr;
static dvpStatus (*pfn_dvpGetOutputIoLevel)(dvpHandle, dvpOutputIo, bool*) = nullptr;
static dvpStatus (*pfn_dvpSetOutputIoLevel)(dvpHandle, dvpOutputIo, bool) = nullptr;
static dvpStatus (*pfn_dvpGetInputIoLevel)(dvpHandle, dvpInputIo, bool*) = nullptr;
static dvpStatus (*pfn_dvpSetOutputIoFunction)(dvpHandle, dvpOutputIo, dvpOutputIoFunction) = nullptr;
static dvpStatus (*pfn_dvpGetPacketSizeSel)(dvpHandle, dvpUint32*) = nullptr;
static dvpStatus (*pfn_dvpSetPacketSizeSel)(dvpHandle, dvpUint32) = nullptr;

// Load DVP library dynamically
static bool LoadDVPLibrary() {
    DEBUG_LOG("LoadDVPLibrary() entered");
    
    if (g_dvpLibrary) {
        DEBUG_LOG("DVP library already loaded");
        return true;
    }
    
#ifdef _WIN32
    const char* libraryNames[] = {
        "DVPCamera64.dll",
        "DVPCamera.dll",
        nullptr
    };
    DEBUG_LOG("Windows platform - looking for DVPCamera64.dll or DVPCamera.dll");
#else
    const char* libraryNames[] = {
        "libDVPCamera.so",
        "./libDVPCamera.so",
        "/usr/lib/libDVPCamera.so",
        "/usr/local/lib/libDVPCamera.so",
        nullptr
    };
    DEBUG_LOG("Linux platform - looking for libDVPCamera.so");
#endif
    
    for (int i = 0; libraryNames[i] != nullptr; ++i) {
        DEBUG_LOG(std::string("Attempting to load: ") + libraryNames[i]);
        g_dvpLibrary = LOAD_LIBRARY(libraryNames[i]);
        if (g_dvpLibrary) {
            DEBUG_LOG(std::string("SUCCESS: Loaded ") + libraryNames[i]);
            break;
        } else {
#ifdef _WIN32
            DWORD error = GetLastError();
            std::stringstream ss;
            ss << "FAILED to load " << libraryNames[i] << " - Error code: " << error;
            DEBUG_LOG(ss.str());
#else
            DEBUG_LOG(std::string("FAILED to load ") + libraryNames[i]);
#endif
        }
    }
    
    if (!g_dvpLibrary) {
        DEBUG_LOG("ERROR: Failed to load any DVP library");
        return false;
    }
    
    // Load function pointers
    #define LOAD_FUNC(name) pfn_##name = (decltype(pfn_##name))GET_FUNCTION(g_dvpLibrary, #name)
    
    LOAD_FUNC(dvpInit);
    LOAD_FUNC(dvpExit);
    LOAD_FUNC(dvpRefresh);
    LOAD_FUNC(dvpEnum);
    LOAD_FUNC(dvpOpen);
    LOAD_FUNC(dvpOpenByName);
    LOAD_FUNC(dvpClose);
    LOAD_FUNC(dvpIsValid);
    LOAD_FUNC(dvpStart);
    LOAD_FUNC(dvpStop);
    LOAD_FUNC(dvpGetStreamState);
    LOAD_FUNC(dvpGetFrame);
    LOAD_FUNC(dvpGetFrameBuffer);
    LOAD_FUNC(dvpRegisterStreamCallback);
    LOAD_FUNC(dvpUnregisterStreamCallback);
    LOAD_FUNC(dvpGetCameraInfo);
    LOAD_FUNC(dvpGetSensorInfo);
    LOAD_FUNC(dvpGetExposure);
    LOAD_FUNC(dvpSetExposure);
    LOAD_FUNC(dvpGetExposureDescr);
    LOAD_FUNC(dvpGetAnalogGain);
    LOAD_FUNC(dvpSetAnalogGain);
    LOAD_FUNC(dvpGetAnalogGainDescr);
    LOAD_FUNC(dvpGetRoi);
    LOAD_FUNC(dvpSetRoi);
    LOAD_FUNC(dvpGetRoiDescr);
    LOAD_FUNC(dvpGetAeMode);
    LOAD_FUNC(dvpSetAeMode);
    LOAD_FUNC(dvpGetAeOperation);
    LOAD_FUNC(dvpSetAeOperation);
    LOAD_FUNC(dvpGetAwbOperation);
    LOAD_FUNC(dvpSetAwbOperation);
    LOAD_FUNC(dvpGetRgbGain);
    LOAD_FUNC(dvpSetRgbGain);
    LOAD_FUNC(dvpGetGamma);
    LOAD_FUNC(dvpSetGamma);
    LOAD_FUNC(dvpGetTargetFormat);
    LOAD_FUNC(dvpSetTargetFormat);
    LOAD_FUNC(dvpGetResolutionModeSel);
    LOAD_FUNC(dvpSetResolutionModeSel);
    LOAD_FUNC(dvpGetResolutionModeSelDescr);
    LOAD_FUNC(dvpGetResolutionModeSelDetail);
    LOAD_FUNC(dvpGetTriggerSource);
    LOAD_FUNC(dvpSetTriggerSource);
    LOAD_FUNC(dvpGetTriggerInputType);
    LOAD_FUNC(dvpSetTriggerInputType);
    LOAD_FUNC(dvpTriggerFire);
    LOAD_FUNC(dvpSetTriggerDelay);
    LOAD_FUNC(dvpGetBufferQueueSize);
    LOAD_FUNC(dvpSetBufferQueueSize);
    LOAD_FUNC(dvpGetFrameCount);
    LOAD_FUNC(dvpGetTemperatureInfo);
    LOAD_FUNC(dvpGetColorCorrection);
    LOAD_FUNC(dvpSetColorCorrection);
    LOAD_FUNC(dvpGetColorSolutionSel);
    LOAD_FUNC(dvpSetColorSolutionSel);
    LOAD_FUNC(dvpGetColorSolutionSelDescr);
    LOAD_FUNC(dvpGetHardwareIspState);
    LOAD_FUNC(dvpSetHardwareIspState);
    LOAD_FUNC(dvpSaveConfig);
    LOAD_FUNC(dvpLoadConfig);
    LOAD_FUNC(dvpReadUserData);
    LOAD_FUNC(dvpWriteUserData);
    LOAD_FUNC(dvpGetOutputIoLevel);
    LOAD_FUNC(dvpSetOutputIoLevel);
    LOAD_FUNC(dvpGetInputIoLevel);
    LOAD_FUNC(dvpSetOutputIoFunction);
    LOAD_FUNC(dvpGetPacketSizeSel);
    LOAD_FUNC(dvpSetPacketSizeSel);
    
    #undef LOAD_FUNC
    
    // Check critical functions
    DEBUG_LOG("Checking critical function pointers");
    if (!pfn_dvpInit) DEBUG_LOG("ERROR: dvpInit function not loaded");
    if (!pfn_dvpRefresh) DEBUG_LOG("ERROR: dvpRefresh function not loaded");
    if (!pfn_dvpEnum) DEBUG_LOG("ERROR: dvpEnum function not loaded");
    if (!pfn_dvpOpen) DEBUG_LOG("ERROR: dvpOpen function not loaded");
    
    DEBUG_LOG("LoadDVPLibrary() completed successfully");
    return true;
}

// Dynamic loading wrapper functions
extern "C" {

dvpStatus dvpInit() {
    DEBUG_LOG("dvpInit() called");
    
    if (!LoadDVPLibrary()) {
        DEBUG_LOG("ERROR: Failed to load DVP library in dvpInit");
        return DVP_STATUS_NOT_INITIALIZED;
    }
    
    if (!pfn_dvpInit) {
        DEBUG_LOG("ERROR: dvpInit function pointer is null");
        return DVP_STATUS_FUNCTION_INVALID;
    }
    
    DEBUG_LOG("Calling actual dvpInit from DLL");
    dvpStatus result = pfn_dvpInit();
    std::stringstream ss;
    ss << "dvpInit returned status: " << result;
    DEBUG_LOG(ss.str());
    
    return result;
}

dvpStatus dvpExit() {
    if (!pfn_dvpExit) return DVP_STATUS_FUNCTION_INVALID;
    dvpStatus status = pfn_dvpExit();
    if (g_dvpLibrary) {
        FREE_LIBRARY(g_dvpLibrary);
        g_dvpLibrary = nullptr;
    }
    return status;
}

dvpStatus dvpRefresh(dvpUint32* pCount) {
    DEBUG_LOG("dvpRefresh() called");
    
    if (!LoadDVPLibrary()) {
        DEBUG_LOG("ERROR: Failed to load DVP library in dvpRefresh");
        return DVP_STATUS_NOT_INITIALIZED;
    }
    
    if (!pfn_dvpRefresh) {
        DEBUG_LOG("ERROR: dvpRefresh function pointer is null");
        return DVP_STATUS_FUNCTION_INVALID;
    }
    
    DEBUG_LOG("Calling actual dvpRefresh from DLL");
    dvpStatus result = pfn_dvpRefresh(pCount);
    std::stringstream ss;
    ss << "dvpRefresh returned status: " << result << ", count: ";
    if (pCount) {
        ss << *pCount;
    } else {
        ss << "null";
    }
    DEBUG_LOG(ss.str());
    
    return result;
}

dvpStatus dvpEnum(dvpUint32 index, dvpCameraInfo* pInfo) {
    if (!pfn_dvpEnum) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpEnum(index, pInfo);
}

dvpStatus dvpOpen(dvpUint32 index, dvpOpenMode mode, dvpHandle* pHandle) {
    if (!pfn_dvpOpen) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpOpen(index, mode, pHandle);
}

dvpStatus dvpOpenByName(const char* name, dvpOpenMode mode, dvpHandle* pHandle) {
    if (!pfn_dvpOpenByName) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpOpenByName(name, mode, pHandle);
}

dvpStatus dvpClose(dvpHandle handle) {
    if (!pfn_dvpClose) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpClose(handle);
}

dvpStatus dvpIsValid(dvpHandle handle, bool* pIsValid) {
    if (!pfn_dvpIsValid) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpIsValid(handle, pIsValid);
}

dvpStatus dvpStart(dvpHandle handle) {
    if (!pfn_dvpStart) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpStart(handle);
}

dvpStatus dvpStop(dvpHandle handle) {
    if (!pfn_dvpStop) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpStop(handle);
}

dvpStatus dvpGetStreamState(dvpHandle handle, dvpStreamState* pState) {
    if (!pfn_dvpGetStreamState) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetStreamState(handle, pState);
}

dvpStatus dvpGetFrame(dvpHandle handle, dvpFrame* pFrame, void** pBuffer, dvpUint32 timeout) {
    if (!pfn_dvpGetFrame) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetFrame(handle, pFrame, pBuffer, timeout);
}

dvpStatus dvpGetFrameBuffer(dvpHandle handle, dvpFrameBuffer* pFrameBuffer, dvpUint32 timeout) {
    if (!pfn_dvpGetFrameBuffer) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetFrameBuffer(handle, pFrameBuffer, timeout);
}

dvpStatus dvpRegisterStreamCallback(dvpHandle handle, dvpStreamCallback callback, dvpStreamEvent event, void* pContext) {
    if (!pfn_dvpRegisterStreamCallback) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpRegisterStreamCallback(handle, callback, event, pContext);
}

dvpStatus dvpUnregisterStreamCallback(dvpHandle handle, dvpStreamCallback callback, dvpStreamEvent event, void* pContext) {
    if (!pfn_dvpUnregisterStreamCallback) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpUnregisterStreamCallback(handle, callback, event, pContext);
}

dvpStatus dvpGetCameraInfo(dvpHandle handle, dvpCameraInfo* pInfo) {
    if (!pfn_dvpGetCameraInfo) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetCameraInfo(handle, pInfo);
}

dvpStatus dvpGetSensorInfo(dvpHandle handle, dvpSensorInfo* pInfo) {
    if (!pfn_dvpGetSensorInfo) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetSensorInfo(handle, pInfo);
}

dvpStatus dvpGetExposure(dvpHandle handle, dvpFloat64* pExposure) {
    if (!pfn_dvpGetExposure) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetExposure(handle, pExposure);
}

dvpStatus dvpSetExposure(dvpHandle handle, dvpFloat64 exposure) {
    if (!pfn_dvpSetExposure) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetExposure(handle, exposure);
}

dvpStatus dvpGetExposureDescr(dvpHandle handle, dvpDoubleDescr* pDescr) {
    if (!pfn_dvpGetExposureDescr) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetExposureDescr(handle, pDescr);
}

dvpStatus dvpGetAnalogGain(dvpHandle handle, float* pGain) {
    if (!pfn_dvpGetAnalogGain) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetAnalogGain(handle, pGain);
}

dvpStatus dvpSetAnalogGain(dvpHandle handle, float gain) {
    if (!pfn_dvpSetAnalogGain) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetAnalogGain(handle, gain);
}

dvpStatus dvpGetAnalogGainDescr(dvpHandle handle, dvpFloatDescr* pDescr) {
    if (!pfn_dvpGetAnalogGainDescr) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetAnalogGainDescr(handle, pDescr);
}

dvpStatus dvpGetRoi(dvpHandle handle, dvpRegion* pRoi) {
    if (!pfn_dvpGetRoi) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetRoi(handle, pRoi);
}

dvpStatus dvpSetRoi(dvpHandle handle, dvpRegion roi) {
    if (!pfn_dvpSetRoi) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetRoi(handle, roi);
}

dvpStatus dvpGetRoiDescr(dvpHandle handle, dvpRegionDescr* pDescr) {
    if (!pfn_dvpGetRoiDescr) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetRoiDescr(handle, pDescr);
}

dvpStatus dvpGetAeMode(dvpHandle handle, dvpAeMode* pMode) {
    if (!pfn_dvpGetAeMode) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetAeMode(handle, pMode);
}

dvpStatus dvpSetAeMode(dvpHandle handle, dvpAeMode mode) {
    if (!pfn_dvpSetAeMode) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetAeMode(handle, mode);
}

dvpStatus dvpGetAeOperation(dvpHandle handle, dvpAeOperation* pOperation) {
    if (!pfn_dvpGetAeOperation) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetAeOperation(handle, pOperation);
}

dvpStatus dvpSetAeOperation(dvpHandle handle, dvpAeOperation operation) {
    if (!pfn_dvpSetAeOperation) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetAeOperation(handle, operation);
}

dvpStatus dvpGetAwbOperation(dvpHandle handle, dvpAwbOperation* pOperation) {
    if (!pfn_dvpGetAwbOperation) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetAwbOperation(handle, pOperation);
}

dvpStatus dvpSetAwbOperation(dvpHandle handle, dvpAwbOperation operation) {
    if (!pfn_dvpSetAwbOperation) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetAwbOperation(handle, operation);
}

dvpStatus dvpGetRgbGain(dvpHandle handle, float* pRGain, float* pGGain, float* pBGain) {
    if (!pfn_dvpGetRgbGain) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetRgbGain(handle, pRGain, pGGain, pBGain);
}

dvpStatus dvpSetRgbGain(dvpHandle handle, float rGain, float gGain, float bGain) {
    if (!pfn_dvpSetRgbGain) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetRgbGain(handle, rGain, gGain, bGain);
}

dvpStatus dvpGetGamma(dvpHandle handle, dvpInt32* pGamma) {
    if (!pfn_dvpGetGamma) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetGamma(handle, pGamma);
}

dvpStatus dvpSetGamma(dvpHandle handle, dvpInt32 gamma) {
    if (!pfn_dvpSetGamma) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetGamma(handle, gamma);
}

dvpStatus dvpGetTargetFormat(dvpHandle handle, dvpStreamFormat* pFormat) {
    if (!pfn_dvpGetTargetFormat) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetTargetFormat(handle, pFormat);
}

dvpStatus dvpSetTargetFormat(dvpHandle handle, dvpStreamFormat format) {
    if (!pfn_dvpSetTargetFormat) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetTargetFormat(handle, format);
}

dvpStatus dvpGetResolutionModeSel(dvpHandle handle, dvpUint32* pIndex) {
    if (!pfn_dvpGetResolutionModeSel) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetResolutionModeSel(handle, pIndex);
}

dvpStatus dvpSetResolutionModeSel(dvpHandle handle, dvpUint32 index) {
    if (!pfn_dvpSetResolutionModeSel) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetResolutionModeSel(handle, index);
}

dvpStatus dvpGetResolutionModeSelDescr(dvpHandle handle, dvpSelectionDescr* pDescr) {
    if (!pfn_dvpGetResolutionModeSelDescr) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetResolutionModeSelDescr(handle, pDescr);
}

dvpStatus dvpGetResolutionModeSelDetail(dvpHandle handle, dvpUint32 index, dvpResolutionMode* pMode) {
    if (!pfn_dvpGetResolutionModeSelDetail) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetResolutionModeSelDetail(handle, index, pMode);
}

dvpStatus dvpGetTriggerSource(dvpHandle handle, dvpTriggerSource* pSource) {
    if (!pfn_dvpGetTriggerSource) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetTriggerSource(handle, pSource);
}

dvpStatus dvpSetTriggerSource(dvpHandle handle, dvpTriggerSource source) {
    if (!pfn_dvpSetTriggerSource) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetTriggerSource(handle, source);
}

dvpStatus dvpGetTriggerInputType(dvpHandle handle, dvpLine line, dvpTriggerInputType* pType) {
    if (!pfn_dvpGetTriggerInputType) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetTriggerInputType(handle, line, pType);
}

dvpStatus dvpSetTriggerInputType(dvpHandle handle, dvpTriggerInputType type) {
    if (!pfn_dvpSetTriggerInputType) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetTriggerInputType(handle, type);
}

dvpStatus dvpTriggerFire(dvpHandle handle) {
    if (!pfn_dvpTriggerFire) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpTriggerFire(handle);
}

dvpStatus dvpSetTriggerDelay(dvpHandle handle, dvpFloat64 delay) {
    if (!pfn_dvpSetTriggerDelay) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetTriggerDelay(handle, delay);
}

dvpStatus dvpGetBufferQueueSize(dvpHandle handle, dvpUint32* pSize) {
    if (!pfn_dvpGetBufferQueueSize) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetBufferQueueSize(handle, pSize);
}

dvpStatus dvpSetBufferQueueSize(dvpHandle handle, dvpUint32 size) {
    if (!pfn_dvpSetBufferQueueSize) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetBufferQueueSize(handle, size);
}

dvpStatus dvpGetFrameCount(dvpHandle handle, dvpFrameCount* pCount) {
    if (!pfn_dvpGetFrameCount) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetFrameCount(handle, pCount);
}

dvpStatus dvpGetTemperatureInfo(dvpHandle handle, dvpTemperatureInfo* pInfo) {
    if (!pfn_dvpGetTemperatureInfo) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetTemperatureInfo(handle, pInfo);
}

dvpStatus dvpGetColorCorrection(dvpHandle handle, dvpColorCorrection* pCorrection) {
    if (!pfn_dvpGetColorCorrection) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetColorCorrection(handle, pCorrection);
}

dvpStatus dvpSetColorCorrection(dvpHandle handle, dvpColorCorrection correction) {
    if (!pfn_dvpSetColorCorrection) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetColorCorrection(handle, correction);
}

dvpStatus dvpGetColorSolutionSel(dvpHandle handle, dvpUint32* pIndex) {
    if (!pfn_dvpGetColorSolutionSel) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetColorSolutionSel(handle, pIndex);
}

dvpStatus dvpSetColorSolutionSel(dvpHandle handle, dvpUint32 index) {
    if (!pfn_dvpSetColorSolutionSel) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetColorSolutionSel(handle, index);
}

dvpStatus dvpGetColorSolutionSelDescr(dvpHandle handle, dvpSelectionDescr* pDescr) {
    if (!pfn_dvpGetColorSolutionSelDescr) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetColorSolutionSelDescr(handle, pDescr);
}

dvpStatus dvpGetHardwareIspState(dvpHandle handle, bool* pState) {
    if (!pfn_dvpGetHardwareIspState) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetHardwareIspState(handle, pState);
}

dvpStatus dvpSetHardwareIspState(dvpHandle handle, bool state) {
    if (!pfn_dvpSetHardwareIspState) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetHardwareIspState(handle, state);
}

dvpStatus dvpSaveConfig(dvpHandle handle, const char* path) {
    if (!pfn_dvpSaveConfig) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSaveConfig(handle, path);
}

dvpStatus dvpLoadConfig(dvpHandle handle, const char* path) {
    if (!pfn_dvpLoadConfig) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpLoadConfig(handle, path);
}

dvpStatus dvpReadUserData(dvpHandle handle, dvpUint32 offset, void* pBuffer, dvpUint32 size) {
    if (!pfn_dvpReadUserData) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpReadUserData(handle, offset, pBuffer, size);
}

dvpStatus dvpWriteUserData(dvpHandle handle, dvpUint32 offset, const void* pBuffer, dvpUint32 size) {
    if (!pfn_dvpWriteUserData) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpWriteUserData(handle, offset, pBuffer, size);
}

dvpStatus dvpGetOutputIoLevel(dvpHandle handle, dvpOutputIo io, bool* pLevel) {
    if (!pfn_dvpGetOutputIoLevel) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetOutputIoLevel(handle, io, pLevel);
}

dvpStatus dvpSetOutputIoLevel(dvpHandle handle, dvpOutputIo io, bool level) {
    if (!pfn_dvpSetOutputIoLevel) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetOutputIoLevel(handle, io, level);
}

dvpStatus dvpGetInputIoLevel(dvpHandle handle, dvpInputIo io, bool* pLevel) {
    if (!pfn_dvpGetInputIoLevel) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetInputIoLevel(handle, io, pLevel);
}

dvpStatus dvpSetOutputIoFunction(dvpHandle handle, dvpOutputIo io, dvpOutputIoFunction func) {
    if (!pfn_dvpSetOutputIoFunction) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetOutputIoFunction(handle, io, func);
}

dvpStatus dvpGetPacketSizeSel(dvpHandle handle, dvpUint32* pSize) {
    if (!pfn_dvpGetPacketSizeSel) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpGetPacketSizeSel(handle, pSize);
}

dvpStatus dvpSetPacketSizeSel(dvpHandle handle, dvpUint32 size) {
    if (!pfn_dvpSetPacketSizeSel) return DVP_STATUS_FUNCTION_INVALID;
    return pfn_dvpSetPacketSizeSel(handle, size);
}

} // extern "C"

#else // !USE_DYNAMIC_LOADING

// ============================================================================
// Stub Implementation
// ============================================================================

extern "C" {

dvpStatus dvpInit() {
    // Stub: always succeed
    return DVP_STATUS_OK;
}

dvpStatus dvpExit() {
    return DVP_STATUS_OK;
}

dvpStatus dvpRefresh(dvpUint32* pCount) {
    if (!pCount) return DVP_STATUS_PARAMETER_INVALID;
    *pCount = 0;  // No devices in stub mode
    return DVP_STATUS_OK;
}

dvpStatus dvpEnum(dvpUint32 index, dvpCameraInfo* pInfo) {
    return DVP_STATUS_INVALID_INDEX;  // No devices in stub mode
}

dvpStatus dvpOpen(dvpUint32 index, dvpOpenMode mode, dvpHandle* pHandle) {
    return DVP_STATUS_INVALID_INDEX;  // No devices in stub mode
}

dvpStatus dvpOpenByName(const char* name, dvpOpenMode mode, dvpHandle* pHandle) {
    return DVP_STATUS_UNCONNECTED;  // Can't connect in stub mode
}

dvpStatus dvpClose(dvpHandle handle) {
    return DVP_STATUS_OK;
}

dvpStatus dvpIsValid(dvpHandle handle, bool* pIsValid) {
    if (!pIsValid) return DVP_STATUS_PARAMETER_INVALID;
    *pIsValid = false;  // No valid handles in stub mode
    return DVP_STATUS_OK;
}

dvpStatus dvpStart(dvpHandle handle) {
    return DVP_STATUS_INVALID_HANDLE;
}

dvpStatus dvpStop(dvpHandle handle) {
    return DVP_STATUS_INVALID_HANDLE;
}

dvpStatus dvpGetStreamState(dvpHandle handle, dvpStreamState* pState) {
    if (!pState) return DVP_STATUS_PARAMETER_INVALID;
    *pState = STATE_STOPED;
    return DVP_STATUS_OK;
}

dvpStatus dvpGetFrame(dvpHandle handle, dvpFrame* pFrame, void** pBuffer, dvpUint32 timeout) {
    return DVP_STATUS_INVALID_HANDLE;
}

dvpStatus dvpGetFrameBuffer(dvpHandle handle, dvpFrameBuffer* pFrameBuffer, dvpUint32 timeout) {
    return DVP_STATUS_INVALID_HANDLE;
}

dvpStatus dvpRegisterStreamCallback(dvpHandle handle, dvpStreamCallback callback, dvpStreamEvent event, void* pContext) {
    return DVP_STATUS_INVALID_HANDLE;
}

dvpStatus dvpUnregisterStreamCallback(dvpHandle handle, dvpStreamCallback callback, dvpStreamEvent event, void* pContext) {
    return DVP_STATUS_INVALID_HANDLE;
}

dvpStatus dvpGetCameraInfo(dvpHandle handle, dvpCameraInfo* pInfo) {
    if (!pInfo) return DVP_STATUS_PARAMETER_INVALID;
    memset(pInfo, 0, sizeof(dvpCameraInfo));
    strcpy(pInfo->Model, "STUB_CAMERA");
    strcpy(pInfo->Manufacturer, "STUB");
    return DVP_STATUS_OK;
}

dvpStatus dvpGetSensorInfo(dvpHandle handle, dvpSensorInfo* pInfo) {
    if (!pInfo) return DVP_STATUS_PARAMETER_INVALID;
    memset(pInfo, 0, sizeof(dvpSensorInfo));
    strcpy(pInfo->descr, "STUB_SENSOR");
    pInfo->sensor = SENSOR_TYPE_CMOS;
    pInfo->pixel = SENSOR_PIXEL_MONO;
    pInfo->region.iMaxW = 1920;
    pInfo->region.iMaxH = 1080;
    return DVP_STATUS_OK;
}

dvpStatus dvpGetExposure(dvpHandle handle, dvpFloat64* pExposure) {
    if (!pExposure) return DVP_STATUS_PARAMETER_INVALID;
    *pExposure = 10000.0;  // 10ms default
    return DVP_STATUS_OK;
}

dvpStatus dvpSetExposure(dvpHandle handle, dvpFloat64 exposure) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetExposureDescr(dvpHandle handle, dvpDoubleDescr* pDescr) {
    if (!pDescr) return DVP_STATUS_PARAMETER_INVALID;
    pDescr->fMin = 10.0;
    pDescr->fMax = 1000000.0;
    pDescr->fStep = 1.0;
    pDescr->fDefault = 10000.0;
    pDescr->bIsValid = true;
    return DVP_STATUS_OK;
}

dvpStatus dvpGetAnalogGain(dvpHandle handle, float* pGain) {
    if (!pGain) return DVP_STATUS_PARAMETER_INVALID;
    *pGain = 1.0f;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetAnalogGain(dvpHandle handle, float gain) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetAnalogGainDescr(dvpHandle handle, dvpFloatDescr* pDescr) {
    if (!pDescr) return DVP_STATUS_PARAMETER_INVALID;
    pDescr->fMin = 1.0f;
    pDescr->fMax = 16.0f;
    pDescr->fStep = 0.1f;
    pDescr->fDefault = 1.0f;
    pDescr->bIsValid = true;
    return DVP_STATUS_OK;
}

dvpStatus dvpGetRoi(dvpHandle handle, dvpRegion* pRoi) {
    if (!pRoi) return DVP_STATUS_PARAMETER_INVALID;
    pRoi->X = 0;
    pRoi->Y = 0;
    pRoi->W = 1920;
    pRoi->H = 1080;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetRoi(dvpHandle handle, dvpRegion roi) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetRoiDescr(dvpHandle handle, dvpRegionDescr* pDescr) {
    if (!pDescr) return DVP_STATUS_PARAMETER_INVALID;
    pDescr->iMinW = 16;
    pDescr->iMinH = 16;
    pDescr->iMaxW = 1920;
    pDescr->iMaxH = 1080;
    pDescr->iStepW = 2;
    pDescr->iStepH = 2;
    return DVP_STATUS_OK;
}

// Stub implementations for remaining functions
dvpStatus dvpGetAeMode(dvpHandle handle, dvpAeMode* pMode) {
    if (!pMode) return DVP_STATUS_PARAMETER_INVALID;
    *pMode = AE_MODE_AE_AG;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetAeMode(dvpHandle handle, dvpAeMode mode) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetAeOperation(dvpHandle handle, dvpAeOperation* pOperation) {
    if (!pOperation) return DVP_STATUS_PARAMETER_INVALID;
    *pOperation = AE_OP_OFF;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetAeOperation(dvpHandle handle, dvpAeOperation operation) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetAwbOperation(dvpHandle handle, dvpAwbOperation* pOperation) {
    if (!pOperation) return DVP_STATUS_PARAMETER_INVALID;
    *pOperation = AWB_OP_OFF;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetAwbOperation(dvpHandle handle, dvpAwbOperation operation) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetRgbGain(dvpHandle handle, float* pRGain, float* pGGain, float* pBGain) {
    if (pRGain) *pRGain = 1.0f;
    if (pGGain) *pGGain = 1.0f;
    if (pBGain) *pBGain = 1.0f;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetRgbGain(dvpHandle handle, float rGain, float gGain, float bGain) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetGamma(dvpHandle handle, dvpInt32* pGamma) {
    if (!pGamma) return DVP_STATUS_PARAMETER_INVALID;
    *pGamma = 100;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetGamma(dvpHandle handle, dvpInt32 gamma) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetTargetFormat(dvpHandle handle, dvpStreamFormat* pFormat) {
    if (!pFormat) return DVP_STATUS_PARAMETER_INVALID;
    *pFormat = S_MONO8;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetTargetFormat(dvpHandle handle, dvpStreamFormat format) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetResolutionModeSel(dvpHandle handle, dvpUint32* pIndex) {
    if (!pIndex) return DVP_STATUS_PARAMETER_INVALID;
    *pIndex = 0;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetResolutionModeSel(dvpHandle handle, dvpUint32 index) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetResolutionModeSelDescr(dvpHandle handle, dvpSelectionDescr* pDescr) {
    if (!pDescr) return DVP_STATUS_PARAMETER_INVALID;
    pDescr->uCount = 1;
    pDescr->uDefault = 0;
    pDescr->bIsValid = true;
    return DVP_STATUS_OK;
}

dvpStatus dvpGetResolutionModeSelDetail(dvpHandle handle, dvpUint32 index, dvpResolutionMode* pMode) {
    if (!pMode) return DVP_STATUS_PARAMETER_INVALID;
    if (index > 0) return DVP_STATUS_INVALID_INDEX;
    pMode->iWidth = 1920;
    pMode->iHeight = 1080;
    strcpy(pMode->selection, "1920x1080");
    pMode->roi.X = 0;
    pMode->roi.Y = 0;
    pMode->roi.W = 1920;
    pMode->roi.H = 1080;
    return DVP_STATUS_OK;
}

dvpStatus dvpGetTriggerSource(dvpHandle handle, dvpTriggerSource* pSource) {
    if (!pSource) return DVP_STATUS_PARAMETER_INVALID;
    *pSource = TRIGGER_SOURCE_SOFTWARE;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetTriggerSource(dvpHandle handle, dvpTriggerSource source) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetTriggerInputType(dvpHandle handle, dvpLine line, dvpTriggerInputType* pType) {
    if (!pType) return DVP_STATUS_PARAMETER_INVALID;
    *pType = TRIGGER_IN_OFF;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetTriggerInputType(dvpHandle handle, dvpTriggerInputType type) {
    return DVP_STATUS_OK;
}

dvpStatus dvpTriggerFire(dvpHandle handle) {
    return DVP_STATUS_OK;
}

dvpStatus dvpSetTriggerDelay(dvpHandle handle, dvpFloat64 delay) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetBufferQueueSize(dvpHandle handle, dvpUint32* pSize) {
    if (!pSize) return DVP_STATUS_PARAMETER_INVALID;
    *pSize = 10;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetBufferQueueSize(dvpHandle handle, dvpUint32 size) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetFrameCount(dvpHandle handle, dvpFrameCount* pCount) {
    if (!pCount) return DVP_STATUS_PARAMETER_INVALID;
    memset(pCount, 0, sizeof(dvpFrameCount));
    return DVP_STATUS_OK;
}

dvpStatus dvpGetTemperatureInfo(dvpHandle handle, dvpTemperatureInfo* pInfo) {
    if (!pInfo) return DVP_STATUS_PARAMETER_INVALID;
    pInfo->fDevice = 25.0f;
    pInfo->fSensor = 25.0f;
    pInfo->fChip = 25.0f;
    return DVP_STATUS_OK;
}

dvpStatus dvpGetColorCorrection(dvpHandle handle, dvpColorCorrection* pCorrection) {
    if (!pCorrection) return DVP_STATUS_PARAMETER_INVALID;
    memset(pCorrection, 0, sizeof(dvpColorCorrection));
    for (int i = 0; i < 3; ++i) {
        pCorrection->fMatrix[i][i] = 1.0f;
    }
    return DVP_STATUS_OK;
}

dvpStatus dvpSetColorCorrection(dvpHandle handle, dvpColorCorrection correction) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetColorSolutionSel(dvpHandle handle, dvpUint32* pIndex) {
    if (!pIndex) return DVP_STATUS_PARAMETER_INVALID;
    *pIndex = 0;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetColorSolutionSel(dvpHandle handle, dvpUint32 index) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetColorSolutionSelDescr(dvpHandle handle, dvpSelectionDescr* pDescr) {
    if (!pDescr) return DVP_STATUS_PARAMETER_INVALID;
    pDescr->uCount = 0;
    pDescr->uDefault = 0;
    pDescr->bIsValid = false;
    return DVP_STATUS_OK;
}

dvpStatus dvpGetHardwareIspState(dvpHandle handle, bool* pState) {
    if (!pState) return DVP_STATUS_PARAMETER_INVALID;
    *pState = false;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetHardwareIspState(dvpHandle handle, bool state) {
    return DVP_STATUS_OK;
}

dvpStatus dvpSaveConfig(dvpHandle handle, const char* path) {
    return DVP_STATUS_OK;
}

dvpStatus dvpLoadConfig(dvpHandle handle, const char* path) {
    return DVP_STATUS_OK;
}

dvpStatus dvpReadUserData(dvpHandle handle, dvpUint32 offset, void* pBuffer, dvpUint32 size) {
    if (!pBuffer) return DVP_STATUS_PARAMETER_INVALID;
    memset(pBuffer, 0, size);
    return DVP_STATUS_OK;
}

dvpStatus dvpWriteUserData(dvpHandle handle, dvpUint32 offset, const void* pBuffer, dvpUint32 size) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetOutputIoLevel(dvpHandle handle, dvpOutputIo io, bool* pLevel) {
    if (!pLevel) return DVP_STATUS_PARAMETER_INVALID;
    *pLevel = false;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetOutputIoLevel(dvpHandle handle, dvpOutputIo io, bool level) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetInputIoLevel(dvpHandle handle, dvpInputIo io, bool* pLevel) {
    if (!pLevel) return DVP_STATUS_PARAMETER_INVALID;
    *pLevel = false;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetOutputIoFunction(dvpHandle handle, dvpOutputIo io, dvpOutputIoFunction func) {
    return DVP_STATUS_OK;
}

dvpStatus dvpGetPacketSizeSel(dvpHandle handle, dvpUint32* pSize) {
    if (!pSize) return DVP_STATUS_PARAMETER_INVALID;
    *pSize = 1500;
    return DVP_STATUS_OK;
}

dvpStatus dvpSetPacketSizeSel(dvpHandle handle, dvpUint32 size) {
    return DVP_STATUS_OK;
}

} // extern "C"

#endif // USE_DYNAMIC_LOADING