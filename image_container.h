/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#ifndef IMAGECONTAINER_H
#define IMAGECONTAINER_H

#include <xiApi.h>
#include <boost/thread.hpp>
#include <QObject>


class ImageContainer : public QObject {
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
