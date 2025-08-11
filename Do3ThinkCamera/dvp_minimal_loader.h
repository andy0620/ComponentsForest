#ifndef DVP_MINIMAL_LOADER_H
#define DVP_MINIMAL_LOADER_H

#include <QLibrary>
#include <QString>
#include <QDebug>

// When using dynamic loading, we don't want dllimport or dllexport
// We'll load functions dynamically at runtime
#ifdef USE_DYNAMIC_LOADING
    #ifdef _WIN32
        #ifdef __cplusplus
            #define dvp2api extern "C"
        #else
            #define dvp2api extern
        #endif
    #else
        #ifdef __cplusplus
            #define dvp2api extern "C"
        #else
            #define dvp2api extern
        #endif
    #endif
#endif

#include "dvp_wrapper.h"

// Minimal dynamic loader with only essential functions
class DvpMinimalLoader {
public:
    static DvpMinimalLoader& instance() {
        static DvpMinimalLoader instance;
        return instance;
    }
    
    bool load() {
        if (m_loaded) return true;
        
        // Try common installation paths
        QStringList paths = {
            "C:/Program Files (x86)/Camera/DVP2 x64/DVPCamera64.dll",
            "C:/Program Files (x86)/Camera/DVP2/DVPCamera.dll",
            "DVPCamera64.dll",  // Try system PATH
            "DVPCamera.dll"
        };
        
        for (const QString& path : paths) {
            m_library.setFileName(path);
            if (m_library.load()) {
                qDebug() << "Loaded Do3Think SDK from:" << path;
                if (loadFunctions()) {
                    m_loaded = true;
                    return true;
                }
                m_library.unload();
            }
        }
        
        qWarning() << "Failed to load Do3Think SDK DLL. Error:" << m_library.errorString();
        return false;
    }
    
    // Declare all SDK functions we need as function pointers
    // Add functions as needed when compilation fails
    
private:
    DvpMinimalLoader() = default;
    ~DvpMinimalLoader() { 
        if (m_loaded) {
            m_library.unload();
        }
    }
    
    bool loadFunctions() {
        // For now return true, we'll add function loading as needed
        return true;
    }
    
    QLibrary m_library;
    bool m_loaded = false;
};

#ifdef USE_DYNAMIC_LOADING
// Dynamic loading implementation for runtime DLL loading
// This approach loads all functions directly without function pointers
// We redefine each dvp function to call through GetProcAddress
#ifdef _WIN32
#include <windows.h>

static HMODULE g_dvpModule = nullptr;

inline bool LoadDvpLibrary() {
    if (g_dvpModule) return true;
    
    // Try to load from known locations
    const wchar_t* paths[] = {
        L"C:\\Program Files (x86)\\Camera\\DVP2 x64\\DVPCamera64.dll",
        L"C:\\Program Files (x86)\\Camera\\DVP2\\DVPCamera.dll",
        L"DVPCamera64.dll",
        L"DVPCamera.dll"
    };
    
    for (const auto& path : paths) {
        g_dvpModule = LoadLibraryW(path);
        if (g_dvpModule) {
            qDebug() << "Loaded DVP SDK from:" << QString::fromWCharArray(path);
            return true;
        }
    }
    
    qWarning() << "Failed to load DVP SDK DLL";
    return false;
}

// Define wrapper functions that load dynamically
#define DEFINE_DVP_FUNCTION(returnType, funcName, ...) \
    inline returnType funcName(__VA_ARGS__) { \
        if (!g_dvpModule && !LoadDvpLibrary()) { \
            return DVP_STATUS_NOT_INITIALIZED; \
        } \
        typedef returnType (*FuncType)(__VA_ARGS__); \
        static FuncType func = nullptr; \
        if (!func) { \
            func = (FuncType)GetProcAddress(g_dvpModule, #funcName); \
            if (!func) { \
                qWarning() << "Failed to load function:" << #funcName; \
                return DVP_STATUS_FUNCTION_ERROR; \
            } \
        } \
        return func; \
    }

// For functions with parameters, we need to list them twice
#define DVP_FUNC_0(ret, name) \
    static inline ret name() { \
        if (!g_dvpModule && !LoadDvpLibrary()) return DVP_STATUS_NOT_INITIALIZED; \
        typedef ret (*FuncType)(); \
        static FuncType func = (FuncType)GetProcAddress(g_dvpModule, #name); \
        if (!func) return DVP_STATUS_FAILED; \
        return func(); \
    }

#define DVP_FUNC_1(ret, name, t1, p1) \
    static inline ret name(t1 p1) { \
        if (!g_dvpModule && !LoadDvpLibrary()) return DVP_STATUS_NOT_INITIALIZED; \
        typedef ret (*FuncType)(t1); \
        static FuncType func = (FuncType)GetProcAddress(g_dvpModule, #name); \
        if (!func) return DVP_STATUS_FAILED; \
        return func(p1); \
    }

#define DVP_FUNC_2(ret, name, t1, p1, t2, p2) \
    static inline ret name(t1 p1, t2 p2) { \
        if (!g_dvpModule && !LoadDvpLibrary()) return DVP_STATUS_NOT_INITIALIZED; \
        typedef ret (*FuncType)(t1, t2); \
        static FuncType func = (FuncType)GetProcAddress(g_dvpModule, #name); \
        if (!func) return DVP_STATUS_FAILED; \
        return func(p1, p2); \
    }

#define DVP_FUNC_3(ret, name, t1, p1, t2, p2, t3, p3) \
    static inline ret name(t1 p1, t2 p2, t3 p3) { \
        if (!g_dvpModule && !LoadDvpLibrary()) return DVP_STATUS_NOT_INITIALIZED; \
        typedef ret (*FuncType)(t1, t2, t3); \
        static FuncType func = (FuncType)GetProcAddress(g_dvpModule, #name); \
        if (!func) return DVP_STATUS_FAILED; \
        return func(p1, p2, p3); \
    }

#define DVP_FUNC_4(ret, name, t1, p1, t2, p2, t3, p3, t4, p4) \
    static inline ret name(t1 p1, t2 p2, t3 p3, t4 p4) { \
        if (!g_dvpModule && !LoadDvpLibrary()) return DVP_STATUS_NOT_INITIALIZED; \
        typedef ret (*FuncType)(t1, t2, t3, t4); \
        static FuncType func = (FuncType)GetProcAddress(g_dvpModule, #name); \
        if (!func) return DVP_STATUS_FAILED; \
        return func(p1, p2, p3, p4); \
    }

// Now define all the functions we need - static to avoid multiple definition errors
DVP_FUNC_0(dvpStatus, dvpInit)
DVP_FUNC_0(dvpStatus, dvpUninit)
DVP_FUNC_1(dvpStatus, dvpRefresh, dvpUint32*, pCount)
DVP_FUNC_2(dvpStatus, dvpEnum, dvpUint32, index, dvpCameraInfo*, pInfo)
DVP_FUNC_3(dvpStatus, dvpOpenByName, const char*, name, dvpOpenMode, mode, dvpHandle*, pHandle)
DVP_FUNC_1(dvpStatus, dvpClose, dvpHandle, handle)
DVP_FUNC_2(dvpStatus, dvpIsValid, dvpHandle, handle, bool*, pValid)
DVP_FUNC_1(dvpStatus, dvpStart, dvpHandle, handle)
DVP_FUNC_1(dvpStatus, dvpStop, dvpHandle, handle)
DVP_FUNC_4(dvpStatus, dvpGetFrame, dvpHandle, handle, dvpFrame*, pFrame, void**, pBuffer, dvpUint32, timeout)
DVP_FUNC_4(dvpStatus, dvpRegisterStreamCallback, dvpHandle, handle, dvpStreamCallback, proc, dvpStreamEvent, event, void*, pContext)
DVP_FUNC_4(dvpStatus, dvpUnregisterStreamCallback, dvpHandle, handle, dvpStreamCallback, proc, dvpStreamEvent, event, void*, pContext)

// Camera control functions
DVP_FUNC_2(dvpStatus, dvpGetExposure, dvpHandle, handle, float*, pExposure)
DVP_FUNC_2(dvpStatus, dvpSetExposure, dvpHandle, handle, float, exposure)
DVP_FUNC_2(dvpStatus, dvpGetAnalogGain, dvpHandle, handle, float*, pGain)
DVP_FUNC_2(dvpStatus, dvpSetAnalogGain, dvpHandle, handle, float, gain)
DVP_FUNC_2(dvpStatus, dvpGetRoi, dvpHandle, handle, dvpRegion*, pRoi)
DVP_FUNC_2(dvpStatus, dvpSetRoi, dvpHandle, handle, dvpRegion, roi)
DVP_FUNC_1(dvpStatus, dvpTriggerFire, dvpHandle, handle)
DVP_FUNC_2(dvpStatus, dvpGetFrameCount, dvpHandle, handle, dvpFrameCount*, pCount)

// Add more functions as compilation requires them
DVP_FUNC_2(dvpStatus, dvpGetCameraInfo, dvpHandle, handle, dvpCameraInfo*, pInfo)
DVP_FUNC_2(dvpStatus, dvpGetSensorInfo, dvpHandle, handle, dvpSensorInfo*, pInfo)
DVP_FUNC_2(dvpStatus, dvpGetTemperatureInfo, dvpHandle, handle, dvpTemperatureInfo*, pInfo)
DVP_FUNC_2(dvpStatus, dvpGetStreamState, dvpHandle, handle, dvpStreamState*, pState)
DVP_FUNC_2(dvpStatus, dvpGetGamma, dvpHandle, handle, dvpInt32*, pGamma)
DVP_FUNC_2(dvpStatus, dvpSetGamma, dvpHandle, handle, dvpInt32, gamma)
DVP_FUNC_2(dvpStatus, dvpGetRoiDescr, dvpHandle, handle, dvpRegionDescr*, pDescr)
DVP_FUNC_2(dvpStatus, dvpGetAnalogGainDescr, dvpHandle, handle, dvpFloatDescr*, pDescr)
DVP_FUNC_2(dvpStatus, dvpGetExposureDescr, dvpHandle, handle, dvpDoubleDescr*, pDescr)
DVP_FUNC_2(dvpStatus, dvpSetTriggerDelay, dvpHandle, handle, double, delay)
DVP_FUNC_2(dvpStatus, dvpGetBufferQueueSize, dvpHandle, handle, dvpUint32*, pSize)
DVP_FUNC_2(dvpStatus, dvpSetBufferQueueSize, dvpHandle, handle, dvpUint32, size)
DVP_FUNC_2(dvpStatus, dvpGetTriggerSource, dvpHandle, handle, dvpTriggerSource*, pSource)
DVP_FUNC_2(dvpStatus, dvpSetTriggerSource, dvpHandle, handle, dvpTriggerSource, source)
DVP_FUNC_2(dvpStatus, dvpSetColorCorrection, dvpHandle, handle, dvpColorCorrection, correction)
DVP_FUNC_2(dvpStatus, dvpGetHardwareIspState, dvpHandle, handle, bool*, pState)
DVP_FUNC_2(dvpStatus, dvpSetHardwareIspState, dvpHandle, handle, bool, state)
DVP_FUNC_2(dvpStatus, dvpGetAeMode, dvpHandle, handle, dvpAeMode*, pMode)
DVP_FUNC_2(dvpStatus, dvpSetAeMode, dvpHandle, handle, dvpAeMode, mode)
DVP_FUNC_2(dvpStatus, dvpGetAeOperation, dvpHandle, handle, dvpAeOperation*, pOp)
DVP_FUNC_2(dvpStatus, dvpGetAwbOperation, dvpHandle, handle, dvpAwbOperation*, pOp)
DVP_FUNC_2(dvpStatus, dvpSetTriggerInputType, dvpHandle, handle, dvpTriggerInputType, type)
DVP_FUNC_2(dvpStatus, dvpGetTargetFormat, dvpHandle, handle, dvpStreamFormat*, pFormat)
DVP_FUNC_2(dvpStatus, dvpSetTargetFormat, dvpHandle, handle, dvpStreamFormat, format)
DVP_FUNC_3(dvpStatus, dvpGetInputIoLevel, dvpHandle, handle, dvpInputIo, io, bool*, pLevel)
DVP_FUNC_3(dvpStatus, dvpSetOutputIoLevel, dvpHandle, handle, dvpOutputIo, io, bool, level)
DVP_FUNC_3(dvpStatus, dvpSetOutputIoFunction, dvpHandle, handle, dvpOutputIo, io, dvpOutputIoFunction, function)
DVP_FUNC_2(dvpStatus, dvpSetColorSolutionSel, dvpHandle, handle, dvpUint32, sel)
DVP_FUNC_2(dvpStatus, dvpGetColorSolutionSel, dvpHandle, handle, dvpUint32*, pSel)
DVP_FUNC_2(dvpStatus, dvpGetColorSolutionSelDescr, dvpHandle, handle, dvpSelectionDescr*, pDescr)
DVP_FUNC_2(dvpStatus, dvpGetResolutionModeSel, dvpHandle, handle, dvpUint32*, pSel)
DVP_FUNC_2(dvpStatus, dvpSetResolutionModeSel, dvpHandle, handle, dvpUint32, sel)
DVP_FUNC_2(dvpStatus, dvpGetResolutionModeSelDescr, dvpHandle, handle, dvpSelectionDescr*, pDescr)
DVP_FUNC_3(dvpStatus, dvpGetResolutionModeSelDetail, dvpHandle, handle, dvpUint32, index, dvpResolutionMode*, pMode)
DVP_FUNC_2(dvpStatus, dvpSetPacketSizeSel, dvpHandle, handle, dvpUint32, sel)
DVP_FUNC_4(dvpStatus, dvpSetRgbGain, dvpHandle, handle, float, red, float, green, float, blue)
DVP_FUNC_4(dvpStatus, dvpGetRgbGain, dvpHandle, handle, float*, pRed, float*, pGreen, float*, pBlue)
DVP_FUNC_2(dvpStatus, dvpLoadConfig, dvpHandle, handle, const char*, path)
DVP_FUNC_2(dvpStatus, dvpSaveConfig, dvpHandle, handle, const char*, path)
DVP_FUNC_4(dvpStatus, dvpReadUserData, dvpHandle, handle, dvpUint32, addr, void*, pData, dvpUint32, size)
DVP_FUNC_4(dvpStatus, dvpWriteUserData, dvpHandle, handle, dvpUint32, addr, const void*, pData, dvpUint32, size)
DVP_FUNC_2(dvpStatus, dvpGetTriggerState, dvpHandle, handle, bool*, pState)
DVP_FUNC_2(dvpStatus, dvpSetTriggerState, dvpHandle, handle, bool, state)
DVP_FUNC_2(dvpStatus, dvpGetFrameRate, dvpHandle, handle, double*, pRate)
DVP_FUNC_2(dvpStatus, dvpSetFrameRate, dvpHandle, handle, double, rate)
DVP_FUNC_2(dvpStatus, dvpGetColorCorrection, dvpHandle, handle, dvpColorCorrection*, pCorrection)
DVP_FUNC_3(dvpStatus, dvpGetOutputIoFunction, dvpHandle, handle, dvpOutputIo, io, dvpOutputIoFunction*, pFunction)
DVP_FUNC_3(dvpStatus, dvpGetOutputIoLevel, dvpHandle, handle, dvpOutputIo, io, bool*, pLevel)

// Version array - defined when using dynamic loading
inline const unsigned int* getDvpVersion() {
    static unsigned int version[4] = {2, 25, 43, 1332};
    return version;
}
#define dvpVersion (*getDvpVersion())

#else // !_WIN32
#error "Dynamic loading is currently only supported on Windows"
#endif // _WIN32

#endif // USE_DYNAMIC_LOADING

#endif // DVP_MINIMAL_LOADER_H