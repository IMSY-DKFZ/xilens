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
#include "image_container.h"


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
    CameraInterface();

    /**
     * @brief Destructor of camera interface
     */
    ~CameraInterface();

    /**
     * @brief Sets the exposure value for the camera.
     *
     * This function is used to set the exposure value for the camera.
     *
     * @param exp The exposure value to be set.
     */
    void SetExposure(int exp);

    /**
     * \brief Sets the exposure time in milliseconds.
     *
     * This function is used to set the exposure time in milliseconds for a device.
     *
     * \param exp The exposure time in milliseconds.
     */
    void SetExposureMs(int exp);

    /**
     * @brief Retrieves the exposure value.
     *
     * This function returns the exposure value, which represents the amount of light
     * that reaches the camera sensor. The exposure value determines the brightness
     * of the captured image.
     *
     * @return The exposure value.
     */
    int GetExposure();

    /**
     * @brief Retrieves the exposure time in milliseconds.
     *
     * This function is used to retrieve the exposure time in milliseconds.
     *
     * @return The exposure time in milliseconds.
     */
    int GetExposureMs();

    /**
     * \brief A method to control auto exposure settings.
     *
     * The AutoExposure method enables or disables auto exposure for a given camera.
     * It is used to adjust the camera settings automatically based on the lighting conditions.
     */
    void AutoExposure(bool on);

    /**
     * @brief Initializes a device with the specified camera ID.
     *
     * This function opens a device with the specified camera serial number.
     *
     * @param camera_sn The serial number/ID of the camera to open.
     *
     * @return 0 if the device was successfully opened, 1 otherwise.
     */
    int OpenDevice(DWORD camera_sn);

    /**
     * @brief StartAcquisition function
     *
     * This function starts the acquisition process for a specified camera.
     *
     * @param camera_name The name of the camera to be used for acquisition.
     */
    int StartAcquisition(QString camera_name);

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
     * @brief This function updates the recorded temperature of a camera.
     *
     * This function is responsible for updating the recorded temperature of a camera.
     *
     * This function does not return any value. However, it modifies the state of the camera's recorded temperature.
     *
     * @note This function assumes that the camera object has already been instantiated and the necessary data has been set.
     */
    void UpdateRecordedCameraTemperature();

    /**
     * \brief Set the camera type.
     *
     * This function is used to set the camera type for further processing.
     * The camera type is specified using a QString parameter which should
     * contain the appropriate camera type.
     *
     * \param camera_type A QString specifying the camera type.
     */
    void SetCameraType(QString camera_type);

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
     * @brief The type of camera used for capturing images.
     */
    QString m_cameraType;


private:
    /**
     * @brief Initializes the camera.
     *
     * This function initializes the camera and prepares it for capturing images.
     *
     * @note Before calling this function, ensure that the camera is properly connected and accessible.
     *
     * @warning This function may throw an exception if the camera initialization fails.
     */
    int InitializeCamera();

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
    HANDLE m_camHandle;
};

#endif // CAMERA_INTERFACE_H
