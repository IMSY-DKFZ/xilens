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


#ifndef DISPLAYRAW_H
#define DISPLAYRAW_H


#include <QObject>
#include <display.h>

#include <opencv2/core/core.hpp>
#include <string>
#include <boost/thread.hpp>

#include "xiApi.h"

class MainWindow;

class DisplayerRaw : public Displayer
{
    Q_OBJECT

public:

    explicit DisplayerRaw(MainWindow* mainWindow);
    void DrawHistogram(const cv::Mat roiImg);
    ~DisplayerRaw();

protected:

    void CreateWindows();
    void DestroyWindows();
    virtual cv::Mat PrepareImageToDisplay(cv::Mat& image);


public slots:

    virtual void Display(XI_IMG& image);


private:


    void DisplayImage(cv::Mat& image, const std::string windowName);

    // reference to mainwindow, necessary to detect if normalization is turned on
    const MainWindow  * const m_mainWindow;
    boost::mutex mtx_; // explicit mutex declaration

    cv::Mat m_displayImage;
    cv::Mat m_rawImage;
};

#endif // DISPLAY_H
