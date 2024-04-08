/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#ifndef IMAGE_CONTAINER_H
#define IMAGE_CONTAINER_H

#include <xiApi.h>
#include <boost/thread.hpp>
#include <QObject>

#include "xiAPIWrapper.h"
#include "util.h"


class ImageContainer : public QObject {
Q_OBJECT

public:
    /**
     * Pointer to image file object in charge of writing data to file.
     */
    std::unique_ptr<FileImage> m_imageFile;

    /**
     * Wrapper to xiAPI, useful for mocking the aPI during testing
     */
    std::shared_ptr<XiAPIWrapper> m_apiWrapper;

    /**
     * Constructor of image container. The memory of the current image in container is set here with memset
     */
    ImageContainer();

    /**
     * Initializes the container by setting the API for communication with the cameras
     *
     * @param apiWrapper
     */
    void Initialize(std::shared_ptr<XiAPIWrapper> apiWrapper);

    /**
     * Initializes the file where the data will be stored
     * @param filePath
     */
    void InitializeFile(const char *filePath);

    /**
     * Destructor of image container
     */
    ~ImageContainer() override;

    /**
     * @brief Polls for a new image from the camera at a given polling rate.
     *
     * This function continuously polls for a new image from the camera using the specified camera handle.
     * It uses the given polling rate to determine how frequently to poll for new images.
     * Once a new image is obtained, it emits a signal to notify that a new image is available.
     *
     * A lock guard is used to avoid overwriting the current container image when other processes are using it.
     *
     * @param cameraHandle The handle to the camera device.
     * @param pollingRate The polling rate in milliseconds.
     */
    void PollImage(HANDLE *cameraHandle, int pollingRate);

    /**
     * Queries current imag ein container
     *
     * @return current image in container
     */
    XI_IMG GetCurrentImage();

    /**
     * Stops image polling
     */
    void StopPolling();

    /**
     * Starts image polling
     */
    void StartPolling();

    /**
     * Indicates if images should be polled or not
     */
    bool m_PollImage;

signals:

    /**
     * Qt signal used to indicate that a new image has arrived to the container
     */
    void NewImage();

private:

    /**
     * Current container image
     */
    XI_IMG m_Image{};

    /**
     * mutex declaration used to lock guard the current image in the container
     */
    boost::mutex mtx_;
};

#endif // IMAGE_CONTAINER_H
