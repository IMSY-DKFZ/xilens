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

TEST_F(MockMainWindowTest, EnableUI) {
    ASSERT_NO_THROW(mockMainWindow->EnableUi(true));
    ASSERT_NO_THROW(mockMainWindow->EnableUi(false));
}

TEST_F(MockMainWindowTest, WriteLogHeaderTest)
{
    // Code to clear or delete the log file before starting the test
    QFile::remove(LOG_FILE_NAME);

    // Call the method under test
    mockMainWindow->WriteLogHeader();

    // Now, read the content of the log file
    QFile file(LOG_FILE_NAME);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        EXPECT_EQ(" git hash: " + QString::fromLatin1(libfiveGitRevision()), in.readLine());
        EXPECT_EQ(" git branch: " + QString::fromLatin1(libfiveGitBranch()), in.readLine());
        EXPECT_EQ(" git tags matching hash: " + QString::fromLatin1(libfiveGitVersion()), in.readLine());
        file.close();
        QFile::remove(LOG_FILE_NAME);
    } else {
        // Handle error: Couldn't open the file, test should fail
        ASSERT_TRUE(false);
    }
}
