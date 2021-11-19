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


#include "displaySaturation.h"
#include "displayRaw.h"

#include <iostream>
#include <string>
#include <stdlib.h>     //for using the function sleep

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#if CV_VERSION_MAJOR==3
#include <opencv2/videoio.hpp>
#endif
#if __has_include(<opencv2/contrib/contrib.hpp>)
#else
#  define opencv2_has_contrib 0
#endif

#include <boost/chrono.hpp>
#include <boost/thread.hpp>


#include "mainwindow.h"
#include "util.h"
#include "default_defines.h"
#include "caffe_interface.h"


const std::string DISPLAY_WINDOW_NAME = "SuSI Live Cam";




DisplayerSaturation::DisplayerSaturation(MainWindow* mainWindow) :
    DisplayerRaw(mainWindow)
{
    CreateWindows();
}


DisplayerSaturation::~DisplayerSaturation()
{
    DestroyWindows();
}


cv::Mat DisplayerSaturation::PrepareImageToDisplay(cv::Mat& image)
{
    image = DisplayerRaw::PrepareImageToDisplay(image);

    cvtColor(image, image, cv::COLOR_GRAY2RGB);
    cv::Mat lut = CreateLut(this->saturation_color, this->dark_color);
    cv::LUT(image, lut, image);

    return image;
}
