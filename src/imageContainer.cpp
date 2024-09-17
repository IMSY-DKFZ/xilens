/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#include "imageContainer.h"

#include <xiApi.h>

#include <boost/thread.hpp>
#include <iostream>

#include "logger.h"
#include "util.h"

ImageContainer::ImageContainer() : m_PollImage(true)
{
    memset(&m_Image, 0, sizeof(m_Image));
    m_Image.size = sizeof(XI_IMG);
}

void ImageContainer::Initialize(std::shared_ptr<XiAPIWrapper> apiWrapper)
{
    this->m_apiWrapper = apiWrapper;
}

void ImageContainer::InitializeFile(const char *filePath)
{
    auto image = GetCurrentImage();
    this->m_imageFile = std::make_unique<FileImage>(filePath, image.height, image.width);
}

void ImageContainer::CloseFile()
{
    if (this->m_imageFile)
    {
        this->m_imageFile->AppendMetadata();
        this->m_imageFile = nullptr;
        LOG_XILENS(info) << "Closed recording file";
    }
}

ImageContainer::~ImageContainer()
{
    LOG_XILENS(info) << "Destroying image container";
}

void ImageContainer::PollImage(HANDLE *cameraHandle, int pollingRate)
{
    static unsigned lastImageId = 0;
    static int stat;
    while (m_PollImage)
    {
        {
            boost::lock_guard<boost::mutex> guard(m_mutexImageAccess);
            boost::this_thread::interruption_point();
            if (cameraHandle != INVALID_HANDLE_VALUE)
            {
                stat = m_apiWrapper->xiGetImage(*cameraHandle, 5000, &m_Image);
                try
                {
                    HandleResult(stat, "xiGetImage");
                }
                catch (const std::exception &e)
                {
                    this->StopPolling();
                    LOG_XILENS(error) << "Error while trying to get image from device";
                    this->CloseFile();
                    throw;
                }
            }
            if (m_Image.acq_nframe != lastImageId)
            {
                emit NewImage();
                lastImageId = m_Image.acq_nframe;
            }
        }
        WaitMilliseconds(pollingRate);
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
    boost::lock_guard<boost::mutex> guard(m_mutexImageAccess);
    return m_Image;
}
