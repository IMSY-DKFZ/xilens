/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#include <QApplication>
#include <QMetaType>
#include <opencv2/core.hpp>

#include "CLI11.h"
#include "mainwindow.h"
#include "util.h"

Q_DECLARE_METATYPE(cv::Mat)
Q_DECLARE_METATYPE(cv::Mat &)

/**
 * @brief Application entry point and command line interface setup.
 */
int main(int argc, char **argv)
{
    CLI::App app{"XIMEA camera recorder"};

    // initialize dummy variables as default values
    g_commandLineArguments.output_folder = "rec";
    g_commandLineArguments.test_mode = false;

    // add options to CLI
    app.add_option("-o,--output", g_commandLineArguments.output_folder, "Output folder");
    app.add_flag("-t,--test", g_commandLineArguments.test_mode, "Test mode");
    app.add_flag("-v,--version", g_commandLineArguments.version, "Print version and build information");

    CLI11_PARSE(app, argc, argv);

    if (g_commandLineArguments.version)
    {
        std::cout << "Version: " << PROJECT_VERSION_MAJOR << "." << PROJECT_VERSION_MINOR << "."
                  << PROJECT_VERSION_PATCH << "\n"
                  << "Build details:"
                  << "\n"
                  << "\tCommit SHA: " << GIT_COMMIT << "\n"
                  << "\tSystem: " << CMAKE_SYSTEM << "\n"
                  << "\tProcessor: " << CMAKE_SYSTEM_PROCESSOR << "\n"
                  << "\tCompiler: " << CMAKE_CXX_COMPILER << "\n"
                  << "\tDate: " << BUILD_TIMESTAMP << "\n";
        exit(0);
    }

    // instantiate application
    QApplication a(argc, argv);
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<cv::Mat &>("cv::Mat&");
    QFile themeFile(":/resources/dark_amber.css");
    if (themeFile.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream stream(&themeFile);
        QString stylesheetContent = stream.readAll();
        a.setStyleSheet(stylesheetContent);
    }
    themeFile.close();
    MainWindow w;
    w.move(400, 10);
    w.show();

    return a.exec();
}
