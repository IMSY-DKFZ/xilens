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


#include "displayDemo.h"

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
#  include <opencv2/contrib/contrib.hpp>
#else
#  define opencv2_has_contrib 0
#endif

#include <boost/chrono.hpp>
#include <boost/thread.hpp>


#include "mainwindow.h"
#include "util.h"
#include "default_defines.h"
#include "displayCaffe.h"


const std::string DISPLAY_WINDOW_NAME = "SuSI Live Cam";

DisplayerDemo::DisplayerDemo(MainWindow* mainWindow) :
    Displayer(), m_mainWindow(mainWindow)
{
    CreateWindows();
}


DisplayerDemo::~DisplayerDemo()
{
    DestroyWindows();
}



void DisplayerDemo::DisplayImage(cv::Mat& image, cv::Mat& functionalImage, const std::string windowName)
{
    int scaleFactor = 2;
    cv::Size newSize(image.size().width*scaleFactor, image.size().height*scaleFactor);
    cv::resize(image, image, newSize);
    cv::resize(functionalImage, functionalImage, newSize);

    // select a functional window ROI
    cv::Rect ROI((256-50)*scaleFactor, (136-30)*scaleFactor, 100*scaleFactor, 60*scaleFactor);
    cv::Mat functionalWindow = functionalImage(ROI);

    // copy this into RGB
    functionalWindow.copyTo(image(ROI));

    cv::imshow(windowName.c_str(), image);
}


void DisplayerDemo::Display(XI_IMG& image)
{
    static int selected_display = 0;

    selected_display++;
    // give it some time to draw the first frame. For some reason neccessary.
    // probably has to do with missing waitkeys after imshow (these crash the application)
    if ((selected_display==1) || (selected_display>10))
    {
        // additionally, give it some chance to recover from lots of ui interaction by skipping
        // every 100th frame
        if (selected_display % 100 > 0)
        {

            boost::lock_guard<boost::mutex> guard(mtx_);

            RunNetwork(image);

            std::vector<cv::Mat> band_image, physParam_image, bgr_image;

            static cv::Mat bgr_composed = cv::Mat::zeros(bgr_image.at(0).size(), CV_32FC3);
            cv::merge(bgr_image, bgr_composed);
            PrepareRGBImage(bgr_composed, m_mainWindow->GetRGBNorm());
            cv::Mat functionalImage = physParam_image.at(1);
            PrepareFunctionalImage(functionalImage, OXY, m_mainWindow->DoParamterScaling(), m_mainWindow->GetUpperLowerBoundsSao2(), cv::COLORMAP_JET);

            DisplayImage(bgr_composed, functionalImage, DISPLAY_WINDOW_NAME);
        }
    }
}


void DisplayerDemo::RunNetwork(XI_IMG& image)
{
    cv::Mat currentImage(image.height, image.width, CV_16UC1, image.bp);

}


void DisplayerDemo::CreateWindows()
{
    // create windows to display result
    cv::namedWindow(DISPLAY_WINDOW_NAME, cv::WINDOW_NORMAL);

    cv::moveWindow(DISPLAY_WINDOW_NAME, 1000, 10);
}


void DisplayerDemo::DestroyWindows()
{
    cv::destroyAllWindows();
}
