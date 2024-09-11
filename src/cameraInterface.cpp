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
    LOG_XILENS(info) << "number of ximea devices found: " << numberDevices;
}

void CameraInterface::SetCameraProperties(QString cameraModel)
{
    if (!getCameraMapper().contains(cameraModel))
    {
        LOG_XILENS(error) << "Could not find camera model in Mapper: " << cameraModel.toStdString();
        throw std::runtime_error("Could not find camera in Mapper");
    }
    this->m_cameraType = getCameraMapper().value(cameraModel).cameraType;
    this->m_cameraFamilyName = getCameraMapper().value(cameraModel).cameraFamily;
}

void CameraInterface::SetCameraIndex(int index)
{
    this->m_cameraIndex = index;
}

int CameraInterface::StartAcquisition(QString cameraIdentifier)
{
    if (!m_availableCameras.contains(cameraIdentifier))
    {
        LOG_XILENS(error) << "camera identifier not in mapper: " << cameraIdentifier.toStdString();
        throw std::runtime_error("Camera identifier not found in Mapper");
    }
    int stat_open = OpenDevice(m_availableCameras[cameraIdentifier]);
    HandleResult(stat_open, "OpenDevice");
    auto openedCameraIdentifier = GetCameraIdentifier(m_cameraHandle);
    if (openedCameraIdentifier != cameraIdentifier)
    {
        LOG_XILENS(error) << "Opened camera not the same as selected camera: " << cameraIdentifier.toStdString()
                          << "!=" << cameraIdentifier.toStdString();
        throw std::runtime_error("Opened camera is not the same as the selected one.");
    }

    char cameraSN[100] = {0};
    this->m_apiWrapper->xiGetParamString(this->m_cameraHandle, XI_PRM_DEVICE_SN, cameraSN, sizeof(cameraSN));
    this->m_cameraSN = QString::fromUtf8(cameraSN);

    if (INVALID_HANDLE_VALUE != this->m_cameraHandle)
    {
        LOG_XILENS(info) << "Starting acquisition";
        int stat = this->m_apiWrapper->xiStartAcquisition(this->m_cameraHandle);
        HandleResult(stat, "xiStartAcquisition");
        LOG_XILENS(info) << "successfully initialized camera\n";
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
        LOG_XILENS(info) << "Stopping acquisition...";
        stat = this->m_apiWrapper->xiStopAcquisition(this->m_cameraHandle);
        HandleResult(stat, "xiStopAcquisition");
        LOG_XILENS(info) << "Acquisition stopped";
    }
    return stat;
}

int CameraInterface::OpenDevice(DWORD cameraDeviceID)
{
    int stat = XI_OK;
    stat = this->m_apiWrapper->xiOpenDevice(cameraDeviceID, &m_cameraHandle);
    HandleResult(stat, "xiOepnDevice");

    this->SetCamera(m_cameraType, m_cameraFamilyName);

    stat = this->m_camera->InitializeCamera();
    if (stat != XI_OK)
    {
        LOG_XILENS(error) << "Failed to initialize camera: " << cameraDeviceID;
        return stat;
    }
    return stat;
}

void CameraInterface::CloseDevice()
{
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_cameraHandle)
    {
        LOG_XILENS(info) << "Closing device";
        stat = this->m_apiWrapper->xiCloseDevice(this->m_cameraHandle);
        this->m_cameraHandle = INVALID_HANDLE_VALUE;
        HandleResult(stat, "xiCloseDevice");
        LOG_XILENS(info) << "Done!";
    }
}

HANDLE CameraInterface::GetHandle()
{
    return this->m_cameraHandle;
}

QStringList CameraInterface::GetAvailableCameraIdentifiers()
{
    m_availableCameras.clear();
    QStringList cameraIdentifiers;
    // DWORD and HANDLE are defined by xiAPI
    DWORD dwCamCount = 0;
    this->m_apiWrapper->xiGetNumberDevices(&dwCamCount);

    for (DWORD i = 0; i < dwCamCount; i++)
    {
        HANDLE cameraHandle = INVALID_HANDLE_VALUE;
        int stat = this->m_apiWrapper->xiOpenDevice(i, &cameraHandle);
        if (stat != XI_OK)
        {
            LOG_XILENS(error) << "cannot open device with ID: " << i << " perhaps already open?";
        }
        else
        {
            auto cameraIdentifier = GetCameraIdentifier(cameraHandle);
            cameraIdentifiers.append(cameraIdentifier);
            m_availableCameras[cameraIdentifier] = i;

            this->m_apiWrapper->xiCloseDevice(cameraHandle);
        }
    }
    return cameraIdentifiers;
}

QString CameraInterface::GetCameraIdentifier(HANDLE cameraHandle)
{
    char cameraModel[256] = {0};
    char sensorSN[100] = "";
    this->m_apiWrapper->xiGetParamString(cameraHandle, XI_PRM_DEVICE_NAME, cameraModel, sizeof(cameraModel));
    this->m_apiWrapper->xiGetParamString(cameraHandle, XI_PRM_DEVICE_SENS_SN, sensorSN, sizeof(sensorSN));
    QString cameraIdentifier = QString("%1@%2").arg(QString::fromUtf8(cameraModel), QString::fromUtf8(sensorSN));
    return cameraIdentifier;
}

CameraInterface::~CameraInterface()
{
    LOG_XILENS(debug) << "Destroying camera interface";
    if (this->m_cameraHandle != INVALID_HANDLE_VALUE)
    {
        this->CloseDevice();
    }
}

void CameraInterface::SetCamera(QString cameraType, QString cameraFamily)
{
    // instantiate camera type
    if (cameraType == CAMERA_TYPE_SPECTRAL)
    {
        this->m_camera = std::make_unique<SpectralCamera>(&m_cameraFamily, &this->m_cameraHandle);
    }
    else if (cameraType == CAMERA_TYPE_GRAY)
    {
        this->m_camera = std::make_unique<GrayCamera>(&m_cameraFamily, &this->m_cameraHandle);
    }
    else if (cameraType == CAMERA_TYPE_RGB)
    {
        this->m_camera = std::make_unique<RGBCamera>(&m_cameraFamily, &this->m_cameraHandle);
    }
    // instantiate camera family
    if (cameraFamily == CAMERA_FAMILY_XISPEC)
    {
        this->m_cameraFamily = std::make_unique<XiSpecFamily>(&this->m_cameraHandle);
    }
    else if (cameraFamily == CAMERA_FAMILY_XIC)
    {
        this->m_cameraFamily = std::make_unique<XiCFamily>(&this->m_cameraHandle);
    }
    else if (cameraFamily == CAMERA_FAMILY_XIQ)
    {
        this->m_cameraFamily = std::make_unique<XiQFamily>(&this->m_cameraHandle);
    }
    else if (cameraFamily == CAMERA_FAMILY_XIB)
    {
        this->m_cameraFamily = std::make_unique<XiBFamily>(&this->m_cameraHandle);
    }
    else if (cameraFamily == CAMERA_FAMILY_XIB64)
    {
        this->m_cameraFamily = std::make_unique<XiB64Family>(&this->m_cameraHandle);
    }
    else if (cameraFamily == CAMERA_FAMILY_XIRAY)
    {
        this->m_cameraFamily = std::make_unique<XiRAYFamily>(&this->m_cameraHandle);
    }
    else if (cameraFamily == CAMERA_FAMILY_XIX)
    {
        this->m_cameraFamily = std::make_unique<XiXFamily>(&this->m_cameraHandle);
    }
    // instantiate API wrapper
    this->m_cameraFamily->m_apiWrapper = this->m_apiWrapper;
    this->m_camera->m_apiWrapper = this->m_apiWrapper;
}
