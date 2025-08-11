#ifndef DVP_SDK_WRAPPER_INTERFACE_H
#define DVP_SDK_WRAPPER_INTERFACE_H

#include "dvp_types.h" // For dvp... types

// This interface abstracts the C-style Do3Think SDK functions,
// allowing for dependency injection and mocking for unit tests.
class IDvpSdkWrapper {
public:
    virtual ~IDvpSdkWrapper() = default;

    virtual dvpStatus dvpRefresh(dvpUint32* puNum) = 0;
    virtual dvpStatus dvpEnum(dvpUint32 i, dvpCameraInfo* pInfo) = 0;
    virtual dvpStatus dvpOpenByName(dvpStr name, dvpOpenMode mode, dvpHandle* pHandle) = 0;
    virtual dvpStatus dvpClose(dvpHandle handle) = 0;
    virtual dvpStatus dvpGetCameraInfo(dvpHandle handle, dvpCameraInfo* pInfo) = 0;

    virtual dvpStatus dvpStart(dvpHandle handle) = 0;
    virtual dvpStatus dvpStop(dvpHandle handle) = 0;

    virtual dvpStatus dvpSetTriggerSource(dvpHandle handle, dvpTriggerSource source) = 0;
    virtual dvpStatus dvpGetTriggerSource(dvpHandle handle, dvpTriggerSource* pSource) = 0;

    virtual dvpStatus dvpSetExposure(dvpHandle handle, double fExposure) = 0;
    virtual dvpStatus dvpGetExposure(dvpHandle handle, double* pfExposure) = 0;

    virtual dvpStatus dvpSetAnalogGain(dvpHandle handle, float fGain) = 0;
    virtual dvpStatus dvpGetAnalogGain(dvpHandle handle, float* pfGain) = 0;

    virtual dvpStatus dvpRegisterStreamCallback(dvpHandle handle, dvpStreamCallback fun, dvpStreamEvent event, void* pContext) = 0;
    virtual dvpStatus dvpUnregisterStreamCallback(dvpHandle handle, dvpStreamCallback fun, dvpStreamEvent event, void* pContext) = 0;

    virtual dvpStatus dvpGetFrame(dvpHandle handle, dvpFrame* pFrame, void** ppBuffer, dvpUint32 uTimeout) = 0;
};

#endif // DVP_SDK_WRAPPER_INTERFACE_H
