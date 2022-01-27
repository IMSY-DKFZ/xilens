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


#include "displayCaffe.h"

#include <iostream>
#include <string>
#include <stdlib.h>     //for using the function sleep
#include <stdio.h>

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
#include "caffe_interface.h"


const std::string DISPLAY_WINDOW_NAME = "SuSI Live Cam";
const std::string VHB_WINDOW_NAME = "Blood volume fraction";
const std::string SAO2_WINDOW_NAME = "Oxygenation";
const std::string BGR_WINDOW_NAME = "RGB estimate";
typedef cv::Point3_<uint8_t> Pixel;


DisplayerCaffe::DisplayerCaffe(MainWindow* mainWindow, Network* network) :
    Displayer(), m_mainWindow(mainWindow), m_network(network)
{
    CreateWindows();
}


DisplayerCaffe::~DisplayerCaffe()
{
    DestroyWindows();
}


void PrepareFunctionalImage(cv::Mat& functional_image, [[maybe_unused]]DisplayImageType displayImage, bool do_scaling, cv::Range bounds, int colormap)
{
    if (do_scaling)
    {
        functional_image *= 100;
        // set first and second pixel to extreme values so the colormap is always scaled the same
        functional_image.at<float>(0,0) = bounds.start;
        functional_image.at<float>(0,1) = bounds.end;
        clamp(functional_image, bounds);
    }

    rescale(functional_image, 255.);
    functional_image.convertTo(functional_image, CV_8UC1);
    applyColorMap(functional_image, functional_image, colormap);
}


void PrepareRGBImage(cv::Mat& bgr_image, int rgb_norm)
{
    double min, max;
    static double last_norm = 1.;
    cv::minMaxLoc(bgr_image, &min, &max);

    last_norm = 0.9*last_norm + (double)rgb_norm*0.01*max;

    bgr_image*=255./last_norm;
    bgr_image.convertTo(bgr_image, CV_8UC3);
}

/**
 * NormalizeRGBImage normalizes the bgr_image using clahe.
 *
 * We define the matrix lab_images and convert the color bgr to lab.
 * We define a vector lab_planes and split the lab_image into lab_planes.
 * We define a pointer clahe, create clahe and set a treshold for constant limiting.
 * We define a matrix dest apply clate to the vector lab_planes and save it in the same vector.
 * We merge lab_planes into lab_image.
 * We convert the color lab to bgr and save it in bgr_image.
 *
 * @param[in] matrix bgr_image
 * @param[out] matrix bgr_image
 */
void DisplayerCaffe::NormalizeRGBImage(cv::Mat& bgr_image)
{
    cv::Mat lab_image;
    cvtColor(bgr_image, lab_image, cv::COLOR_BGR2Lab);

    //ectract L channel
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);


    //apply clahe to the L channel and save it in lab_planes
    cv::Mat dst;
    this->clahe->apply(lab_planes[0], dst);
    dst.copyTo(lab_planes[0]);

    //merge color planes back to bgr_image
    cv::merge(lab_planes, lab_image);

    //convert back to rgb
    cv::cvtColor(lab_image, bgr_image, cv::COLOR_Lab2BGR);

}


bool IsFunctional(DisplayImageType displayImageType)
{
    return (displayImageType == OXY || displayImageType == VHB);
}



void DisplayerCaffe::PrepareRawImage(cv::Mat& raw_image, int scaling_factor, bool equalize_hist)
{
    raw_image /= scaling_factor; // 10 bit to 8 bit
    raw_image.convertTo(raw_image, CV_8UC1);
    cv::Mat mask = raw_image.clone();
    cvtColor(mask, mask, cv::COLOR_GRAY2RGB);
    cv::Mat lut = CreateLut(this->saturation_color, this->dark_color);
    cv::LUT(mask, lut, mask);
    if (equalize_hist)
    {
        this->clahe->apply(raw_image, raw_image);
    }
    cvtColor(raw_image, raw_image, cv::COLOR_GRAY2RGB);

    // Parallel execution on each pixel using C++11 lambda.
    raw_image.forEach<Pixel>([mask, this](Pixel &p, const int position[]) -> void {
        if (mask.at<cv::Vec3b>(position[0], position[1]) == this->saturation_color){
            p.x = this->saturation_color[0];
            p.y = this->saturation_color[1];
            p.z = this->saturation_color[2];
        }
        else if(mask.at<cv::Vec3b>(position[0], position[1]) == this->dark_color){
            p.x = this->dark_color[0];
            p.y = this->dark_color[1];
            p.z = this->dark_color[2];
        }
    });
}



void DisplayerCaffe::DisplayImage(cv::Mat& image, const std::string windowName)
{
    if (image.channels() != 1 && image.channels() != 3)
    {
        throw std::runtime_error("number of channels need to be either 1 or 3");
    }

    cv::Size newSize(image.size().width, image.size().height);
    cv::resize(image, image, newSize);
    cv::imshow(windowName.c_str(), image);
}


/**
 * DisplayerCaffe calls methods and shows images on the display.
 *
 * We define a static int, set it's value zero and increment.
 * If the value equals one or is bigger than 10 and if we skip every 100th frame we run the network.
 * We define vectors which we use in the methods we are calling from the network.
 * We prepare the raw image from XIMEA camera to be displayed then we display the image.
 * We define a matrix, vector and int to be used in the method calling from the network.
 * If normalization is checked in the ui we normalize the image and then display the image.
 * We also display the functional image.
 *
 * @param[in] struct XI_IMG pointing to image
 * @param[out] matrix
 */
void DisplayerCaffe::Display(XI_IMG& image)
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

            m_network->GetBand(band_image, m_mainWindow->GetBand());
            m_network->GetPhysiologicalParameters(physParam_image);
            m_network->GetBGR(bgr_image);

            PrepareRawImage(band_image.at(0), 4, m_mainWindow->GetNormalize());
            DisplayImage(band_image.at(0), DISPLAY_WINDOW_NAME);

//            static cv::Mat bgr_composed = cv::Mat::zeros(bgr_image.at(0).size(), CV_32FC3);
//            cv::merge(bgr_image, bgr_composed);
//            PrepareRGBImage(bgr_composed, m_mainWindow->GetRGBNorm());
//            DisplayImage(bgr_composed, BGR_WINDOW_NAME);
            
            cv::Mat rgb_image;
            std::vector<int> bands{3, 15, 11};
            int scaling_factor = 1024;
            m_network->GetBands(rgb_image, scaling_factor);
            if (m_mainWindow->GetRGBNorm())
            {
                //test
                std::cout << "This works";
                // do normalization
                NormalizeRGBImage(rgb_image);
            }

            DisplayImage(rgb_image, BGR_WINDOW_NAME);
           

            PrepareFunctionalImage(physParam_image.at(0), VHB, m_mainWindow->DoParamterScaling(), m_mainWindow->GetUpperLowerBoundsVhb(), cv::COLORMAP_JET);
            DisplayImage(physParam_image.at(0), VHB_WINDOW_NAME);

            PrepareFunctionalImage(physParam_image.at(1), OXY, m_mainWindow->DoParamterScaling(), m_mainWindow->GetUpperLowerBoundsSao2(), cv::COLORMAP_JET);
            DisplayImage(physParam_image.at(1), SAO2_WINDOW_NAME);
        }
    }
}


void DisplayerCaffe::RunNetwork(XI_IMG& image)
{
    cv::Mat currentImage(image.height, image.width, CV_16UC1, image.bp);
    m_network->SetImage(currentImage, Network::IMAGE);
    m_network->Run();
}


void DisplayerCaffe::CreateWindows()
{
    // create windows to display result
    cv::namedWindow(DISPLAY_WINDOW_NAME, cv::WINDOW_NORMAL);
    cv::namedWindow(BGR_WINDOW_NAME, cv::WINDOW_NORMAL);
    cv::namedWindow(VHB_WINDOW_NAME, cv::WINDOW_NORMAL);
    cv::namedWindow(SAO2_WINDOW_NAME, cv::WINDOW_NORMAL);

    cv::moveWindow(DISPLAY_WINDOW_NAME, 900, 10);
    cv::moveWindow(BGR_WINDOW_NAME, 2024+11, 10);
    cv::moveWindow(VHB_WINDOW_NAME, 900, 10+626);
    cv::moveWindow(SAO2_WINDOW_NAME, 2024+11, 10+626);
}


void DisplayerCaffe::DestroyWindows()
{
    cv::destroyAllWindows();
}

