/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#include "cameraInterface.h"

#include <boost/log/trivial.hpp>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "constants.h"
#include "logger.h"
#include "util.h"

void CameraInterface::Initialize(std::shared_ptr<XiAPIWrapper> apiWrapper)
{
    int stat = XI_OK;
    this->m_apiWrapper = apiWrapper;
    DWORD numberDevices;
    stat = this->m_apiWrapper->xiGetNumberDevices(&numberDevices);
    HandleResult(stat, "xiGetNumberDevices");
    LOG_SUSICAM(info) << "number of ximea devices found: " << numberDevices;
}

void CameraInterface::SetCameraProperties(QString cameraModel)
{
    QString cameraType = getCameraMapper().value(cameraModel).cameraType;
    QString cameraFamily = getCameraMapper().value(cameraModel).cameraFamily;
    this->m_cameraType = std::move(cameraType);
    this->m_cameraFamilyName = std::move(cameraFamily);
}

void CameraInterface::SetCameraIndex(int index)
{
    this->m_cameraIndex = index;
}

int CameraInterface::StartAcquisition(QString camera_identifier)
{
    int stat_open = OpenDevice(m_availableCameras[camera_identifier]);
    HandleResult(stat_open, "OpenDevice");

    char cameraSN[100] = {0};
    this->m_apiWrapper->xiGetParamString(this->m_cameraHandle, XI_PRM_DEVICE_SN, cameraSN, sizeof(cameraSN));
    this->m_cameraSN = QString::fromUtf8(cameraSN);

    if (INVALID_HANDLE_VALUE != this->m_cameraHandle)
    {
        LOG_SUSICAM(info) << "Starting acquisition";
        int stat = this->m_apiWrapper->xiStartAcquisition(this->m_cameraHandle);
        HandleResult(stat, "xiStartAcquisition");
        LOG_SUSICAM(info) << "successfully initialized camera\n";
        return stat;
    }
    else
    {
        throw std::runtime_error("didn't start acquisition, camera invalid handle");
    }
}

int CameraInterface::StopAcquisition()
{
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_cameraHandle)
    {
        LOG_SUSICAM(info) << "Stopping acquisition...";
        stat = this->m_apiWrapper->xiStopAcquisition(this->m_cameraHandle);
        HandleResult(stat, "xiStopAcquisition");
        LOG_SUSICAM(info) << "Acquisition stopped";
    }
    return stat;
}

int CameraInterface::OpenDevice(DWORD cameraIdentifier)
{
    int stat = XI_OK;
    stat = this->m_apiWrapper->xiOpenDevice(cameraIdentifier, &m_cameraHandle);
    HandleResult(stat, "xiGetNumberDevices");

    this->setCamera(m_cameraType, m_cameraFamilyName);

    stat = this->m_camera->InitializeCamera();
    if (stat != XI_OK)
    {
        LOG_SUSICAM(error) << "Failed to initialize camera: " << cameraIdentifier;
        return stat;
    }
    return stat;
}

void CameraInterface::CloseDevice()
{
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_cameraHandle)
    {
        LOG_SUSICAM(info) << "Closing device";
        stat = this->m_apiWrapper->xiCloseDevice(this->m_cameraHandle);
        this->m_cameraHandle = INVALID_HANDLE_VALUE;
        HandleResult(stat, "xiCloseDevice");
        LOG_SUSICAM(info) << "Done!";
    }
}

HANDLE CameraInterface::GetHandle()
{
    return this->m_cameraHandle;
}

QStringList CameraInterface::GetAvailableCameraModels()
{
    QStringList cameraModels;
    // DWORD and HANDLE are defined by xiAPI
    DWORD dwCamCount = 0;
    this->m_apiWrapper->xiGetNumberDevices(&dwCamCount);

    for (DWORD i = 0; i < dwCamCount; i++)
    {
        HANDLE cameraHandle = INVALID_HANDLE_VALUE;
        int stat = this->m_apiWrapper->xiOpenDevice(i, &cameraHandle);
        if (stat != XI_OK)
        {
            LOG_SUSICAM(error) << "cannot open device with ID: " << i << " perhaps already open?";
        }
        else
        {
            char cameraModel[256] = {0};
            this->m_apiWrapper->xiGetParamString(cameraHandle, XI_PRM_DEVICE_NAME, cameraModel, sizeof(cameraModel));

            cameraModels.append(QString::fromUtf8(cameraModel));
            m_availableCameras[QString::fromUtf8(cameraModel)] = i;

            this->m_apiWrapper->xiCloseDevice(cameraHandle);
        }
    }
    return cameraModels;
}

CameraInterface::~CameraInterface()
{
    LOG_SUSICAM(debug) << "CameraInterface::~CameraInterface()";
    if (this->m_cameraHandle != INVALID_HANDLE_VALUE)
    {
        this->CloseDevice();
    }
}

void CameraInterface::setCamera(QString cameraType, QString cameraFamily)
{
    if (cameraType == CAMERA_TYPE_SPECTRAL)
    {
        this->m_cameraFamily = std::make_unique<XiSpecFamily>(&this->m_cameraHandle);
        this->m_camera = std::make_unique<SpectralCamera>(&m_cameraFamily, &this->m_cameraHandle);
        this->m_cameraFamily->m_apiWrapper = this->m_apiWrapper;
        this->m_camera->m_apiWrapper = this->m_apiWrapper;
    }
    else if (cameraType == CAMERA_TYPE_GRAY)
    {
        this->m_cameraFamily = std::make_unique<XiCFamily>(&this->m_cameraHandle);
        this->m_camera = std::make_unique<GrayCamera>(&m_cameraFamily, &this->m_cameraHandle);
        this->m_cameraFamily->m_apiWrapper = this->m_apiWrapper;
        this->m_camera->m_apiWrapper = this->m_apiWrapper;
    }
    else if (cameraType == CAMERA_TYPE_RGB)
    {
        this->m_cameraFamily = std::make_unique<XiQFamily>(&this->m_cameraHandle);
        this->m_camera = std::make_unique<RGBCamera>(&m_cameraFamily, &this->m_cameraHandle);
        this->m_cameraFamily->m_apiWrapper = this->m_apiWrapper;
        this->m_camera->m_apiWrapper = this->m_apiWrapper;
    }
}
