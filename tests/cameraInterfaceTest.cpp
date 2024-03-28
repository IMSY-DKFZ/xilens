/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/

#include <gtest/gtest.h>

#include "src/cameraInterface.h"
#include "mocks.h"
#include "src/constants.h"


TEST(CameraInterfaceTest, SetCameraTypeTest) {
    std::shared_ptr<MockXiAPIWrapper> apiWrapper = std::make_shared<MockXiAPIWrapper>();
    CameraInterface cameraInterface;
    cameraInterface.m_apiWrapper = apiWrapper;
    QString testCameraModel = "MQ022HG-IM-SM4X4-VIS3";
    cameraInterface.SetCameraProperties(testCameraModel);

    ASSERT_EQ(cameraInterface.m_cameraType, CAMERA_TYPE_SPECTRAL);
}


TEST(CameraInterfaceTest, StartAcquisition_InvalidHandle) {
    std::shared_ptr<MockXiAPIWrapper> apiWrapper = std::make_shared<MockXiAPIWrapper>();
    CameraInterface cameraInterface;
    cameraInterface.m_apiWrapper = apiWrapper;
    cameraInterface.setCamera(CAMERA_TYPE_SPECTRAL, CAMERA_FAMILY_XISPEC);
    QString testCameraModel = "MQ022HG-IM-SM4X4-VIS3";

    EXPECT_THROW(cameraInterface.StartAcquisition(testCameraModel), std::runtime_error);
}


TEST(CameraInterfaceTest, StartAcquisition_StartSuccess)
{
    std::shared_ptr<MockXiAPIWrapper> apiWrapper = std::make_shared<MockXiAPIWrapper>();
    CameraInterface cameraInterface;
    HANDLE cameraHandle;
    cameraInterface.m_cameraHandle = cameraHandle;
    cameraInterface.m_apiWrapper = apiWrapper;
    cameraInterface.setCamera(CAMERA_TYPE_SPECTRAL, CAMERA_FAMILY_XISPEC);
    QString camera_identifier = "TestType";

    EXPECT_NO_THROW(cameraInterface.StartAcquisition(camera_identifier));
}
