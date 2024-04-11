/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#ifndef CAMERA_INTERFACE_H
#define CAMERA_INTERFACE_H

#include <string>

#include <xiApi.h>
#include <opencv2/core/core.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/scoped_ptr.hpp>
#include <QObject>
#include <QString>
#include <QtCore>

#include "constants.h"
#include "imageContainer.h"
#include "camera.h"
#include "xiAPIWrapper.h"


class CameraInterface : public QObject {
Q_OBJECT

public:
    /**
     * @brief The CameraInterface class provides an interface for interacting with a camera.
     *
     * This class represents a camera interface that allows control over various camera functionalities
     * such as capturing images, starting and stopping video recording, and adjusting camera settings.
     *
     * The CameraInterface class does not include any example code or implementation details. It serves
     * as a high-level documentation for the camera interface.
     */
    CameraInterface() : m_cameraHandle(INVALID_HANDLE_VALUE)  {};

    /**
     * Check if XIMEA cameras are connected and counts them
     */
    void Initialize(std::shared_ptr<XiAPIWrapper> apiWrapper);

    /**
     * Wrapper to xiAPI, useful for mocking the aPI during testing
     */
    std::shared_ptr<XiAPIWrapper> m_apiWrapper;

    /**
     * @brief Destructor of camera interface
     */
    ~CameraInterface();

    void setCamera(QString cameraType, QString cameraFamily);

    /**
     * @brief Initializes a device with the specified camera ID.
     *
     * This function opens a device with the specified camera serial number.
     *
     * @param cameraIdentifier The serial number/ID of the camera to open.
     *
     * @return 0 if the device was successfully opened, 1 otherwise.
     */
    int OpenDevice(DWORD cameraIdentifier);

    /**
     * @brief StartAcquisition function
     *
     * This function starts the acquisition process for a specified camera.
     *
     * @param camera_identifier The name of the camera to be used for acquisition.
     */
    int StartAcquisition(QString camera_identifier);

    /**
     * @brief Stops the acquisition process.
     *
     * This function is used to stop the ongoing acquisition process. It ensures that all
     * resources are properly released and the acquisition is halted.
     *
     * @see StartAcquisition
     */
    int StopAcquisition();

    /**
     * @brief Closes the device.
     *
     * This function closes the opened device. It performs necessary cleanup and frees any resources used by the device.
     *
     * @note Before calling this function, the device should be opened using the OpenDevice() function.
     *
     * @see OpenDevice()
     */
    void CloseDevice();


    /**
     * \brief Set the camera type.
     *
     * This function is used to set the camera type for further processing.
     * The camera type is specified using a QString parameter which should
     * contain the appropriate camera type.
     *
     * \param cameraModel A QString specifying the camera type.
     */
    void SetCameraProperties(QString cameraModel);


    /**
     * @brief Sets the camera index.
     *
     * This function sets the camera index as appeared in the ComboBox of the GUI
     *
     * @param index The index of the camera to be set.
     */
    void SetCameraIndex(int index);

    /**
     * @brief The camera temperature data structure.
     *
     * This structure stores temperature values for various camera components.
     * It uses a nested pair structure, where each element consists of a temperature type
     * and its corresponding temperature value.
     *
     * The temperature types are defined as follows:
     *  - CHIP_TEMP: Temperature of the camera chip
     *  - HOUSE_TEMP: Temperature inside the camera housing
     *  - HOUSE_BACK_TEMP: Temperature at the back of the camera housing
     *  - SENSOR_BOARD_TEMP: Temperature of the camera sensor board
     *
     * The temperature values are of type double and represent the temperature in degrees Celsius.
     *
     * Example usage:
     *
     * @code{.cpp}
     * // Accessing temperature values
     * double chipTemp = m_cameraTemperature[CHIP_TEMP];
     * double houseTemp = m_cameraTemperature[HOUSE_TEMP];
     *
     * // Updating temperature values
     * m_cameraTemperature[CHIP_TEMP] = 35.7;
     * m_cameraTemperature[HOUSE_TEMP] = 28.2;
     * @endcode
     */
    QMap<QString, float> m_cameraTemperature = {
            {CHIP_TEMP,         0.},
            {HOUSE_TEMP,        0.},
            {HOUSE_BACK_TEMP,   0.},
            {SENSOR_BOARD_TEMP, 0.},
    };

    /**
     * @brief Retrieves the handle associated with the current camera.
     *
     * This function retrieves the handle, which represents a unique identifier
     * associated with the current camera.
     *
     * @return The handle associated with the current object.
     */
    HANDLE GetHandle();

    /**
     * @brief Map of available cameras.
     *
     * This variable represents the available cameras in the system.
     * It is used to keep track of the number of cameras that can be accessed or used
     * for capturing images or recording videos.
     *
     * @note This variable value may change dynamically based on the currently connected
     *       cameras, their availability or any configuration changes.
     */
    QMap<QString, DWORD> m_availableCameras;

    /**
     * @brief Retrieves a list of available camera models.
     *
     * This function retrieves a list of available camera models that can be used with the application.
     * The returned list contains the names of the camera models.
     *
     * @return QList<QString> A vector of strings representing the available camera models and their ID in the system.
     */
    QStringList GetAvailableCameraModels();

    /**
     * @brief The variable m_cameraModel represents the model of the camera being used.
     */
    QString m_cameraModel;

    /**
     * Serial number of camera
     */
    QString m_cameraSN;

    /**
     * @brief The type of camera used for capturing images.
     */
    QString m_cameraType;

    /**
     * camera index in dropdown menu of GUI
     */
    int m_cameraIndex;

    /**
     * Custom camera object used to differentiate the actions that need to be taken into account across different types
     * of cameras: gray scale, RGB and spectral. E.g. number of bands, etc.
     */
    std::unique_ptr<Camera> m_camera;

    /**
     * Camera family used to identify the behaviour of different families: xiQ, xiSpec, xiC, etc. E.g. xiSpec support
     * some API calls that xiC does not.
     */
    std::unique_ptr<CameraFamily> m_cameraFamily;

    /**
     * Camera family name, e.g. xiSpec, xiC, etc.
     */
    QString m_cameraFamilyName;

    /**
     * @brief The handle for managing the camera device.
     *
     * This variable represents the handle used for managing the camera device. It is typically
     * obtained from the camera API and is responsible for retrieving camera data, controlling
     * camera parameters, and performing camera-related operations.
     *
     * The handle is an opaque pointer that provides an abstraction layer for controlling the
     * camera device. Users are strongly advised against modifying or accessing the handle directly.
     *
     * The handle is used for various camera operations, such as opening and closing the camera,
     * starting and stopping video recording, capturing images, and configuring camera settings.
     * It should be properly initialized and released to prevent resource leaks and ensure proper
     * functioning of the camera module.
     *
     * @see https://www.ximea.com/support/wiki/apis/xiapi_manual for more details on how to use the camera
     * handle and its associated functions.
     *
     * @note This variable should be properly initialized before use and released when no longer
     * required to avoid resource leaks and ensure correct behavior of the camera module.
     */
    HANDLE m_cameraHandle;
};

#endif // CAMERA_INTERFACE_H
