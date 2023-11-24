/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include <iostream>
#include <string>
#include <stdexcept>
#include <algorithm>

#include <xiApi.h>
#include <boost/log/trivial.hpp>

#include "camera_interface.h"
#include "util.h"
#include "constants.h"


/**
 * @brief A mapper that maps camera models to their corresponding type, e.g. spectral, gray, etc.
 *
 * This mapper is represented as a constant map with camera models as keys and camera types as values.
 */
const QMap<QString, QString> CAMERA_TYPE_MAPPER = {
        {"MQ022HG-IM-SM4X4-VIS",  "spectral"},
        {"MQ022HG-IM-SM4X4-VIS3", "spectral"},
        {"MC050MG-SY-UB",         "gray"}
};


/**
 * \brief Sets the camera type.
 *
 * This function sets the camera type for the camera interface.
 * The camera type is represented by a QString parameter called camera_type.
 *
 * \param camera_type The camera type to be set.
 */
void CameraInterface::SetCameraType(QString camera_type) {
    this->m_cameraType = std::move(camera_type);
}


/**
 * @brief Set the index of the camera.
 *
 * @param index The index of the camera to be set.
 */
void CameraInterface::SetCameraIndex(int index) {
    this->m_cameraIndex = index;
}


/**
 * @brief Updates the recorded temperature of the camera.
 *
 * @return void
 */
void CameraInterface::UpdateRecordedCameraTemperature() {
    float chipTemp, housTemp, housBackSideTemp, sensorBoardTemp;

    xiGetParamFloat(m_camHandle, XI_PRM_CHIP_TEMP, &chipTemp);
    xiGetParamFloat(m_camHandle, XI_PRM_HOUS_TEMP, &housTemp);
    xiGetParamFloat(m_camHandle, XI_PRM_HOUS_BACK_SIDE_TEMP, &housBackSideTemp);
    xiGetParamFloat(m_camHandle, XI_PRM_SENSOR_BOARD_TEMP, &sensorBoardTemp);
    this->m_cameraTemperature[CHIP_TEMP] = chipTemp;
    this->m_cameraTemperature[HOUSE_TEMP] = housTemp;
    this->m_cameraTemperature[HOUSE_BACK_TEMP] = housBackSideTemp;
    this->m_cameraTemperature[SENSOR_BOARD_TEMP] = sensorBoardTemp;
}


/**
 * @brief sets exposure to be updated automatically internally
 */
void CameraInterface::AutoExposure(bool on) {
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_camHandle) {
        stat = xiSetParamInt(m_camHandle, XI_PRM_AEAG, on);
        HandleResult(stat, "xiSetParam (autoexposure on/off)");
    } else {
        BOOST_LOG_TRIVIAL(warning) << "autoexposure not set: camera not initialized";
    }
}


/**
 * @brief Set exposure value for the camera interface.
 *
 * This function sets the exposure value for the camera interface. The exposure
 * value determines how long the camera sensor collects light from the scene.
 *
 * @param exp The exposure value to be set.
 *
 * @warning A valid camera connection is required before calling this function.
 */

void CameraInterface::SetExposure(int exp) {
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_camHandle) {
        // Setting "exposure" parameter (10ms=10000us)
        stat = xiSetParamInt(m_camHandle, XI_PRM_EXPOSURE, exp);
        HandleResult(stat, "xiSetParam (exposure set)");
        BOOST_LOG_TRIVIAL(info) << "set exposure to " << exp / 1000 << "ms\n" << std::flush;
    } else {
        BOOST_LOG_TRIVIAL(warning) << "exposure not set: camera not initialized";
    }
}


/**
 * @brief Sets the exposure time in milliseconds for the camera.
 *
 * @param exp The desired exposure time in milliseconds.
 */
void CameraInterface::SetExposureMs(int exp) {
    this->SetExposure(exp * 1000);
}


/**
 * \brief Retrieves the exposure value from the camera interface.
 * \return The current exposure value.
 */
int CameraInterface::GetExposure() {
    int stat = XI_OK;
    int exp = 40000;
    if (INVALID_HANDLE_VALUE != this->m_camHandle) {
        // Setting "exposure" parameter (10ms=10000us)
        stat = xiGetParamInt(m_camHandle, XI_PRM_EXPOSURE, &exp);
        HandleResult(stat, "xiGetParam (exposure get)");
    } else {
        BOOST_LOG_TRIVIAL(warning) << "exposure not determined, camera not initalized. Return standard value.";
    }

    return exp;
}

/**
 * \brief Retrieves the exposure value from the camera interface.
 * \return The current exposure value in milliseconds.
 */
int CameraInterface::GetExposureMs() {
    return (this->GetExposure() + 5) / 1000;
}


/**
 * @brief Initializes the camera interface.
 *
 * This function initializes the camera interface and prepares it for use.
 * It must be called before any camera-related operations can be performed.
 * It sets basic parameters such as data format to be collected form the camera, bandwidth, buffer size,
 * maximum framerate, downsampling mode and value, and initial exposure time.
 *
 * @note It is recommended to call this function only once during the initialization phase of the program.
 * @note If the camera interface is already initialized, calling this function again will have no effect.
 */
int CameraInterface::InitializeCamera() {
    int current_max_framerate;
    int stat = XI_OK;
    // an overview on all the parameters can be found in:
    // https://www.ximea.com/support/wiki/apis/XiAPI_Manual

    stat = xiSetParamInt(m_camHandle, XI_PRM_IMAGE_DATA_FORMAT, XI_RAW16);
    HandleResult(stat, "xiSetParam (data format raw16)");

    stat = xiSetParamInt(m_camHandle, XI_PRM_RECENT_FRAME, 1);
    HandleResult(stat, "xiSetParam (set to acquire most recent frame)");

    stat = xiSetParamInt(m_camHandle, XI_PRM_AUTO_BANDWIDTH_CALCULATION, XI_ON);
    HandleResult(stat, "xiSetParam (set auto bandwidth calc to on)");

    stat = xiSetParamInt(m_camHandle, XI_PRM_GAIN, XI_GAIN_SELECTOR_ALL);
    HandleResult(stat, "xiSetParam (set gain selector to all)");

    stat = xiSetParamFloat(m_camHandle, XI_PRM_GAIN, 0.);
    HandleResult(stat, "xiSetParam (set gain to zero)");

    stat = xiGetParamInt(m_camHandle, XI_PRM_FRAMERATE XI_PRM_INFO_MAX, &current_max_framerate);
    HandleResult(stat,"get current maximum frame rate");

    stat = xiSetParamInt(m_camHandle, XI_PRM_ACQ_TIMING_MODE, XI_ACQ_TIMING_MODE_FRAME_RATE);
    HandleResult(stat,"set acquisition timing mode to framerate");

    stat = xiSetParamInt(m_camHandle, XI_PRM_FRAMERATE, std::min(FRAMERATE_MAX, current_max_framerate));
    HandleResult(stat,"set maximum frame rate for ultra-fast cameras");

    if (m_cameraType == SPECTRAL_CAMERA) {
        stat = xiSetParamInt(m_camHandle, XI_PRM_DOWNSAMPLING_TYPE, XI_BINNING);
        HandleResult(stat, "xiSetParam (downsampling mode set to binning)");
    } else if (m_cameraType == GRAY_CAMERA) {
        // XIMEA xiC camera models only allow skipping mode
        stat = xiSetParamInt(m_camHandle, XI_PRM_DOWNSAMPLING_TYPE, XI_SKIPPING);
        HandleResult(stat, "xiSetParam (downsampling mode set to skipping)");
    }

    stat = xiSetParamInt(m_camHandle, XI_PRM_DOWNSAMPLING, 1);
    HandleResult(stat, "xiSetParam (no downsampling)");

    stat = xiSetParamInt(m_camHandle, XI_PRM_COUNTER_SELECTOR, XI_CNT_SEL_TRANSPORT_SKIPPED_FRAMES);
    HandleResult(stat, "skipping frames on transport layer");

    // check if this creates a problem, I don't think so if buffer is large enough
    stat = xiSetParamInt(m_camHandle, XI_PRM_BUFFER_POLICY, XI_BP_UNSAFE);
    HandleResult(stat, "set unsafe buffuring policy");

    stat = xiSetParamInt(m_camHandle, XI_PRM_LUT_EN, 0);
    HandleResult(stat, "switch off lut");

    stat = xiSetParamInt(m_camHandle, XI_PRM_OUTPUT_DATA_PACKING, XI_OFF);
    HandleResult(stat, "disable bit packing");

    stat = xiSetParamInt(m_camHandle, XI_PRM_ACQ_BUFFER_SIZE, 70 * 1000 * 1000);
    HandleResult(stat, "set acquistion buffer to 70MB. This should give us a buffer of about 1s");

    stat = xiSetParamFloat(m_camHandle, XI_PRM_EXP_PRIORITY, 1.);
    HandleResult(stat, "if autoexposure is used: only change exposure, not gain");

    SetExposure(40000);

    return stat;
}


/**
 * @brief Starts the acquisition process for the specified camera.
 *
 * This function initiates the acquisition process for the given camera. The camera
 * is identified by its unique name. This method opends the device and calls `xiStartAcquisition` using the camera
 * handle.
 *
 * @param camera_identifier The name of the camera to start acquisition for.
 *
 * @return 0 if the acquisition process started successfully, 1 otherwise.
 *
 * @see StopAcquisition()
 */
int CameraInterface::StartAcquisition(QString camera_identifier) {
    int stat_open = XI_OK;
    stat_open = OpenDevice(m_availableCameras[camera_identifier]);
    printf("Starting acquisition...\n");

    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_camHandle) {
        stat = xiStartAcquisition(m_camHandle);
        HandleResult(stat, "xiStartAcquisition");
        if (XI_OK == stat)
            BOOST_LOG_TRIVIAL(info) << "successfully initialized camera\n";
        else {
            this->CloseDevice();
            throw std::runtime_error("could not start camera initialization");
        }
    } else {
        throw std::runtime_error("didn't start acquisition, camera not properly initialized");
    }
    return stat;
}


/**
 * @brief Stops the acquisition of camera frames.
 *
 * This function is used to stop the acquisition of camera frames from the camera interface.
 * It performs the necessary operations to halt the data acquisition process.
 *
 * @note This function assumes that the camera interface was already initialized and started
 *       the acquisition process.
 *
 * @note This method does not close the device, it only stops the data acquisition from the camera.
 *
 * @see CameraInterface::StartAcquisition()
 */
int CameraInterface::StopAcquisition() {
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_camHandle) {
        BOOST_LOG_TRIVIAL(info) << "Stopping acquisition...";
        stat = xiStopAcquisition(m_camHandle);
        HandleResult(stat, "xiStopAcquisition");
        BOOST_LOG_TRIVIAL(info) << "Done!";
    }
    return stat;
}


/**
 * \brief Opens a camera device with the specified ID.
 *
 * Opens and initializes the camera device
 *
 * \param camera_sn The camera ID of the camera device to open.
 * \return 0 if the camera device was successfully opened, 1 otherwise.
 */

int CameraInterface::OpenDevice(DWORD camera_sn) {
    int stat = XI_OK;
    stat = xiOpenDevice(camera_sn, &m_camHandle);
    HandleResult(stat, "xiGetNumberDevices");

    stat = InitializeCamera();
    if (stat != XI_OK) {
        BOOST_LOG_TRIVIAL(error) << "Failed to initialize camera: " << camera_sn;
        return stat;
    }
    return stat;
}


/**
 * @brief Closes the camera device.
 *
 * Closes the currently used camera device
 *
 * @note Before calling this function, make sure the camera device is opened using OpenDevice().
 *
 * @see OpenDevice()
 */

void CameraInterface::CloseDevice() {
    StopAcquisition();
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_camHandle) {
        BOOST_LOG_TRIVIAL(info) << "Closing device";
        stat = xiCloseDevice(this->m_camHandle);
        HandleResult(stat, "xiCloseDevice");
        //this->m_camHandle = INVALID_HANDLE_VALUE;
        BOOST_LOG_TRIVIAL(info) << "Done!";
    }
}


/**
 * @brief Gets the handle of the CameraInterface object.
 *
 * This function returns the handle of the CameraInterface object. The handle can be used
 * to access the camera interface's properties and methods.
 *
 * @return The handle of the camera object.
 */
HANDLE CameraInterface::GetHandle() {
    return this->m_camHandle;
}


/**
 * @brief Represents an interface for interacting with a camera.
 */
CameraInterface::CameraInterface() :
        m_camHandle(INVALID_HANDLE_VALUE) {
    int stat = XI_OK;
    DWORD numberDevices;
    stat = xiGetNumberDevices(&numberDevices);
    HandleResult(stat, "xiGetNumberDevices");
    BOOST_LOG_TRIVIAL(info) << "number of ximea devices found: " << numberDevices;
}


/**
 * @brief GetAvailableCameraModels
 *
 * This function retrieves the list of available camera models from the CameraInterface.
 *
 * @return QList<QString> - The list of available camera models as keys and device IDs that can be passed to
 * `xiOpenDevice`
 */
QStringList CameraInterface::GetAvailableCameraModels() {
    QStringList cameraModels;
    QStringList cameraSNs;
    // DWORD and HANDLE are defined by xiAPI
    DWORD dwCamCount = 0;
    int stat = XI_OK;
    xiGetNumberDevices(&dwCamCount);

    for (DWORD i = 0; i < dwCamCount; i++) {
        HANDLE hDevice = INVALID_HANDLE_VALUE;
        stat = xiOpenDevice(i, &hDevice);
        if (stat != XI_OK){
            BOOST_LOG_TRIVIAL(error) << "cannot open device with ID: " << i << " perhaps already open?";
        } else {
            char camera_model[256] = {0};
            xiGetParamString(hDevice, XI_PRM_DEVICE_NAME, camera_model, sizeof(camera_model));

            cameraModels.append(QString::fromUtf8(camera_model));
            m_availableCameras[QString::fromUtf8(camera_model)] = i;

            xiCloseDevice(hDevice);
        }
    }

    return cameraModels;
}


/**
 * \brief Destructor of the camera interface.
 */
CameraInterface::~CameraInterface() {
    BOOST_LOG_TRIVIAL(debug) << "CameraInterface::~CameraInterface()";
    this->CloseDevice();
}
