#ifndef DVP_SDK_WRAPPER_H
#define DVP_SDK_WRAPPER_H

#include "dvp_sdk_wrapper_interface.h"
#include <memory>

// All the C-style typedefs and enums from the original dvp_wrapper.h
// should be moved to a separate header, e.g., "dvp_types.h"
// For now, we assume dvp_sdk_wrapper_interface.h includes it.

// Concrete implementation of the DVP SDK wrapper that uses dynamic loading.
class DvpSdkWrapper : public IDvpSdkWrapper {
public:
    DvpSdkWrapper();
    virtual ~DvpSdkWrapper() override;

    // Factory method to create an instance
    static std::shared_ptr<IDvpSdkWrapper> create();

    // Disable copy and assign
    DvpSdkWrapper(const DvpSdkWrapper&) = delete;
    DvpSdkWrapper& operator=(const DvpSdkWrapper&) = delete;

    // Interface implementation
    dvpStatus dvpRefresh(dvpUint32* puNum) override;
    dvpStatus dvpEnum(dvpUint32 i, dvpCameraInfo* pInfo) override;
    dvpStatus dvpOpenByName(dvpStr name, dvpOpenMode mode, dvpHandle* pHandle) override;
    dvpStatus dvpClose(dvpHandle handle) override;
    dvpStatus dvpGetCameraInfo(dvpHandle handle, dvpCameraInfo* pInfo) override;
    dvpStatus dvpStart(dvpHandle handle) override;
    dvpStatus dvpStop(dvpHandle handle) override;
    dvpStatus dvpSetTriggerSource(dvpHandle handle, dvpTriggerSource source) override;
    dvpStatus dvpGetTriggerSource(dvpHandle handle, dvpTriggerSource* pSource) override;
    dvpStatus dvpSetExposure(dvpHandle handle, double fExposure) override;
    dvpStatus dvpGetExposure(dvpHandle handle, double* pfExposure) override;
    dvpStatus dvpSetAnalogGain(dvpHandle handle, float fGain) override;
    dvpStatus dvpGetAnalogGain(dvpHandle handle, float* pfGain) override;
    dvpStatus dvpRegisterStreamCallback(dvpHandle handle, dvpStreamCallback fun, dvpStreamEvent event, void* pContext) override;
    dvpStatus dvpUnregisterStreamCallback(dvpHandle handle, dvpStreamCallback fun, dvpStreamEvent event, void* pContext) override;
    dvpStatus dvpGetFrame(dvpHandle handle, dvpFrame* pFrame, void** ppBuffer, dvpUint32 uTimeout) override;

private:
    // PIMPL idiom to hide implementation details (like the library handle and function pointers)
    class DvpSdkImpl;
    std::unique_ptr<DvpSdkImpl> m_impl;

    bool isLoaded() const;
};

#endif // DVP_SDK_WRAPPER_H