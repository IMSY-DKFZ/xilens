/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include "image_container.h"
#include "util.h"

#include <iostream>
#include "xiApi.h"
#include <boost/thread.hpp>



ImageContainer::ImageContainer() : m_PollImage(true)
{
    // image buffer
    memset(&m_Image,0,sizeof(m_Image));
    m_Image.size = sizeof(XI_IMG);
}

ImageContainer::~ImageContainer()
{
#ifdef DEBUG_THIS
    std::cout << "DEBUG: ImageContainer::~ImageContainer()\n" << std::flush;
#endif
}


void ImageContainer::PollImage(HANDLE camHandle, int pollingRate)
{
    static unsigned lastImageId = 0;
    while (m_PollImage)
    {
        int stat = XI_OK;
        {
            boost::lock_guard<boost::mutex> guard(mtx_);
            stat = xiGetImage(camHandle, 5000, &m_Image);
            HandleResult(stat, "xiGetImage");
            if (m_Image.acq_nframe != lastImageId)
            {
                emit NewImage();
                lastImageId = m_Image.acq_nframe;
            }
        }
        wait(pollingRate);
        // std::cout << "grabbed image " << image.acq_nframe << "\n" << std::flush;
    }
}


void ImageContainer::StopPolling()
{
    m_PollImage = false;
}

void ImageContainer::StartPolling()
{
    m_PollImage = true;
}


XI_IMG ImageContainer::GetCurrentImage()
{
    boost::lock_guard<boost::mutex> guard(mtx_);
    return m_Image;
}
