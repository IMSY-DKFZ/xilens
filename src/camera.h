/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#ifndef XILENS_CAMERA_H
#define XILENS_CAMERA_H

#include <QMap>
#include <QString>
#include <boost/thread.hpp>
#include <xiApi.h>

#include "constants.h"
#include "xiAPIWrapper.h"

/**
 * @brief CBase class used to identify camera families, e.g. xiQ, xiSpec, etc.
 */
class CameraFamily
{
  protected:
    /**
     * Camera identifier used by API to communicate with camera
     */
    HANDLE *m_cameraHandle;

    /**
     * Mutex used to lock access to variables like the camera temperature, this allows updating temperature from
     * multiple threads.
     */
    boost::mutex m_mutexCameraTemperature;

  public:
    explicit CameraFamily(HANDLE *handle) : m_cameraHandle(handle)
    {
    }

    /**
     * Wrapper to xiAPI, useful for mocking the aPI during testing
     */
    std::shared_ptr<XiAPIWrapper> m_apiWrapper;

    /**
     * @brief The camera temperature data structure.
     *
     * This structure stores temperature values for various camera components.
     * It uses a nested pair structure, where each element consists of a
     * temperature type and its corresponding temperature value.
     *
     * The temperature types are defined as follows:
     *  - CHIP_TEMP: Temperature of the camera chip
     *  - HOUSE_TEMP: Temperature inside the camera housing
     *  - HOUSE_BACK_TEMP: Temperature at the back of the camera housing
     *  - SENSOR_BOARD_TEMP: Temperature of the camera sensor board
     *
     * The temperature values are of type double and represent the temperature in
     * degrees Celsius.
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
        {CHIP_TEMP, 0.},
        {HOUSE_TEMP, 0.},
        {HOUSE_BACK_TEMP, 0.},
        {SENSOR_BOARD_TEMP, 0.},
    };

    /**
     * @brief This function updates the recorded temperature of a camera.
     *
     * This function is responsible for updating the recorded temperature of a
     * camera.
     *
     * This function does not return any value. However, it modifies the state of
     * the camera's recorded temperature.
     *
     * @note This function assumes that the camera object has already been
     * instantiated and the necessary data has been set.
     */
    virtual void UpdateCameraTemperature();

    /*
     * Queries camera temperature
     */
    QMap<QString, float> GetCameraTemperature();
};

/**
 * @brief Class representing the spectral camera family.
 *
 * This camera family manages custom properties of the spectral camera family. Such as updating camera temperature, etc.
 */
class XiSpecFamily : public CameraFamily
{
  private:
    /**
     * Camera handle used to handle all interactions with it
     */
    HANDLE *m_cameraHandle;

  public:
    /**
     * Constructor of camera family
     *
     * @param handle camera handle used for all interactions with it
     */
    explicit XiSpecFamily(HANDLE *handle) : CameraFamily(handle), m_cameraHandle(handle)
    {
    }

    /**
     * Updates recorded camera temperature
     */
    void UpdateCameraTemperature() override;
};

/**
 * @brief Class representing the xiC camera family.
 *
 * This camera family manages custom properties of the xiC camera family. Such as updating camera temperature, etc.
 */
class XiCFamily : public CameraFamily
{
  private:
    /**
     * Camera handle used to handle all interactions with it
     */
    HANDLE *m_cameraHandle;

  public:
    /**
     * Constructor of camera family
     *
     * @param handle camera handle used for all interactions with it
     */
    explicit XiCFamily(HANDLE *handle) : CameraFamily(handle), m_cameraHandle(handle)
    {
    }

    /**
     * Updates camera temperature
     */
    void UpdateCameraTemperature() override;
};

/**
 * @brief Class representing the xiQ camera family.
 *
 * This camera family manages custom properties of the xiQ camera family. Such as updating camera temperature, etc.
 */
class XiQFamily : public CameraFamily
{
  private:
    /**
     * Camera handle used to handle all interactions with it
     */
    HANDLE *m_cameraHandle;

  public:
    /**
     * Constructor of camera family
     *
     * @param handle camera handle used for all interactions with it
     */
    explicit XiQFamily(HANDLE *handle) : CameraFamily(handle), m_cameraHandle(handle)
    {
    }

    /**
     * Updates camera temperature
     */
    void UpdateCameraTemperature() override;
};

/**
 * XiB camera family
 */
using XiBFamily = XiCFamily;

/**
 * XiB64 camera family
 */
using XiB64Family = XiCFamily;

/**
 * XiRAY camera family
 */
using XiRAYFamily = XiCFamily;

/**
 * XiX camera family
 */
using XiXFamily = XiCFamily;

/**
 * @brief Base class used to identify different camera types such as spectral, RGB and gray cameras.
 *
 * This class handles the connection the the API wrapper for communicating with each camera, and defines custom
 * initialization parameters (binning, data format, etc.) for each camera.
 */
class Camera
{
  protected:
    /**
     * Camera handle used to handle all interactions with it
     */
    HANDLE *m_cameraHandle;

  public:
    /**
     * Constructor of camera class
     *
     * @param family family of the camera to be constructed
     * @param handle camera handle used for all interactions with it
     */
    Camera(std::unique_ptr<CameraFamily> *family, HANDLE *handle) : m_cameraFamily(family), m_cameraHandle(handle)
    {
    }

    /**
     * Wrapper to xiAPI, useful for mocking the aPI during testing
     */
    std::shared_ptr<XiAPIWrapper> m_apiWrapper;

    /**
     * Unique pointer to camera family
     */
    std::unique_ptr<CameraFamily> *m_cameraFamily;

    /**
     * initializes camera by setting parameters such as framerate, binning mode,
     * etc.
     *
     * @param cameraHandle camera handle for communication with the camera
     */
    virtual int InitializeCamera();

    /**
     * Initializes camera parameters that are common across all supported cameras:
     *
     * @return status code as an integer
     */
    virtual int InitializeCameraCommonParameters();

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
     * This function is used to set the exposure time in milliseconds for a
     * device.
     *
     * \param exp The exposure time in milliseconds.
     */
    void SetExposureMs(int exp);

    /**
     * @brief Retrieves the exposure value.
     *
     * This function returns the exposure value, which represents the amount of
     * light that reaches the camera sensor. The exposure value determines the
     * brightness of the captured image.
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
     * The AutoExposure method enables or disables auto exposure for a given
     * camera. It is used to adjust the camera settings automatically based on the
     * lighting conditions.
     */
    void AutoExposure(bool on);
};

/**
 * @brief Spectral camera class. Used to communicate with all spectral cameras.
 */
class SpectralCamera : public Camera
{
  public:
    /**
     * Spectral camera constructor
     * @param family family of the camera to be constructed
     * @param handle camera handle used for all interactions with it
     */
    SpectralCamera(std::unique_ptr<CameraFamily> *family, HANDLE *handle) : Camera(family, handle)
    {
    }

    /**
     * Initializes the camera by setting parameters common to all cameras and also
     * specific values for spectral cameras.
     *
     * @param cameraHandle camera handle for management of all interactions with
     * it
     */
    int InitializeCamera() override;
};

/**
 * @brief Gray scale camera class. Used to communicate with all gray scale cameras.
 */
class GrayCamera : public Camera
{
  public:
    /**
     * Constructor of gray scale camera construct
     *
     * @param family family of the camera to be constructed
     * @param handle camera handle used for all interactions with it
     */
    GrayCamera(std::unique_ptr<CameraFamily> *family, HANDLE *handle) : Camera(family, handle)
    {
    }

    /**
     * Initializes the camera by setting parameters common to all cameras and also
     * specific values for gray scale cameras.
     *
     * @param cameraHandle camera handle used to manage all interactions with it
     */
    int InitializeCamera() override;
};

/**
 * @brief RGB camera class. USer to communicate with all color cameras.
 */
class RGBCamera : public Camera
{
  public:
    /**
     * Constructor of RGB camera construct
     *
     * @param family family of the camera to be constructed
     * @param handle camera handle used for all interactions with it
     */
    RGBCamera(std::unique_ptr<CameraFamily> *family, HANDLE *handle) : Camera(family, handle)
    {
    }

    /**
     * Initializes the camera by setting parameters common to all cameras and also
     * specific values for RGB cameras.
     *
     * @param cameraHandle camera handle used to manage all interactions with it
     */
    int InitializeCamera() override;
};

#endif // XILENS_CAMERA_H
