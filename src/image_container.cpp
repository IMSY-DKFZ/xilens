/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include <iostream>

#include <xiApi.h>
#include <boost/thread.hpp>

#include "image_container.h"
#include "util.h"


/**
 * @brief The ImageContainer class represents an image container object.
 *
 * This class encapsulates an image container object used for storing and manipulating images. The memory for the image
 * is is set by memset here.
 */
ImageContainer::ImageContainer() : m_PollImage(true) {
    memset(&m_Image, 0, sizeof(m_Image));
    m_Image.size = sizeof(XI_IMG);
}


/**
 * Destructor of image container
 */
ImageContainer::~ImageContainer() {
    BOOST_LOG_TRIVIAL(info) << "Destroying image container";
}


/**
 * @brief Polls for a new image from the camera at a given polling rate.
 *
 * This function continuously polls for a new image from the camera using the specified camera handle.
 * It uses the given polling rate to determine how frequently to poll for new images.
 * Once a new image is obtained, it emits a signal to notify that a new image is available.
 *
 * A lock guard is used to avoid overwriting the current container image when other processes are using it.
 *
 * @param camHandle The handle to the camera device.
 * @param pollingRate The polling rate in milliseconds.
 */
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


/**
 * Stop image polling
 */
void ImageContainer::StopPolling() {
    m_PollImage = false;
}


/**
 * Starts image polling
 */
void ImageContainer::StartPolling() {
    m_PollImage = true;
}


/**
 * Queries the current image int he container. a lock guard is used to avoid the current image in the container to be
 * overwritten while it is queried.
 *
 * @return current image in container
 */
XI_IMG ImageContainer::GetCurrentImage() {
    boost::lock_guard<boost::mutex> guard(mtx_);
    return m_Image;
}
