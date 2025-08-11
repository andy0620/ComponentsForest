/**
 * @file dvp_wrapper.h
 * @brief DVP Camera API Wrapper Header
 * 
 * This header provides all necessary DVP types and function declarations
 * without including the problematic DVPCamera.h header directly.
 * Supports both stub mode and dynamic loading mode.
 */

#ifndef DVP_WRAPPER_H
#define DVP_WRAPPER_H

#include <stdint.h>

#ifdef _WIN32
    #include <windows.h>
#endif

// ============================================================================
// Basic Type Definitions
// ============================================================================

// Platform-specific basic types
#ifdef _WIN32
    typedef unsigned char BYTE;
    typedef int INT32;
    typedef unsigned int UINT32;
    typedef uint8_t dvpByte;
    typedef int32_t dvpInt32;
    typedef uint32_t dvpUint32;
    typedef float dvpFloat32;
    typedef double dvpFloat64;
#else
    typedef uint8_t dvpByte;
    typedef int32_t dvpInt32;
    typedef uint32_t dvpUint32;
    typedef float dvpFloat32;
    typedef double dvpFloat64;
#endif

// Handle types
typedef dvpUint32 dvpHandle;
typedef dvpUint32 dvpRecordHandle;

// Compound types
typedef dvpUint32 dvpReserved[32];
typedef char dvpChar[64];
typedef char dvpString16[16];
typedef char dvpString32[32];
typedef char dvpString64[64];
typedef char dvpString128[128];
typedef char dvpString256[256];
typedef bool dvpBool32[32];
typedef dvpUint32 dvpSize3c[3];

// ============================================================================
// Enumeration Definitions
// ============================================================================

/** @brief Status codes */
typedef enum dvpStatus {
    DVP_STATUS_IGNORED = 7,
    DVP_STATUS_NEED_OTHER = 6,
    DVP_STATUS_NEXT_STAGE = 5,
    DVP_STATUS_BUSY = 4,
    DVP_STATUS_WAIT = 3,
    DVP_STATUS_IN_PROCESS = 2,
    DVP_STATUS_OK = 0,
    DVP_STATUS_FAILED = -1,
    DVP_STATUS_UNKNOW = -2,
    DVP_STATUS_NOT_SUPPORTED = -3,
    DVP_STATUS_NOT_INITIALIZED = -4,
    DVP_STATUS_PARAMETER_INVALID = -5,
    DVP_STATUS_PARAMETER_OUT_OF_BOUND = -6,
    DVP_STATUS_UNENABLED = -7,
    DVP_STATUS_UNCONNECTED = -8,
    DVP_STATUS_NOT_VALID = -9,
    DVP_STATUS_UNPLAY = -10,
    DVP_STATUS_NOT_STARTED = -11,
    DVP_STATUS_NOT_STOPPED = -12,
    DVP_STATUS_NOT_READY = -13,
    DVP_STATUS_INVALID_HANDLE = -14,
    DVP_STATUS_INVALID_INDEX = -15,
    DVP_STATUS_INVALID_ADDRESS = -16,
    DVP_STATUS_FILE_ERROR = -17,
    DVP_STATUS_INSUFFICIENT_MEMORY = -18,
    DVP_STATUS_FUNCTION_INVALID = -19,
    DVP_STATUS_DEVICE_DISCONNECTED = -20,
    DVP_STATUS_DEVICE_IS_DISCONNECTED = -20, // Alias for compatibility
    DVP_STATUS_FORMAT_INVALID = -21,
    DVP_STATUS_TIMEOUT = -22,
    DVP_STATUS_TIME_OUT = -22, // Alias for compatibility
    DVP_STATUS_DENIED = -30,
    DVP_STATUS_NO_DEVICE_FOUND = -31,
    DVP_STATUS_OVER_LOAD = -32,
    DVP_STATUS_GRAB_FAILED = -23,
    DVP_STATUS_FRAME_LOST = -24,
    DVP_STATUS_SIZE_DISMATCH = -25,
    DVP_STATUS_CANCELED = -26
} dvpStatus;

/** @brief Image format */
typedef enum dvpImageFormat {
    FORMAT_MONO = 0,
    FORMAT_BAYER_BG = 1,
    FORMAT_BAYER_GB = 2,
    FORMAT_BAYER_GR = 3,
    FORMAT_BAYER_RG = 4,
    FORMAT_BGR24 = 10,
    FORMAT_BGR32 = 11,
    FORMAT_BGR48 = 12,
    FORMAT_BGR64 = 13,
    FORMAT_RGB24 = 14,
    FORMAT_RGB32 = 15,
    FORMAT_RGB48 = 16,
    FORMAT_YUV411 = 20,
    FORMAT_YUV422 = 21,
    FORMAT_YUV444 = 22,
    FORMAT_YUV420 = 23,
    FORMAT_B8_G8_R8 = 40,
    FORMAT_B16_G16_R16 = 44
} dvpImageFormat;

/** @brief Stream format */
typedef enum dvpStreamFormat {
    S_RAW8 = 0,
    S_RAW10 = 1,
    S_RAW12 = 2,
    S_RAW14 = 3,
    S_RAW16 = 4,
    S_BGR24 = 10,
    S_BGR32 = 11,
    S_BGR48 = 12,
    S_BGR64 = 13,
    S_RGB24 = 14,
    S_RGB32 = 15,
    S_RGB48 = 16,
    S_RGB64 = 17,
    S_YUV411 = 20,
    S_YUV422 = 21,
    S_YUV444 = 22,
    S_YUV420 = 23,
    S_MONO8 = 30,
    S_MONO10 = 31,
    S_MONO12 = 32,
    S_MONO14 = 33,
    S_MONO16 = 34,
    S_B8_G8_R8 = 40,
    S_B16_G16_R16 = 44
} dvpStreamFormat;

/** @brief Bits depth */
typedef enum dvpBits {
    BITS_8 = 0,
    BITS_10 = 1,
    BITS_12 = 2,
    BITS_14 = 3,
    BITS_16 = 4
} dvpBits;

/** @brief Auto exposure mode */
typedef enum dvpAeMode {
    AE_MODE_AE_AG = 0,
    AE_MODE_AG_AE = 1,
    AE_MODE_AE_ONLY = 2,
    AE_MODE_AG_ONLY = 3
} dvpAeMode;

/** @brief Auto exposure operation */
typedef enum dvpAeOperation {
    AE_OP_OFF = 0,
    AE_OP_ONCE = 1,
    AE_OP_CONTINUOUS = 2
} dvpAeOperation;

/** @brief Auto white balance operation */
typedef enum dvpAwbOperation {
    AWB_OP_OFF = 0,
    AWB_OP_ONCE = 1,
    AWB_OP_CONTINUOUS = 2
} dvpAwbOperation;

/** @brief Stream state */
typedef enum dvpStreamState {
    STATE_STOPED = 0,
    STATE_STARTED = 2
} dvpStreamState;

/** @brief Stream event */
typedef enum dvpStreamEvent {
    STREAM_EVENT_ARRIVED = 0,
    STREAM_EVENT_CORRECTED = 1,
    STREAM_EVENT_PROCESSED = 2,
    STREAM_EVENT_FRAME_THREAD = 3
} dvpStreamEvent;

/** @brief Event types */
typedef enum dvpEvent {
    EVENT_UNKNOW = 0,
    EVENT_CONNECTED = 1,
    EVENT_DISCONNECTED = 2,
    EVENT_STREAM_STARTRD = 3,
    EVENT_STREAM_STOPPED = 4,
    EVENT_FRAME_LOST = 5,
    EVENT_FRAME_TIMEOUT = 6,
    EVENT_LOST_CONNECTION = 7,
    EVENT_RECONNECTED = 8,
    EVENT_FRAME_START = 9,
    EVENT_FRAME_END = 10
} dvpEvent;

/** @brief Trigger source */
typedef enum dvpTriggerSource {
    TRIGGER_SOURCE_SOFTWARE = 0,
    TRIGGER_SOURCE_LINE1 = 1,
    TRIGGER_SOURCE_LINE2 = 2,
    TRIGGER_SOURCE_LINE3 = 3,
    TRIGGER_SOURCE_LINE4 = 4
} dvpTriggerSource;

/** @brief Trigger input type */
typedef enum dvpTriggerInputType {
    TRIGGER_IN_OFF = 0,
    TRIGGER_NEG_EDGE = 1,
    TRIGGER_LOW_LEVEL = 2,
    TRIGGER_POS_EDGE = 3,
    TRIGGER_HIGH_LEVEL = 4
} dvpTriggerInputType;

/** @brief Open mode */
typedef enum dvpOpenMode {
    OPEN_OFFLINE = 0,
    OPEN_NORMAL = 1 << 0,
    OPEN_DEBUG = 1 << 3,
    HIGH_PRIORITY = 1 << 4,
    OPEN_AUTOIP = 1 << 5
} dvpOpenMode;

/** @brief Sensor type */
typedef enum dvpSensorType {
    SENSOR_TYPE_CMOS = 0,
    SENSOR_TYPE_CCD = 1
} dvpSensorType;

/** @brief Sensor pixel type */
typedef enum dvpSensorPixel {
    SENSOR_PIXEL_MONO = 0,
    SENSOR_PIXEL_BAYER_RG = 1,
    SENSOR_PIXEL_BAYER_GB = 2,
    SENSOR_PIXEL_BAYER_GR = 3,
    SENSOR_PIXEL_BAYER_BG = 4
} dvpSensorPixel;

/** @brief Buffer mode for frame acquisition */
typedef enum dvpBufferMode {
    BUFFER_MODE_FIFO = 0,
    BUFFER_MODE_NEWEST = 1
} dvpBufferMode;

/** @brief Output IO function */
typedef enum dvpOutputIoFunction {
    OUTPUT_FUNCTION_NORMAL = 0x0000,
    OUTPUT_FUNCTION_STROBE = 0x0001
} dvpOutputIoFunction;

/** @brief Output IO pins */
typedef enum dvpOutputIo {
    OUTPUT_IO_1 = 0x0001,
    OUTPUT_IO_2 = 0x0002,
    OUTPUT_IO_3 = 0x0003,
    OUTPUT_IO_4 = 0x0004
} dvpOutputIo;

/** @brief Input IO pins */
typedef enum dvpInputIo {
    INPUT_IO_1 = 0x8001,
    INPUT_IO_2 = 0x8002,
    INPUT_IO_3 = 0x8003,
    INPUT_IO_4 = 0x8004
} dvpInputIo;

// ============================================================================
// Structure Definitions
// ============================================================================

/** @brief Camera information */
typedef struct dvpCameraInfo {
    dvpString64 Vendor;
    dvpString64 Manufacturer;
    dvpString64 Model;
    dvpString64 Family;
    dvpString64 LinkName;
    dvpString32 SensorInfo;
    dvpString32 HardwareVersion;
    dvpString32 FirmwareVersion;
    dvpString32 PackageVersion;
    dvpString64 FriendlyName;
    dvpString16 PortInfo;
    dvpString128 SerialNumber;
    dvpString128 CameraGuid;
    dvpString128 UserID;
    dvpString32 KernelVersion;
    dvpString32 DscamVersion;
    dvpString32 CameraInfo;
    dvpReserved reserved;
} dvpCameraInfo;

/** @brief Region (ROI) */
typedef struct dvpRegion {
    dvpInt32 X;
    dvpInt32 Y;
    dvpInt32 W;
    dvpInt32 H;
    dvpReserved reserved;
} dvpRegion;

/** @brief Region descriptor */
typedef struct dvpRegionDescr {
    dvpInt32 iMinW;
    dvpInt32 iMinH;
    dvpInt32 iMaxW;
    dvpInt32 iMaxH;
    dvpInt32 iStepW;
    dvpInt32 iStepH;
    dvpReserved reserved;
} dvpRegionDescr;

/** @brief Frame information */
typedef struct dvpFrame {
    enum dvpImageFormat format;
    enum dvpBits bits;
    dvpUint32 uBytes;
    dvpInt32 iWidth;
    dvpInt32 iHeight;
    dvpUint32 uFrameID;
    dvpUint32 uTimestamp;
    dvpUint32 uExposure;
    float fAGain;
    float fExposure;  // Added for compatibility
    dvpInt32 iHorizontalOffset;
    dvpInt32 iVerticalOffset;
    dvpUint32 uFrameStatus;
    bool bFlipHorizontal;
    bool bFlipVertical;
    dvpInt32 iRotate;
    dvpReserved reserved;
} dvpFrame;

/** @brief Frame buffer */
typedef struct dvpFrameBuffer {
    struct dvpFrame frame;
    dvpByte* pBuffer;
} dvpFrameBuffer;

/** @brief Frame count statistics */
typedef struct dvpFrameCount {
    dvpUint32 uFrameCount;
    dvpUint32 uFrameDrop;
    dvpUint32 uFrameIgnore;
    dvpUint32 uFrameError;
    dvpUint32 uFrameOK;
    dvpReserved reserved;
} dvpFrameCount;

/** @brief Sensor information */
typedef struct dvpSensorInfo {
    dvpString64 descr;
    enum dvpSensorType sensor;
    enum dvpSensorPixel pixel;
    struct dvpRegionDescr region;
    dvpReserved reserved;
} dvpSensorInfo;

/** @brief Resolution mode */
typedef struct dvpResolutionMode {
    dvpInt32 iWidth;
    dvpInt32 iHeight;
    dvpString64 selection;
    struct dvpRegion roi;
    dvpReserved reserved;
} dvpResolutionMode;

/** @brief Temperature info */
typedef struct dvpTemperatureInfo {
    dvpFloat32 fDevice;
    dvpFloat32 fSensor;
    dvpFloat32 fChip;
    dvpReserved reserved;
} dvpTemperatureInfo;

/** @brief Float descriptor */
typedef struct dvpFloatDescr {
    dvpFloat32 fMin;
    dvpFloat32 fMax;
    dvpFloat32 fStep;
    dvpFloat32 fDefault;
    bool bIsValid;
    dvpReserved reserved;
} dvpFloatDescr;

/** @brief Double descriptor */
typedef struct dvpDoubleDescr {
    dvpFloat64 fMin;
    dvpFloat64 fMax;
    dvpFloat64 fStep;
    dvpFloat64 fDefault;
    bool bIsValid;
    dvpReserved reserved;
} dvpDoubleDescr;

/** @brief Int descriptor */
typedef struct dvpIntDescr {
    dvpInt32 iMin;
    dvpInt32 iMax;
    dvpInt32 iStep;
    dvpInt32 iDefault;
    bool bIsValid;
    dvpReserved reserved;
} dvpIntDescr;

/** @brief Uint descriptor */
typedef struct dvpUintDescr {
    dvpUint32 uMin;
    dvpUint32 uMax;
    dvpUint32 uStep;
    dvpUint32 uDefault;
    bool bIsValid;
    dvpReserved reserved;
} dvpUintDescr;

/** @brief Selection descriptor */
typedef struct dvpSelectionDescr {
    dvpUint32 uCount;
    dvpUint32 uDefault;
    bool bIsValid;
    dvpReserved reserved;
} dvpSelectionDescr;

/** @brief Color correction */
typedef struct dvpColorCorrection {
    float fMatrix[3][3];
    struct {
        float red;
        float green;
        float blue;
    } bgr;
    dvpReserved reserved;
} dvpColorCorrection;

/** @brief Variant data */
typedef struct dvpVariant {
    union {
        dvpInt32 i32;
        dvpUint32 u32;
        dvpFloat32 f32;
        dvpFloat64 f64;
        void* ptr;
    } data;
    dvpReserved reserved;
} dvpVariant;

// ============================================================================
// Callback Function Types
// ============================================================================

/** @brief Stream callback function type */
typedef dvpInt32(*dvpStreamCallback)(dvpHandle handle, dvpStreamEvent event, void* pContext, dvpFrame* pFrame, void* pBuffer);

/** @brief Event callback function type */
typedef dvpInt32(*dvpEventCallback)(dvpHandle handle, dvpEvent event, void* pContext, dvpInt32 param, struct dvpVariant* pVariant);

// Line enum - needed before function declarations
typedef enum dvpLine {
    LINE_1 = 0x10001,
    LINE_2 = 0x10002,
    LINE_3 = 0x10003,
    LINE_4 = 0x10004
} dvpLine;

// ============================================================================
// Function Declarations
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// Core initialization and cleanup
dvpStatus dvpInit();
dvpStatus dvpExit();
dvpStatus dvpRefresh(dvpUint32* pCount);
dvpStatus dvpEnum(dvpUint32 index, dvpCameraInfo* pInfo);

// Device operations
dvpStatus dvpOpen(dvpUint32 index, dvpOpenMode mode, dvpHandle* pHandle);
dvpStatus dvpOpenByName(const char* name, dvpOpenMode mode, dvpHandle* pHandle);
dvpStatus dvpClose(dvpHandle handle);
dvpStatus dvpIsValid(dvpHandle handle, bool* pIsValid);

// Stream control
dvpStatus dvpStart(dvpHandle handle);
dvpStatus dvpStop(dvpHandle handle);
dvpStatus dvpGetStreamState(dvpHandle handle, dvpStreamState* pState);

// Image acquisition
dvpStatus dvpGetFrame(dvpHandle handle, dvpFrame* pFrame, void** pBuffer, dvpUint32 timeout);
dvpStatus dvpGetFrameBuffer(dvpHandle handle, dvpFrameBuffer* pFrameBuffer, dvpUint32 timeout);

// Callback registration
dvpStatus dvpRegisterStreamCallback(dvpHandle handle, dvpStreamCallback callback, dvpStreamEvent event, void* pContext);
dvpStatus dvpUnregisterStreamCallback(dvpHandle handle, dvpStreamCallback callback, dvpStreamEvent event, void* pContext);

// Camera information
dvpStatus dvpGetCameraInfo(dvpHandle handle, dvpCameraInfo* pInfo);
dvpStatus dvpGetSensorInfo(dvpHandle handle, dvpSensorInfo* pInfo);

// Exposure control
dvpStatus dvpGetExposure(dvpHandle handle, dvpFloat64* pExposure);
dvpStatus dvpSetExposure(dvpHandle handle, dvpFloat64 exposure);
dvpStatus dvpGetExposureDescr(dvpHandle handle, dvpDoubleDescr* pDescr);

// Gain control
dvpStatus dvpGetAnalogGain(dvpHandle handle, float* pGain);
dvpStatus dvpSetAnalogGain(dvpHandle handle, float gain);
dvpStatus dvpGetAnalogGainDescr(dvpHandle handle, dvpFloatDescr* pDescr);

// ROI control
dvpStatus dvpGetRoi(dvpHandle handle, dvpRegion* pRoi);
dvpStatus dvpSetRoi(dvpHandle handle, dvpRegion roi);
dvpStatus dvpGetRoiDescr(dvpHandle handle, dvpRegionDescr* pDescr);

// Auto exposure control
dvpStatus dvpGetAeMode(dvpHandle handle, dvpAeMode* pMode);
dvpStatus dvpSetAeMode(dvpHandle handle, dvpAeMode mode);
dvpStatus dvpGetAeOperation(dvpHandle handle, dvpAeOperation* pOperation);
dvpStatus dvpSetAeOperation(dvpHandle handle, dvpAeOperation operation);

// Auto white balance control
dvpStatus dvpGetAwbOperation(dvpHandle handle, dvpAwbOperation* pOperation);
dvpStatus dvpSetAwbOperation(dvpHandle handle, dvpAwbOperation operation);

// Color control
dvpStatus dvpGetRgbGain(dvpHandle handle, float* pRGain, float* pGGain, float* pBGain);
dvpStatus dvpSetRgbGain(dvpHandle handle, float rGain, float gGain, float bGain);

// Gamma control
dvpStatus dvpGetGamma(dvpHandle handle, dvpInt32* pGamma);
dvpStatus dvpSetGamma(dvpHandle handle, dvpInt32 gamma);

// Format control
dvpStatus dvpGetTargetFormat(dvpHandle handle, dvpStreamFormat* pFormat);
dvpStatus dvpSetTargetFormat(dvpHandle handle, dvpStreamFormat format);

// Resolution modes
dvpStatus dvpGetResolutionModeSel(dvpHandle handle, dvpUint32* pIndex);
dvpStatus dvpSetResolutionModeSel(dvpHandle handle, dvpUint32 index);
dvpStatus dvpGetResolutionModeSelDescr(dvpHandle handle, dvpSelectionDescr* pDescr);
dvpStatus dvpGetResolutionModeSelDetail(dvpHandle handle, dvpUint32 index, dvpResolutionMode* pMode);

// Trigger control
dvpStatus dvpGetTriggerSource(dvpHandle handle, dvpTriggerSource* pSource);
dvpStatus dvpSetTriggerSource(dvpHandle handle, dvpTriggerSource source);
dvpStatus dvpGetTriggerInputType(dvpHandle handle, dvpLine line, dvpTriggerInputType* pType);
dvpStatus dvpSetTriggerInputType(dvpHandle handle, dvpTriggerInputType type);
dvpStatus dvpTriggerFire(dvpHandle handle);
dvpStatus dvpSetTriggerDelay(dvpHandle handle, dvpFloat64 delay);

// Buffer management
dvpStatus dvpGetBufferQueueSize(dvpHandle handle, dvpUint32* pSize);
dvpStatus dvpSetBufferQueueSize(dvpHandle handle, dvpUint32 size);

// Frame statistics
dvpStatus dvpGetFrameCount(dvpHandle handle, dvpFrameCount* pCount);

// Temperature
dvpStatus dvpGetTemperatureInfo(dvpHandle handle, dvpTemperatureInfo* pInfo);

// Color correction
dvpStatus dvpGetColorCorrection(dvpHandle handle, dvpColorCorrection* pCorrection);
dvpStatus dvpSetColorCorrection(dvpHandle handle, dvpColorCorrection correction);
dvpStatus dvpGetColorSolutionSel(dvpHandle handle, dvpUint32* pIndex);
dvpStatus dvpSetColorSolutionSel(dvpHandle handle, dvpUint32 index);
dvpStatus dvpGetColorSolutionSelDescr(dvpHandle handle, dvpSelectionDescr* pDescr);

// Hardware ISP
dvpStatus dvpGetHardwareIspState(dvpHandle handle, bool* pState);
dvpStatus dvpSetHardwareIspState(dvpHandle handle, bool state);

// Configuration save/load
dvpStatus dvpSaveConfig(dvpHandle handle, const char* path);
dvpStatus dvpLoadConfig(dvpHandle handle, const char* path);

// User data
dvpStatus dvpReadUserData(dvpHandle handle, dvpUint32 offset, void* pBuffer, dvpUint32 size);
dvpStatus dvpWriteUserData(dvpHandle handle, dvpUint32 offset, const void* pBuffer, dvpUint32 size);

// GPIO control
dvpStatus dvpGetOutputIoLevel(dvpHandle handle, dvpOutputIo io, bool* pLevel);
dvpStatus dvpSetOutputIoLevel(dvpHandle handle, dvpOutputIo io, bool level);
dvpStatus dvpGetInputIoLevel(dvpHandle handle, dvpInputIo io, bool* pLevel);
dvpStatus dvpSetOutputIoFunction(dvpHandle handle, dvpOutputIo io, dvpOutputIoFunction func);

// Packet size (for network cameras)
dvpStatus dvpGetPacketSizeSel(dvpHandle handle, dvpUint32* pSize);
dvpStatus dvpSetPacketSizeSel(dvpHandle handle, dvpUint32 size);

#ifdef __cplusplus
}
#endif

// ============================================================================
// Wrapper Implementation Selection
// ============================================================================

// The actual implementation is in dvp_wrapper.cpp
// It will either use stub functions or dynamic loading based on USE_DYNAMIC_LOADING

#endif // DVP_WRAPPER_H