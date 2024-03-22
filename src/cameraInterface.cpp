/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include <iostream>
#include <stdexcept>
#include <boost/log/trivial.hpp>
#include <utility>

#include "cameraInterface.h"
#include "util.h"
#include "constants.h"
#include "logger.h"

/**
 * @brief A mapper that maps camera models to their corresponding type and family, e.g. (spectral, xiSpec),
 * (gray, xiC), etc.
 *
 * This mapper is represented as a constant map with camera models as keys and camera types as values.
 */
const QMap<QString, QMap<QString, QString>> CAMERA_MAPPER = {
        {"MQ022HG-IM-SM4X4-VIS",  {{CAMERA_TYPE_KEY_NAME, CAMERA_TYPE_SPECTRAL}, {CAMERA_FAMILY_KEY_NAME, CAMERA_FAMILY_XISPEC} }},
        {"MQ022HG-IM-SM4X4-VIS3", {{CAMERA_TYPE_KEY_NAME, CAMERA_TYPE_SPECTRAL}, {CAMERA_FAMILY_KEY_NAME, CAMERA_FAMILY_XISPEC} }},
        {"MC050MG-SY-UB",         {{CAMERA_TYPE_KEY_NAME, CAMERA_TYPE_GRAY}, {CAMERA_FAMILY_KEY_NAME, CAMERA_FAMILY_XIC} }}
};


/**
 * Check if XIMEA cameras are connected and counts them
 */
void CameraInterface::Initialize(std::shared_ptr<XiAPIWrapper> apiWrapper){
    int stat = XI_OK;
    this->m_apiWrapper = apiWrapper;
    DWORD numberDevices;
    stat = this->m_apiWrapper->xiGetNumberDevices(&numberDevices);
    HandleResult(stat, "xiGetNumberDevices");
    LOG_SUSICAM(info) << "number of ximea devices found: " << numberDevices;
}


/**
 * \brief Sets the camera type.
 *
 * This function sets the camera type for the camera interface.
 * The camera type is represented by a QString parameter called cameraType.
 *
 * \param cameraType The camera type to be set.
 */
void CameraInterface::SetCameraType(QString cameraType) {
    this->m_cameraType = std::move(cameraType);
}


void CameraInterface::SetCameraFamily(QString cameraFamily) {
    this->m_cameraFamilyName = std::move(cameraFamily);
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
    HandleResult(stat_open, "OpenDevice");

    char cameraSN[100] = {0};
    this->m_apiWrapper->xiGetParamString(this->m_cameraHandle, XI_PRM_DEVICE_SN, cameraSN, sizeof(cameraSN));
    m_cameraSN = QString::fromUtf8(cameraSN);

    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_cameraHandle) {
        LOG_SUSICAM(info) << "Starting acquisition";
        stat = this->m_apiWrapper->xiStartAcquisition(m_cameraHandle);
        HandleResult(stat, "xiStartAcquisition");
        if (stat == XI_OK){
            LOG_SUSICAM(info) << "successfully initialized camera\n";
        }
        else {
            this->CloseDevice();
            throw std::runtime_error("could not start camera, camera acquisition start failed");
        }
    } else {
        throw std::runtime_error("didn't start acquisition, camera invalid handle");
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
    if (INVALID_HANDLE_VALUE != this->m_cameraHandle) {
        LOG_SUSICAM(info) << "Stopping acquisition...";
        stat = this->m_apiWrapper->xiStopAcquisition(m_cameraHandle);
        HandleResult(stat, "xiStopAcquisition");
        LOG_SUSICAM(info) << "Done!";
    }
    return stat;
}


/**
 * \brief Opens a camera device with the specified ID.
 *
 * Opens and initializes the camera device
 *
 * \param cameraIdentifier The camera ID of the camera device to open.
 * \return 0 if the camera device was successfully opened, 1 otherwise.
 */

int CameraInterface::OpenDevice(DWORD cameraIdentifier) {
    int stat = XI_OK;
    stat = this->m_apiWrapper->xiOpenDevice(cameraIdentifier, &m_cameraHandle);
    HandleResult(stat, "xiGetNumberDevices");

    this->setCamera(m_cameraType, m_cameraFamilyName);

    stat = this->m_camera->InitializeCamera();
    if (stat != XI_OK) {
        LOG_SUSICAM(error) << "Failed to initialize camera: " << cameraIdentifier;
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
    if (INVALID_HANDLE_VALUE != this->m_cameraHandle) {
        LOG_SUSICAM(info) << "Closing device";
        stat = this->m_apiWrapper->xiCloseDevice(this->m_cameraHandle);
        HandleResult(stat, "xiCloseDevice");
        //this->m_cameraHandle = INVALID_HANDLE_VALUE;
        LOG_SUSICAM(info) << "Done!";
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
    return this->m_cameraHandle;
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
    // DWORD and HANDLE are defined by xiAPI
    DWORD dwCamCount = 0;
    int stat = XI_OK;
    this->m_apiWrapper->xiGetNumberDevices(&dwCamCount);

    for (DWORD i = 0; i < dwCamCount; i++) {
        HANDLE cameraHandle = INVALID_HANDLE_VALUE;
        stat = this->m_apiWrapper->xiOpenDevice(i, &cameraHandle);
        if (stat != XI_OK){
            LOG_SUSICAM(error) << "cannot open device with ID: " << i << " perhaps already open?";
        } else {
            char cameraModel[256] = {0};
            this->m_apiWrapper->xiGetParamString(cameraHandle, XI_PRM_DEVICE_NAME, cameraModel, sizeof(cameraModel));

            cameraModels.append(QString::fromUtf8(cameraModel));
            m_availableCameras[QString::fromUtf8(cameraModel)] = i;

            this->m_apiWrapper->xiCloseDevice(cameraHandle);
        }
    }
    return cameraModels;
}


/**
 * \brief Destructor of the camera interface.
 */
CameraInterface::~CameraInterface() {
    LOG_SUSICAM(debug) << "CameraInterface::~CameraInterface()";
    this->CloseDevice();
}

/**
 * Sets the camera type as a member variable of camera and camera family members
 *
 * @param cameraType camera type
 * @param cameraFamily camera family
 *
 * @see CAMERA_MAPPER
 */
void CameraInterface::setCamera(QString cameraType, QString cameraFamily) {
    if (cameraType == CAMERA_TYPE_SPECTRAL){
        this->m_cameraFamily = std::make_unique<XiSpecFamily>(&this->m_cameraHandle);
        this->m_camera = std::make_unique<SpectralCamera>(&m_cameraFamily, &this->m_cameraHandle);
        this->m_cameraFamily->m_apiWrapper = this->m_apiWrapper;
        this->m_camera->m_apiWrapper = this->m_apiWrapper;
    }
    else if (cameraType == CAMERA_TYPE_GRAY){
        this->m_cameraFamily = std::make_unique<XiCFamily>(&this->m_cameraHandle);
        this->m_camera = std::make_unique<SpectralCamera>(&m_cameraFamily, &this->m_cameraHandle);
        this->m_cameraFamily->m_apiWrapper = this->m_apiWrapper;
        this->m_camera->m_apiWrapper = this->m_apiWrapper;
    }
}
