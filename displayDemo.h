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


#ifndef DISPLAYDEMO_H
#define DISPLAYDEMO_H


#include <QObject>
#include <string>
#include <opencv2/core/core.hpp>
#include <boost/thread.hpp>
#include <display.h>

#include "xiApi.h"

class MainWindow;
class Network;

class DisplayerDemo : public Displayer
{
    Q_OBJECT

public:

    explicit DisplayerDemo(MainWindow* mainWindow, Network* network);
    ~DisplayerDemo();

protected:

    void CreateWindows();
    void DestroyWindows();


public slots:

    virtual void Display(XI_IMG& image);

private:

    // run the deep net to be able to display results
    void RunNetwork(XI_IMG& image);

    /**
     * @brief DisplayImage General purpose display image function
     *
     * @param image the image to be displayed
     * @param functional_image physiological image to be overlayed
     * @param windowName the name of the window for displaying
     */
    void DisplayImage(cv::Mat& image, cv::Mat& functional_image, const std::string windowName);

    // TODO: needs rework
    friend void OcvCallBackFunc(int event, int x, int y, int flags, void* userdata);

    // reference to mainwindow, necessary to detect if normalization is turned on / which band to display
    const MainWindow  * const m_mainWindow;
    // network needed to calculate the data for displaying
    Network* const m_network;

    boost::mutex mtx_; // explicit mutex declaration
};

#endif // DISPLAY_H
