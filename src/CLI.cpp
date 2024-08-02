/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#include <QApplication>

#include "CLI11.h"
#include "mainwindow.h"
#include "util.h"

/**
 * @brief Application entry point and command line interface setup.
 */
int main(int argc, char **argv)
{
    CLI::App app{"XIMEA camera recorder"};

    // initialize dummy variables as default values
    g_commandLineArguments.model_file = "network.prototxt";
    g_commandLineArguments.trained_file = "model.caffemodel";
    g_commandLineArguments.white_file = "white.tif";
    g_commandLineArguments.dark_file = "dark.tif";
    g_commandLineArguments.output_folder = "rec";
    g_commandLineArguments.test_mode = false;

    // add options to CLI
    app.add_option("-n,--net", g_commandLineArguments.model_file, "Network prototxt file");
    app.add_option("-m,--model", g_commandLineArguments.trained_file, "Model caffemodel file");
    app.add_option("-w,--white", g_commandLineArguments.white_file, "White tif file");
    app.add_option("-d,--dark", g_commandLineArguments.dark_file, "Dark tif file");
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
    QFile themeFile(":/resources/dark_amber.qss");
    if (themeFile.open(QFile::ReadOnly | QFile::Text))
    {
        QTextStream stream(&themeFile);
        QString stylesheetContent = stream.readAll();
        a.setStyleSheet(stylesheetContent);
    }
    MainWindow w;
    w.move(400, 10);
    w.show();

    return a.exec();
}
