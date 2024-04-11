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
    static QRegularExpression timestampRegex(R"(^\d{8}_\d{2}-\d{2}-\d{2}-\d{3})");
    auto logFilePath = mockMainWindow->GetLogFilePath(LOG_FILE_NAME);
    QFile::remove(logFilePath);

    mockMainWindow->WriteLogHeader();

    QFile file(logFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);

        auto line = in.readLine();
        EXPECT_TRUE(timestampRegex.match(line).hasMatch());
        EXPECT_TRUE(line.contains(" git hash: " + QString::fromLatin1(libfiveGitRevision())));

        line = in.readLine();
        EXPECT_TRUE(timestampRegex.match(line).hasMatch());
        EXPECT_TRUE(line.contains(" git branch: " + QString::fromLatin1(libfiveGitBranch())));

        line = in.readLine();
        EXPECT_TRUE(timestampRegex.match(line).hasMatch());
        EXPECT_TRUE(line.contains(" git tags matching hash: " + QString::fromLatin1(libfiveGitVersion())));

        file.close();
        QFile::remove(logFilePath);
    } else {
        ASSERT_TRUE(false);
    }
}
