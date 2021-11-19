/*
 * ===================================================================
 * Surgical Spectral Imaging Library (SuSI)
 *
 * Copyright (c) German Cancer Research Center,
 * Division of Medical and Biological Informatics.
 * All rights reserved.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 *
 * See LICENSE.txt for details.
 * ===================================================================
 */


#include "mainwindow.h"
#include "default_defines.h"
#include "util.h"

#include <string>
#include <QApplication>



void print_help(char* program_name)
{
    std::cerr << "Usage: " << program_name
              << " [network.prototxt] [model.caffemodel]"
              << " [white.tif] [dark.tif] [data_folder] [testmode:true|false]"
              << std::endl;
}


int main(int argc, char *argv[])
{
    // if one argument: -h or --help:
    if ((argc == 2) && ((std::strcmp(argv[1],"-h")==0) || (std::strcmp(argv[1],"--help")==0)))
    {
        print_help(argv[0]);
        return 1;
    }
    g_commandLineArguments.model_file = "network.prototxt";
    g_commandLineArguments.trained_file = "model.caffemodel";
    g_commandLineArguments.white_file = "white.tif";
    g_commandLineArguments.dark_file = "dark.tif";
    g_commandLineArguments.output_folder = "rec";
    g_commandLineArguments.test_mode = false;

    if (argc > 1)
        g_commandLineArguments.model_file = argv[1];
    if (argc > 2)
        g_commandLineArguments.trained_file = argv[2];
    if (argc > 3)
        g_commandLineArguments.white_file = argv[3];
    if (argc > 4)
        g_commandLineArguments.dark_file = argv[4];
    if (argc > 5)
        g_commandLineArguments.output_folder = argv[5];
    if (argc > 6)
        g_commandLineArguments.test_mode = std::strcmp(argv[6],"true")==0;
    if (argc > 7)
        print_help(argv[0]);

    initLogging(boost::log::trivial::info);
    QApplication a(argc, argv);
    MainWindow w;
    w.move(400, 10);
    w.show();

    return a.exec();
}
