/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include <gtest/gtest.h>

#include <QApplication>

#include "mocks.h"
#include "src/displayFunctional.h"

/**
 * Test that creating the functional displayer and displaying a fake image does
 * not fail
 */
TEST(DisplayerFunctional, DisplayImage)
{
    MockDisplayerFunctional df;
    QString testCameraModel = "MQ022HG-IM-SM4X4-VIS3";
    df.SetCameraProperties(testCameraModel);
    df.m_cameraType = CAMERA_TYPE_SPECTRAL;

    auto *image = new XI_IMG;
    image->width = 4 * 512;
    image->height = 4 * 272;
    image->bp = new unsigned char[image->width * image->height * 2]();

    EXPECT_NO_THROW(df.Display(*image));
    delete[] static_cast<unsigned char *>(image->bp);
    delete image;
}
