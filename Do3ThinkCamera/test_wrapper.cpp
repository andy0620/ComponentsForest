/**
 * @file test_wrapper.cpp
 * @brief Simple test to verify DVP wrapper compilation and basic functionality
 */

#include "dvp_wrapper.h"
#include <cstdio>
#include <cstring>

int main() {
    printf("Testing DVP Wrapper...\n");
    
    // Test initialization
    dvpStatus status = dvpInit();
    printf("dvpInit() returned: %d (DVP_STATUS_OK = %d)\n", status, DVP_STATUS_OK);
    
    // Test device enumeration
    dvpUint32 deviceCount = 0;
    status = dvpRefresh(&deviceCount);
    printf("dvpRefresh() returned: %d, device count: %u\n", status, deviceCount);
    
    // Test camera info structure
    dvpCameraInfo info;
    memset(&info, 0, sizeof(info));
    
    if (deviceCount > 0) {
        status = dvpEnum(0, &info);
        printf("dvpEnum(0) returned: %d\n", status);
        if (status == DVP_STATUS_OK) {
            printf("Camera Model: %s\n", info.Model);
            printf("Manufacturer: %s\n", info.Manufacturer);
        }
    }
    
    // Test invalid handle operations (should fail gracefully)
    dvpHandle invalidHandle = 0;
    bool isValid = false;
    status = dvpIsValid(invalidHandle, &isValid);
    printf("dvpIsValid(0) returned: %d, isValid: %s\n", status, isValid ? "true" : "false");
    
    // Test exposure descriptor
    dvpDoubleDescr exposureDescr;
    memset(&exposureDescr, 0, sizeof(exposureDescr));
    status = dvpGetExposureDescr(invalidHandle, &exposureDescr);
    printf("dvpGetExposureDescr() returned: %d\n", status);
    if (status == DVP_STATUS_OK) {
        printf("Exposure range: %.1f - %.1f us, default: %.1f us\n",
               exposureDescr.fMin, exposureDescr.fMax, exposureDescr.fDefault);
    }
    
    // Test cleanup
    status = dvpExit();
    printf("dvpExit() returned: %d\n", status);
    
#ifdef USE_DYNAMIC_LOADING
    printf("\nWrapper mode: DYNAMIC LOADING\n");
#else
    printf("\nWrapper mode: STUB IMPLEMENTATION\n");
#endif
    
    printf("\nTest completed successfully!\n");
    return 0;
}