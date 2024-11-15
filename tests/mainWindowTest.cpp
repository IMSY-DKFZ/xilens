/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include <gtest/gtest.h>

#include <opencv2/core/core.hpp>

#include "mocks.h"
#include "ui_mainwindow.h"

class MockMainWindowTest : public ::testing::Test
{
  protected:
    std::unique_ptr<MockMainWindow> mockMainWindow;

    void SetUp() override
    {
        mockMainWindow = std::make_unique<MockMainWindow>();
    }
};

TEST_F(MockMainWindowTest, UpdateSaturationDisplays)
{
    ASSERT_NO_THROW(mockMainWindow->UpdateSaturationPercentageLCDDisplays(100, 0));
}

TEST_F(MockMainWindowTest, EnableUI)
{
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
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);

        auto line = in.readLine();
        EXPECT_TRUE(timestampRegex.match(line).hasMatch());
        EXPECT_TRUE(line.contains(" git hash: " + QString(GIT_COMMIT)));

        file.close();
        QFile::remove(logFilePath);
    }
    else
    {
        ASSERT_TRUE(false);
    }
}

TEST_F(MockMainWindowTest, DisplayRecordedImageCounter)
{
    int valueToDisplay = 10;
    mockMainWindow->SetRecordedCount(valueToDisplay);
    mockMainWindow->DisplayRecordCount();
    qApp->processEvents(QEventLoop::AllEvents, 100);
    auto ui = mockMainWindow->GetUI();
    auto displayedValue = ui->recordedImagesLCDNumber->value();
    ASSERT_TRUE(displayedValue == valueToDisplay);
}
