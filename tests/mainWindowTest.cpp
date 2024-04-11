/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/

#include <gtest/gtest.h>
#include <opencv2/core/core.hpp>
#include "mocks.h"


class MockMainWindowTest : public ::testing::Test {
protected:
    std::unique_ptr<MockMainWindow> mockMainWindow;

    void SetUp() override {
        mockMainWindow = std::make_unique<MockMainWindow>();
    }
};


TEST_F(MockMainWindowTest, UpdateSaturationDisplaysTest_EmptyImage) {
    cv::Mat image; // Empty image
    ASSERT_THROW(mockMainWindow->UpdateSaturationPercentageLCDDisplays(image), std::invalid_argument);
}

TEST_F(MockMainWindowTest, UpdateSaturationDisplaysTest_WrongImageType) {
    cv::Mat image = cv::Mat::ones(10, 10, CV_8UC3)*128; // RGB image
    ASSERT_THROW(mockMainWindow->UpdateSaturationPercentageLCDDisplays(image), std::invalid_argument);
}

TEST_F(MockMainWindowTest, UpdateSaturationDisplaysTest_ValidImage) {
    cv::Mat image = cv::Mat::ones(10, 10, CV_8UC1)*128; // Valid grayscale image
    ASSERT_NO_THROW(mockMainWindow->UpdateSaturationPercentageLCDDisplays(image));
}
