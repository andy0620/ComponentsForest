#include "dvp_wrapper.h"
#include <iostream>
#include <string>
#include <sstream>

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

#define DEBUG_LOG(msg) do { \
    std::cerr << "[DVP_WRAPPER] " << msg << std::endl; \
    std::cerr.flush(); \
} while(0)

// PIMPL class to hide implementation details
class DvpSdkWrapper::DvpSdkImpl {
public:
    DvpSdkImpl() {
        load();
    }

    ~DvpSdkImpl() {
        if (m_library) {
            // dvpExit should be called before freeing the library
            if (pfn_dvpExit) {
                pfn_dvpExit();
            }
            FREE_LIBRARY(m_library);
        }
    }

    bool isLoaded() const {
        return m_loaded;
    }

    LibraryHandle m_library = nullptr;
    bool m_loaded = false;

#define DVP_FUNC(name, ...) dvpStatus (*pfn_##name)(__VA_ARGS__) = nullptr
    #include "dvp_function_list.inc"
#undef DVP_FUNC

private:
    void load() {
        if (m_library) {
            return;
        }

    #ifdef _WIN32
        const char* libraryNames[] = {"DVPCamera64.dll", "DVPCamera.dll", nullptr};
    #else
        const char* libraryNames[] = {"libDVPCamera.so", "./libDVPCamera.so", nullptr};
    #endif

        for (int i = 0; libraryNames[i] != nullptr; ++i) {
            m_library = LOAD_LIBRARY(libraryNames[i]);
            if (m_library) {
                break;
            }
        }

        if (!m_library) {
            DEBUG_LOG("ERROR: Failed to load any DVP library");
            m_loaded = false;
            return;
        }

        #define LOAD_FUNC_PTR(name, ...) pfn_##name = (decltype(pfn_##name))GET_FUNCTION(m_library, #name)
        #include "dvp_function_list.inc"
        #undef LOAD_FUNC_PTR

        if (!pfn_dvpRefresh) {
             DEBUG_LOG("ERROR: dvpRefresh function not loaded");
             m_loaded = false;
             return;
        }

        // Initialize the SDK after loading
        if (pfn_dvpInit) {
            pfn_dvpInit();
        }

        m_loaded = true;
    }
};

// ============================================================================
// DvpSdkWrapper Implementation
// ============================================================================

DvpSdkWrapper::DvpSdkWrapper() : m_impl(std::make_unique<DvpSdkImpl>()) {}
DvpSdkWrapper::~DvpSdkWrapper() = default;

std::shared_ptr<IDvpSdkWrapper> DvpSdkWrapper::create() {
    return std::make_shared<DvpSdkWrapper>();
}

bool DvpSdkWrapper::isLoaded() const {
    return m_impl && m_impl->isLoaded();
}

#define CHECK_LOADED(name) \
    if (!isLoaded() || !m_impl->pfn_##name) { \
        return DVP_STATUS_NOT_INITIALIZED; \
    }

#define WRAP_DVP_FUNC(name, ...) \
dvpStatus DvpSdkWrapper::name(__VA_ARGS__) { \
    CHECK_LOADED(name); \
    return m_impl->pfn_##name(FORWARD_ARGS(__VA_ARGS__)); \
}

// Helper macros for forwarding arguments
#define GET_ARG_1(a1, ...) a1
#define GET_ARG_2(a1, a2, ...) a2
#define GET_ARG_3(a1, a2, a3, ...) a3
#define GET_ARG_4(a1, a2, a3, a4, ...) a4
#define GET_ARG_5(a1, a2, a3, a4, a5, ...) a5

#define FORWARD_ARGS(...) GET_ARG_1(__VA_ARGS__,), GET_ARG_2(__VA_ARGS__,), GET_ARG_3(__VA_ARGS__,), GET_ARG_4(__VA_ARGS__,), GET_ARG_5(__VA_ARGS__,)


// This is getting too complex for a simple replace.
// The following is a manual implementation of the functions.
// In a real scenario, a more advanced macro would be used.

dvpStatus DvpSdkWrapper::dvpRefresh(dvpUint32* puNum) {
    CHECK_LOADED(dvpRefresh);
    return m_impl->pfn_dvpRefresh(puNum);
}

dvpStatus DvpSdkWrapper::dvpEnum(dvpUint32 i, dvpCameraInfo* pInfo) {
    CHECK_LOADED(dvpEnum);
    return m_impl->pfn_dvpEnum(i, pInfo);
}

dvpStatus DvpSdkWrapper::dvpOpenByName(dvpStr name, dvpOpenMode mode, dvpHandle* pHandle) {
    CHECK_LOADED(dvpOpenByName);
    return m_impl->pfn_dvpOpenByName(name, mode, pHandle);
}

dvpStatus DvpSdkWrapper::dvpClose(dvpHandle handle) {
    CHECK_LOADED(dvpClose);
    return m_impl->pfn_dvpClose(handle);
}

dvpStatus DvpSdkWrapper::dvpGetCameraInfo(dvpHandle handle, dvpCameraInfo* pInfo) {
    CHECK_LOADED(dvpGetCameraInfo);
    return m_impl->pfn_dvpGetCameraInfo(handle, pInfo);
}

dvpStatus DvpSdkWrapper::dvpStart(dvpHandle handle) {
    CHECK_LOADED(dvpStart);
    return m_impl->pfn_dvpStart(handle);
}

dvpStatus DvpSdkWrapper::dvpStop(dvpHandle handle) {
    CHECK_LOADED(dvpStop);
    return m_impl->pfn_dvpStop(handle);
}

dvpStatus DvpSdkWrapper::dvpSetTriggerSource(dvpHandle handle, dvpTriggerSource source) {
    CHECK_LOADED(dvpSetTriggerSource);
    return m_impl->pfn_dvpSetTriggerSource(handle, source);
}

dvpStatus DvpSdkWrapper::dvpGetTriggerSource(dvpHandle handle, dvpTriggerSource* pSource) {
    CHECK_LOADED(dvpGetTriggerSource);
    return m_impl->pfn_dvpGetTriggerSource(handle, pSource);
}

dvpStatus DvpSdkWrapper::dvpSetExposure(dvpHandle handle, double fExposure) {
    CHECK_LOADED(dvpSetExposure);
    return m_impl->pfn_dvpSetExposure(handle, fExposure);
}

dvpStatus DvpSdkWrapper::dvpGetExposure(dvpHandle handle, double* pfExposure) {
    CHECK_LOADED(dvpGetExposure);
    return m_impl->pfn_dvpGetExposure(handle, pfExposure);
}

dvpStatus DvpSdkWrapper::dvpSetAnalogGain(dvpHandle handle, float fGain) {
    CHECK_LOADED(dvpSetAnalogGain);
    return m_impl->pfn_dvpSetAnalogGain(handle, fGain);
}

dvpStatus DvpSdkWrapper::dvpGetAnalogGain(dvpHandle handle, float* pfGain) {
    CHECK_LOADED(dvpGetAnalogGain);
    return m_impl->pfn_dvpGetAnalogGain(handle, pfGain);
}

dvpStatus DvpSdkWrapper::dvpRegisterStreamCallback(dvpHandle handle, dvpStreamCallback fun, dvpStreamEvent event, void* pContext) {
    CHECK_LOADED(dvpRegisterStreamCallback);
    return m_impl->pfn_dvpRegisterStreamCallback(handle, fun, event, pContext);
}

dvpStatus DvpSdkWrapper::dvpUnregisterStreamCallback(dvpHandle handle, dvpStreamCallback fun, dvpStreamEvent event, void* pContext) {
    CHECK_LOADED(dvpUnregisterStreamCallback);
    return m_impl->pfn_dvpUnregisterStreamCallback(handle, fun, event, pContext);
}

dvpStatus DvpSdkWrapper::dvpGetFrame(dvpHandle handle, dvpFrame* pFrame, void** ppBuffer, dvpUint32 uTimeout) {
    CHECK_LOADED(dvpGetFrame);
    return m_impl->pfn_dvpGetFrame(handle, pFrame, ppBuffer, uTimeout);
}