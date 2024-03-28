/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/

#include <gtest/gtest.h>
#include <QApplication>

#include "src/displayFunctional.h"
#include "src/displayRaw.h"
#include "mocks.h"

/**
 * Test that creating the functional displayer and displaying a fake image does not fail
 */
TEST(DisplayerFunctional, DisplayImage) {
    int argc = 0;
    char **argv = nullptr;
    auto app = QApplication(argc, argv);

    MockDisplayerFunctional df;
    df.m_cameraType = CAMERA_TYPE_SPECTRAL;

    auto* image = new XI_IMG;
    image->width = 4 * 512;
    image->height = 4 * 272;
    image->bp = new unsigned char[image->width * image->height * 2]();

    EXPECT_NO_THROW(df.Display(*image));
    delete[] static_cast<unsigned char*>(image->bp);
    delete image;
}


/**
 * Test that creating the functional displayer and displaying a fake image does not fail
 */
TEST(DisplayerRaw, DisplayImage) {
    int argc = 0;
    char **argv = nullptr;
    auto app = QApplication(argc, argv);

    MockDisplayerRaw dr;
    dr.m_cameraType = CAMERA_TYPE_SPECTRAL;

    auto* image = new XI_IMG;
    image->width = 4 * 512;
    image->height = 4 * 272;
    image->bp = new unsigned char[image->width * image->height * 2]();

    EXPECT_NO_THROW(dr.Display(*image));
    delete[] static_cast<unsigned char*>(image->bp);
    delete image;
}