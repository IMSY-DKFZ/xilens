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


#ifndef DISPLAYSATURATION_H
#define DISPLAYSATURATION_H


#include <QObject>
#include <display.h>
#include <displayRaw.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <boost/thread.hpp>

#include "xiApi.h"

class MainWindow;

class DisplayerSaturation : public DisplayerRaw
{
    Q_OBJECT

public:

    explicit DisplayerSaturation(MainWindow* mainWindow);
    ~DisplayerSaturation();

protected:

    virtual cv::Mat PrepareImageToDisplay(cv::Mat& image);
private:
    cv::Vec3b saturation_color = cv::Vec3b(180,105,255);
    cv::Vec3b dark_color = cv::Vec3b(0,0,255);
};

#endif // DISPLAY_H
