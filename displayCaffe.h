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


#ifndef DISPLAYCAFFE_H
#define DISPLAYCAFFE_H


#include <QObject>
#include <string>
#include <opencv2/core/core.hpp>
#include <boost/thread.hpp>
#include <display.h>
#include <opencv2/imgproc.hpp>

#include "xiApi.h"

class MainWindow;
class Network;

enum DisplayImageType {RAW=0, RGB=1, VHB=2, OXY=3};

class DisplayerCaffe : public Displayer
{
    Q_OBJECT

public:

    explicit DisplayerCaffe(MainWindow* mainWindow, Network* network);
    ~DisplayerCaffe();

protected:

    void CreateWindows();
    void DestroyWindows();


public slots:

    virtual void Display(XI_IMG& image);

private:

    cv::Vec3b saturation_color = cv::Vec3b(180,105,255);
    cv::Vec3b dark_color = cv::Vec3b(0,0,255);

    // run the deep net to be able to display results
    void RunNetwork(XI_IMG& image);

    /**
     * @brief DisplayImage General purpose display image function
     *
     * @param image the image to be displayed
     * @param windowName the name of the window for displaying
     */
    void DisplayImage(cv::Mat& image, const std::string windowName);

    // TODO: needs rework
    friend void OcvCallBackFunc(int event, int x, int y, int flags, void* userdata);

    // reference to mainwindow, necessary to detect if normalization is turned on / which band to display
    const MainWindow  * const m_mainWindow;
    // network needed to calculate the data for displaying
    Network* const m_network;

    boost::mutex mtx_; // explicit mutex declaration

    // cv::CLAHE class to do histogram normalization
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();

    /**
     * @brief prepares raw image from XIMEA camera to be displayed, it does histogram normalization in case it is specified
     *
     * @param raw_image, the image to be processed
     */
    void PrepareRawImage(cv::Mat& raw_image, int scaling_factor, bool equalize_hist);

    void NormalizeRGBImage(cv::Mat& bgr_image);


};

// helper function to do proper formatting of functional image
void PrepareFunctionalImage(cv::Mat& functional_image, DisplayImageType displayImage, bool do_scaling, cv::Range bounds, int colormap);

void PrepareRGBImage(cv::Mat& rgb_image, int rgb_norm);

void PrepareRawImage(cv::Mat& raw_image, int scaling_factor, bool equalize_hist);

#endif // DISPLAY_H
