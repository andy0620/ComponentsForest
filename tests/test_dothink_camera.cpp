#include <gtest/gtest.h>
#include <gmock/gmock.hh>
#include <QApplication>
#include <memory>

#include "Do3ThinkCamera/dothink_camera.h"
#include "Do3ThinkCamera/dvp_sdk_wrapper_interface.h"

using namespace ComponentsForest;
using namespace testing;

// 1. Create the Mock Object for the SDK Wrapper
class MockDvpSdkWrapper : public IDvpSdkWrapper {
public:
    MOCK_METHOD(dvpStatus, dvpRefresh, (dvpUint32* puNum), (override));
    MOCK_METHOD(dvpStatus, dvpEnum, (dvpUint32 i, dvpCameraInfo* pInfo), (override));
    MOCK_METHOD(dvpStatus, dvpOpenByName, (dvpStr name, dvpOpenMode mode, dvpHandle* pHandle), (override));
    MOCK_METHOD(dvpStatus, dvpClose, (dvpHandle handle), (override));
    MOCK_METHOD(dvpStatus, dvpGetCameraInfo, (dvpHandle handle, dvpCameraInfo* pInfo), (override));
    MOCK_METHOD(dvpStatus, dvpStart, (dvpHandle handle), (override));
    MOCK_METHOD(dvpStatus, dvpStop, (dvpHandle handle), (override));
    MOCK_METHOD(dvpStatus, dvpSetTriggerSource, (dvpHandle handle, dvpTriggerSource source), (override));
    MOCK_METHOD(dvpStatus, dvpGetTriggerSource, (dvpHandle handle, dvpTriggerSource* pSource), (override));
    MOCK_METHOD(dvpStatus, dvpSetExposure, (dvpHandle handle, double fExposure), (override));
    MOCK_METHOD(dvpStatus, dvpGetExposure, (dvpHandle handle, double* pfExposure), (override));
    MOCK_METHOD(dvpStatus, dvpSetAnalogGain, (dvpHandle handle, float fGain), (override));
    MOCK_METHOD(dvpStatus, dvpGetAnalogGain, (dvpHandle handle, float* pfGain), (override));
    MOCK_METHOD(dvpStatus, dvpRegisterStreamCallback, (dvpHandle handle, dvpStreamCallback fun, dvpStreamEvent event, void* pContext), (override));
    MOCK_METHOD(dvpStatus, dvpUnregisterStreamCallback, (dvpHandle handle, dvpStreamCallback fun, dvpStreamEvent event, void* pContext), (override));
    MOCK_METHOD(dvpStatus, dvpGetFrame, (dvpHandle handle, dvpFrame* pFrame, void** ppBuffer, dvpUint32 uTimeout), (override));
};

// 2. Create the Test Fixture
class Do3ThinkCameraComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!QApplication::instance()) {
            static int argc = 0;
            app = new QApplication(argc, nullptr);
        }
        // Create a mock wrapper for each test
        mockWrapper = std::make_shared<StrictMock<MockDvpSdkWrapper>>();
    }

    void TearDown() override {
        // The component will be deleted by the test case
    }

    std::shared_ptr<StrictMock<MockDvpSdkWrapper>> mockWrapper;
    static QApplication* app;
};

QApplication* Do3ThinkCameraComponentTest::app = nullptr;

// 3. Write the Test Case
TEST_F(Do3ThinkCameraComponentTest, ConnectCamera_SuccessPath) {
    // Arrange: Define the expected behavior of the mock SDK

    // Expect dvpOpenByName to be called once and return OK.
    // Use WillOnce(Return(DVP_STATUS_OK)) for the return value.
    // The handle pointer will be set to a dummy value.
    EXPECT_CALL(*mockWrapper, dvpOpenByName(_, _, _))
        .WillOnce(DoAll(
            SetArgPointee<2>(dvpHandle(1)), // Set the handle pointer to 1
            Return(DVP_STATUS_OK)
        ));

    // Expect dvpGetCameraInfo to be called after opening.
    EXPECT_CALL(*mockWrapper, dvpGetCameraInfo(dvpHandle(1), _))
        .WillOnce(DoAll(
            // Fill the dvpCameraInfo struct with some test data
            Invoke([](dvpHandle h, dvpCameraInfo* pInfo){
                strcpy(pInfo->FriendlyName, "MockCamera");
                strcpy(pInfo->SerialNumber, "SN12345");
            }),
            Return(DVP_STATUS_OK)
        ));

    // Expect initialization and configuration calls
    EXPECT_CALL(*mockWrapper, dvpSetBufferQueueSize(_, _)).WillRepeatedly(Return(DVP_STATUS_OK));
    EXPECT_CALL(*mockWrapper, dvpSetTriggerInputType(_, _)).WillRepeatedly(Return(DVP_STATUS_OK));
    EXPECT_CALL(*mockWrapper, dvpSetExposure(_, _)).WillRepeatedly(Return(DVP_STATUS_OK));
    EXPECT_CALL(*mockWrapper, dvpSetAnalogGain(_, _)).WillRepeatedly(Return(DVP_STATUS_OK));
    EXPECT_CALL(*mockWrapper, dvpSetTargetFormat(_, _)).WillRepeatedly(Return(DVP_STATUS_OK));

    // Act: Create the component with the mock and call the method to be tested.
    auto camera = std::make_unique<Do3ThinkCameraComponent>(mockWrapper);

    QSignalSpy connectionSpy(camera.get(), &Do3ThinkCameraComponent::connectionStateChanged);

    bool result = camera->connectCamera("SN12345");

    // Assert: Verify the results
    ASSERT_TRUE(result);
    EXPECT_EQ(camera->cameraState(), CameraState::Connected);
    EXPECT_EQ(connectionSpy.count(), 1);
    QList<QVariant> arguments = connectionSpy.takeFirst();
    EXPECT_TRUE(arguments.at(0).toBool());
    EXPECT_EQ(camera->getCurrentDevice().serialNumber, "SN12345");
}

TEST_F(Do3ThinkCameraComponentTest, ConnectCamera_FailsOnOpen) {
    // Arrange: Mock SDK fails to open the camera
    EXPECT_CALL(*mockWrapper, dvpOpenByName(_, _, _))
        .WillOnce(Return(DVP_STATUS_FAILED));

    // Act
    auto camera = std::make_unique<Do3ThinkCameraComponent>(mockWrapper);
    QSignalSpy connectionSpy(camera.get(), &Do3ThinkCameraComponent::connectionStateChanged);
    bool result = camera->connectCamera("SN12345");

    // Assert
    ASSERT_FALSE(result);
    EXPECT_EQ(camera->cameraState(), CameraState::Disconnected);
    EXPECT_EQ(connectionSpy.count(), 0);
}
