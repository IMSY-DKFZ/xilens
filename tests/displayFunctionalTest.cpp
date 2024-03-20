/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/

#include <gtest/gtest.h>
#include <QApplication>

#include "src/displayFunctional.h"
#include "mocks.h"


void display_image(const XI_IMG* img, const std::string& window_name = "Image")
{
    // Create a cv::Mat for display. Note: we must tell it the image is
    // interpreted as 16-bit unsigned (CV_16U) data, single channel (grayscale).
    cv::Mat image(img->height, img->width, CV_16U, img->bp);

    // Convert to a displayable range (otherwise, high values appear as white).
    cv::Mat display;
    double min_val = 0, max_val = 1023;  // Min and max for 10 bit range
    image.convertTo(display, CV_8U, 255.0 / (max_val - min_val), -min_val);

    // Show image in a window.
    cv::imshow(window_name, display);
    cv::waitKey(0);  // Wait for a key press
}

TEST(DisplayerFunctional, DisplayImage) {
    int argc = 0;
    char **argv = nullptr;
    auto app = QApplication(argc, argv);

    MockDisplayerFunctional df;
    df.m_cameraType = CAMERA_TYPE_SPECTRAL;

    auto* image = new XI_IMG;
    image->width = MOSAIC_SHAPE[1] * 512;
    image->height = MOSAIC_SHAPE[0] * 272;
    image->bp = new unsigned char[image->width * image->height * 2]();

    EXPECT_NO_THROW(df.Display(*image));
    delete[] static_cast<unsigned char*>(image->bp);
    delete image;
}

