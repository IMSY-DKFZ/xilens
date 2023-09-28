/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include <iostream>

#include <xiApi.h>
#include <boost/thread.hpp>

#include "image_container.h"
#include "util.h"


ImageContainer::ImageContainer() : m_PollImage(true) {
    // image buffer
    memset(&m_Image, 0, sizeof(m_Image));
    m_Image.size = sizeof(XI_IMG);
}

ImageContainer::~ImageContainer() {
    BOOST_LOG_TRIVIAL(debug) << "Destroying image container";
}


void ImageContainer::PollImage(HANDLE camHandle, int pollingRate) {
    static unsigned lastImageId = 0;
    while (m_PollImage) {
        int stat = XI_OK;
        {
            boost::lock_guard<boost::mutex> guard(mtx_);
            stat = xiGetImage(camHandle, 5000, &m_Image);
            HandleResult(stat, "xiGetImage");
            if (m_Image.acq_nframe != lastImageId) {
                emit NewImage();
                lastImageId = m_Image.acq_nframe;
            }
        }
        wait(pollingRate);
    }
}


void ImageContainer::StopPolling() {
    m_PollImage = false;
}

void ImageContainer::StartPolling() {
    m_PollImage = true;
}


XI_IMG ImageContainer::GetCurrentImage() {
    boost::lock_guard<boost::mutex> guard(mtx_);
    return m_Image;
}
