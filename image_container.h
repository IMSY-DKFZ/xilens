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


#ifndef IMAGECONTAINER_H
#define IMAGECONTAINER_H

#include <xiApi.h>
#include <boost/thread.hpp>
#include <QObject>

class ImageContainer : public QObject
{
    Q_OBJECT

public:
    ImageContainer();
    ~ImageContainer();

    // pollingRate in ms
    void PollImage(HANDLE camHandle, int pollingRate);
    XI_IMG GetCurrentImage();
    void StopPolling();
    void StartPolling();

signals:
    void NewImage();

private:

    bool m_PollImage;
    XI_IMG m_Image;
    boost::mutex mtx_; // explicit mutex declaration
};

#endif // IMAGECONTAINER_H
