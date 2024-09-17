/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include <gtest/gtest.h>

#include "mocks.h"
#include "src/cameraInterface.h"
#include "src/constants.h"

TEST(CameraInterfaceTest, SetCameraTypeTest)
{
    std::shared_ptr<MockXiAPIWrapper> apiWrapper = std::make_shared<MockXiAPIWrapper>();
    CameraInterface cameraInterface;
    cameraInterface.m_apiWrapper = apiWrapper;
    QString cameraIdentifier = "MQ022HG-IM-SM4X4-VIS3";
    cameraInterface.m_availableCameras[cameraIdentifier] = 0;
    cameraInterface.SetCameraProperties(cameraIdentifier);

    ASSERT_EQ(cameraInterface.m_cameraType, CAMERA_TYPE_SPECTRAL);
}

TEST(CameraInterfaceTest, SetWrongCameraPropertiesTest)
{
    std::shared_ptr<MockXiAPIWrapper> apiWrapper = std::make_shared<MockXiAPIWrapper>();
    CameraInterface cameraInterface;
    cameraInterface.m_apiWrapper = apiWrapper;
    QString cameraIdentifier = "FakeCameraModel";
    cameraInterface.m_availableCameras[cameraIdentifier] = 0;
    EXPECT_THROW(cameraInterface.SetCameraProperties(cameraIdentifier), std::runtime_error);
}

TEST(CameraInterfaceTest, StartAcquisition_InvalidHandle)
{
    std::shared_ptr<MockXiAPIWrapper> apiWrapper = std::make_shared<MockXiAPIWrapper>();
    CameraInterface cameraInterface;
    cameraInterface.m_apiWrapper = apiWrapper;
    cameraInterface.SetCamera(CAMERA_TYPE_SPECTRAL, CAMERA_FAMILY_XISPEC);
    QString cameraIdentifier = "MockDeviceModel@MockSensorSN";
    cameraInterface.m_availableCameras[cameraIdentifier] = 0;

    EXPECT_THROW(cameraInterface.StartAcquisition(cameraIdentifier), std::runtime_error);
}

TEST(CameraInterfaceTest, StartAcquisition_StartSuccess)
{
    std::shared_ptr<MockXiAPIWrapper> apiWrapper = std::make_shared<MockXiAPIWrapper>();
    CameraInterface cameraInterface;
    HANDLE cameraHandle;
    cameraInterface.m_cameraHandle = cameraHandle;
    cameraInterface.m_apiWrapper = apiWrapper;
    cameraInterface.SetCamera(CAMERA_TYPE_SPECTRAL, CAMERA_FAMILY_XISPEC);
    QString cameraIdentifier = "MockDeviceModel@MockSensorSN";
    cameraInterface.m_availableCameras[cameraIdentifier] = 0;

    EXPECT_NO_THROW(cameraInterface.StartAcquisition(cameraIdentifier));
}
