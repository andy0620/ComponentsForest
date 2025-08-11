#ifndef DVP_DYNAMIC_LOADER_H
#define DVP_DYNAMIC_LOADER_H

#include <QLibrary>
#include <QString>
#include <QDebug>
#include "dvp_wrapper.h"

class DvpDynamicLoader {
public:
    static DvpDynamicLoader& instance() {
        static DvpDynamicLoader instance;
        return instance;
    }
    
    bool load() {
        if (m_loaded) return true;
        
        // Try common installation paths
        QStringList paths = {
            "C:/Program Files (x86)/Camera/DVP2 x64/DVPCamera64.dll",
            "C:/Program Files (x86)/Camera/DVP2/DVPCamera.dll",
            "C:/Program Files/Teledyne DALSA/Sherlockx64/Bin/DVPCamera64.dll",
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
        
        qWarning() << "Failed to load Do3Think SDK DLL";
        return false;
    }
    
    void unload() {
        if (m_loaded) {
            m_library.unload();
            m_loaded = false;
        }
    }
    
    // Function pointers
    dvpStatus (*dvpInit)() = nullptr;
    dvpStatus (*dvpUninit)() = nullptr;
    dvpStatus (*dvpRefresh)(dvpUint32* pCameraNum) = nullptr;
    dvpStatus (*dvpEnum)(dvpUint32 index, dvpCameraInfo* pCameraInfo) = nullptr;
    dvpStatus (*dvpOpen)(dvpUint32 index, dvpOpenMode mode, dvpHandle* pHandle) = nullptr;
    dvpStatus (*dvpOpenByName)(const char* name, dvpOpenMode mode, dvpHandle* pHandle) = nullptr;
    dvpStatus (*dvpClose)(dvpHandle handle) = nullptr;
    dvpStatus (*dvpIsValid)(dvpHandle handle, bool* pValid) = nullptr;
    dvpStatus (*dvpStart)(dvpHandle handle) = nullptr;
    dvpStatus (*dvpStop)(dvpHandle handle) = nullptr;
    dvpStatus (*dvpGetStreamState)(dvpHandle handle, dvpStreamState* pState) = nullptr;
    dvpStatus (*dvpGetFrame)(dvpHandle handle, dvpFrame* pFrame, void** pBuffer, dvpUint32 timeout) = nullptr;
    dvpStatus (*dvpGetFrameBuffer)(dvpHandle handle, dvpFrame* pFrame, dvpBuffer* pBuffer, dvpUint32 timeout) = nullptr;
    
    // Callback functions
    dvpStatus (*dvpRegisterStreamCallback)(dvpHandle handle, dvpStreamCallback callback, dvpStreamEvent event, void* pUserData) = nullptr;
    dvpStatus (*dvpUnregisterStreamCallback)(dvpHandle handle, dvpStreamCallback callback, dvpStreamEvent event, void* pUserData) = nullptr;
    
    // Camera control functions
    dvpStatus (*dvpGetExposure)(dvpHandle handle, float* pExposure) = nullptr;
    dvpStatus (*dvpSetExposure)(dvpHandle handle, float exposure) = nullptr;
    dvpStatus (*dvpGetAnalogGain)(dvpHandle handle, float* pGain) = nullptr;
    dvpStatus (*dvpSetAnalogGain)(dvpHandle handle, float gain) = nullptr;
    dvpStatus (*dvpGetRoi)(dvpHandle handle, dvpRegion* pRoi) = nullptr;
    dvpStatus (*dvpSetRoi)(dvpHandle handle, dvpRegion roi) = nullptr;
    // These functions are commented out as they use types not yet available
    // You can add them later when needed
    // dvpStatus (*dvpGetResolution)(dvpHandle handle, dvpResolution* pResolution) = nullptr;
    // dvpStatus (*dvpSetResolution)(dvpHandle handle, dvpResolution resolution) = nullptr;
    dvpStatus (*dvpTriggerFire)(dvpHandle handle) = nullptr;
    // dvpStatus (*dvpGetTriggerState)(dvpHandle handle, dvpTriggerState* pState) = nullptr;
    // dvpStatus (*dvpSetTriggerState)(dvpHandle handle, dvpTriggerState state) = nullptr;
    // dvpStatus (*dvpGetTriggerSource)(dvpHandle handle, dvpTriggerSource* pSource) = nullptr;
    // dvpStatus (*dvpSetTriggerSource)(dvpHandle handle, dvpTriggerSource source) = nullptr;
    
    // Frame rate functions
    dvpStatus (*dvpGetFrameRate)(dvpHandle handle, double* pFrameRate) = nullptr;
    dvpStatus (*dvpSetFrameRate)(dvpHandle handle, double frameRate) = nullptr;
    
    // Frame count functions
    dvpStatus (*dvpGetFrameCount)(dvpHandle handle, dvpFrameCount* pCount) = nullptr;
    dvpStatus (*dvpResetFrameCount)(dvpHandle handle) = nullptr;
    
    // Add more function pointers as needed
    
private:
    DvpDynamicLoader() = default;
    ~DvpDynamicLoader() { unload(); }
    
    bool loadFunctions() {
        // Load all required functions
        dvpInit = (dvpStatus (*)())m_library.resolve("dvpInit");
        dvpUninit = (dvpStatus (*)())m_library.resolve("dvpUninit");
        dvpRefresh = (dvpStatus (*)(dvpUint32*))m_library.resolve("dvpRefresh");
        dvpEnum = (dvpStatus (*)(dvpUint32, dvpCameraInfo*))m_library.resolve("dvpEnum");
        dvpOpen = (dvpStatus (*)(dvpUint32, dvpOpenMode, dvpHandle*))m_library.resolve("dvpOpen");
        dvpOpenByName = (dvpStatus (*)(const char*, dvpOpenMode, dvpHandle*))m_library.resolve("dvpOpenByName");
        dvpClose = (dvpStatus (*)(dvpHandle))m_library.resolve("dvpClose");
        dvpIsValid = (dvpStatus (*)(dvpHandle, bool*))m_library.resolve("dvpIsValid");
        dvpStart = (dvpStatus (*)(dvpHandle))m_library.resolve("dvpStart");
        dvpStop = (dvpStatus (*)(dvpHandle))m_library.resolve("dvpStop");
        dvpGetStreamState = (dvpStatus (*)(dvpHandle, dvpStreamState*))m_library.resolve("dvpGetStreamState");
        dvpGetFrame = (dvpStatus (*)(dvpHandle, dvpFrame*, void**, dvpUint32))m_library.resolve("dvpGetFrame");
        dvpGetFrameBuffer = (dvpStatus (*)(dvpHandle, dvpFrame*, dvpBuffer*, dvpUint32))m_library.resolve("dvpGetFrameBuffer");
        
        // Callbacks
        dvpRegisterStreamCallback = (dvpStatus (*)(dvpHandle, dvpStreamCallback, dvpStreamEvent, void*))m_library.resolve("dvpRegisterStreamCallback");
        dvpUnregisterStreamCallback = (dvpStatus (*)(dvpHandle, dvpStreamCallback, dvpStreamEvent, void*))m_library.resolve("dvpUnregisterStreamCallback");
        
        // Camera control
        dvpGetExposure = (dvpStatus (*)(dvpHandle, float*))m_library.resolve("dvpGetExposure");
        dvpSetExposure = (dvpStatus (*)(dvpHandle, float))m_library.resolve("dvpSetExposure");
        dvpGetAnalogGain = (dvpStatus (*)(dvpHandle, float*))m_library.resolve("dvpGetAnalogGain");
        dvpSetAnalogGain = (dvpStatus (*)(dvpHandle, float))m_library.resolve("dvpSetAnalogGain");
        dvpGetRoi = (dvpStatus (*)(dvpHandle, dvpRegion*))m_library.resolve("dvpGetRoi");
        dvpSetRoi = (dvpStatus (*)(dvpHandle, dvpRegion))m_library.resolve("dvpSetRoi");
        // dvpGetResolution = (dvpStatus (*)(dvpHandle, dvpResolution*))m_library.resolve("dvpGetResolution");
        // dvpSetResolution = (dvpStatus (*)(dvpHandle, dvpResolution))m_library.resolve("dvpSetResolution");
        dvpTriggerFire = (dvpStatus (*)(dvpHandle))m_library.resolve("dvpTriggerFire");
        // dvpGetTriggerState = (dvpStatus (*)(dvpHandle, dvpTriggerState*))m_library.resolve("dvpGetTriggerState");
        // dvpSetTriggerState = (dvpStatus (*)(dvpHandle, dvpTriggerState))m_library.resolve("dvpSetTriggerState");
        // dvpGetTriggerSource = (dvpStatus (*)(dvpHandle, dvpTriggerSource*))m_library.resolve("dvpGetTriggerSource");
        // dvpSetTriggerSource = (dvpStatus (*)(dvpHandle, dvpTriggerSource))m_library.resolve("dvpSetTriggerSource");
        
        // Frame rate
        dvpGetFrameRate = (dvpStatus (*)(dvpHandle, double*))m_library.resolve("dvpGetFrameRate");
        dvpSetFrameRate = (dvpStatus (*)(dvpHandle, double))m_library.resolve("dvpSetFrameRate");
        
        // Frame count
        dvpGetFrameCount = (dvpStatus (*)(dvpHandle, dvpFrameCount*))m_library.resolve("dvpGetFrameCount");
        dvpResetFrameCount = (dvpStatus (*)(dvpHandle))m_library.resolve("dvpResetFrameCount");
        
        // Check if essential functions were loaded
        if (!dvpInit || !dvpRefresh || !dvpEnum || !dvpOpenByName || !dvpClose || 
            !dvpStart || !dvpStop || !dvpGetFrame) {
            qWarning() << "Failed to resolve essential DVP functions";
            return false;
        }
        
        return true;
    }
    
    QLibrary m_library;
    bool m_loaded = false;
};

// Convenience macros to use the functions
#ifdef USE_DYNAMIC_LOADING
    #define DVP_LOADER DvpDynamicLoader::instance()
    #define dvpInit() DVP_LOADER.dvpInit()
    #define dvpUninit() DVP_LOADER.dvpUninit()
    #define dvpRefresh(p) DVP_LOADER.dvpRefresh(p)
    #define dvpEnum(i, p) DVP_LOADER.dvpEnum(i, p)
    #define dvpOpen(i, m, p) DVP_LOADER.dvpOpen(i, m, p)
    #define dvpOpenByName(n, m, p) DVP_LOADER.dvpOpenByName(n, m, p)
    #define dvpClose(h) DVP_LOADER.dvpClose(h)
    #define dvpIsValid(h, p) DVP_LOADER.dvpIsValid(h, p)
    #define dvpStart(h) DVP_LOADER.dvpStart(h)
    #define dvpStop(h) DVP_LOADER.dvpStop(h)
    #define dvpGetStreamState(h, p) DVP_LOADER.dvpGetStreamState(h, p)
    #define dvpGetFrame(h, f, b, t) DVP_LOADER.dvpGetFrame(h, f, b, t)
    #define dvpGetFrameBuffer(h, f, b, t) DVP_LOADER.dvpGetFrameBuffer(h, f, b, t)
    #define dvpRegisterStreamCallback(h, c, e, u) DVP_LOADER.dvpRegisterStreamCallback(h, c, e, u)
    #define dvpUnregisterStreamCallback(h, c, e, u) DVP_LOADER.dvpUnregisterStreamCallback(h, c, e, u)
    #define dvpGetExposure(h, p) DVP_LOADER.dvpGetExposure(h, p)
    #define dvpSetExposure(h, e) DVP_LOADER.dvpSetExposure(h, e)
    #define dvpGetAnalogGain(h, p) DVP_LOADER.dvpGetAnalogGain(h, p)
    #define dvpSetAnalogGain(h, g) DVP_LOADER.dvpSetAnalogGain(h, g)
    #define dvpGetRoi(h, p) DVP_LOADER.dvpGetRoi(h, p)
    #define dvpSetRoi(h, r) DVP_LOADER.dvpSetRoi(h, r)
    // #define dvpGetResolution(h, p) DVP_LOADER.dvpGetResolution(h, p)
    // #define dvpSetResolution(h, r) DVP_LOADER.dvpSetResolution(h, r)
    #define dvpTriggerFire(h) DVP_LOADER.dvpTriggerFire(h)
    // #define dvpGetTriggerState(h, p) DVP_LOADER.dvpGetTriggerState(h, p)
    // #define dvpSetTriggerState(h, s) DVP_LOADER.dvpSetTriggerState(h, s)
    // #define dvpGetTriggerSource(h, p) DVP_LOADER.dvpGetTriggerSource(h, p)
    // #define dvpSetTriggerSource(h, s) DVP_LOADER.dvpSetTriggerSource(h, s)
    #define dvpGetFrameRate(h, p) DVP_LOADER.dvpGetFrameRate(h, p)
    #define dvpSetFrameRate(h, r) DVP_LOADER.dvpSetFrameRate(h, r)
    #define dvpGetFrameCount(h, p) DVP_LOADER.dvpGetFrameCount(h, p)
    #define dvpResetFrameCount(h) DVP_LOADER.dvpResetFrameCount(h)
#endif

#endif // DVP_DYNAMIC_LOADER_H