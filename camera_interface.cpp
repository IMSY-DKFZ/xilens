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


const QMap<QString, QString> CAMERA_TYPE_MAPPER = {
        {"MQ022HG-IM-SM4X4-VIS",  "spectral"},
        {"MQ022HG-IM-SM4X4-VIS3", "spectral"},
        {"MC050MG-SY-UB",         "gray"}
};


void CameraInterface::SetCameraType(QString camera_type) {
    this->m_cameraType = std::move(camera_type);
}


void CameraInterface::UpdateCameraTemperature() {
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

void CameraInterface::AutoExposure(bool on) {
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_camHandle) {
        stat = xiSetParamInt(m_camHandle, XI_PRM_AEAG, on);
        HandleResult(stat, "xiSetParam (autoexposure on/off)");
    } else {
        BOOST_LOG_TRIVIAL(warning) << "autoexposure not set: camera not initialized";
    }
}


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


void CameraInterface::SetExposureMs(int exp) {
    this->SetExposure(exp * 1000);
}


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


int CameraInterface::GetExposureMs() {
    return (this->GetExposure() + 5) / 1000;
}


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

//    stat = xiGetParamInt(m_camHandle, XI_PRM_FRAMERATE XI_PRM_INFO_MAX, &current_max_framerate);
//    HandleResult(stat,"get current maximum frame rate");
//
//    stat = xiSetParamInt(m_camHandle, XI_PRM_ACQ_TIMING_MODE, XI_ACQ_TIMING_MODE_FRAME_RATE);
//    HandleResult(stat,"set acquisition timing mode to framerate");
//
//    stat = xiSetParamInt(m_camHandle, XI_PRM_FRAMERATE, std::min(FRAMERATE_MAX, current_max_framerate));
//    HandleResult(stat,"set maximum frame rate for ultra-fast cameras");

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


int CameraInterface::StartAcquisition(QString camera_name) {
    OpenDevice(m_availableCameras[camera_name]);
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


int CameraInterface::StopAcquisition() {
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_camHandle) {
        printf("Stopping acquisition...");
        stat = xiStopAcquisition(m_camHandle);
        HandleResult(stat, "xiStopAcquisition");
        std::cout << "Done!\n";
    }
    return stat;
}


int CameraInterface::OpenDevice(DWORD camera_sn) {
    int stat = XI_INVALID_HANDLE;

    stat = xiOpenDevice(camera_sn, &m_camHandle);
    HandleResult(stat, "xiGetNumberDevices");

    stat = InitializeCamera();
    HandleResult(stat, "InitializeCamera");

    return stat;
}


void CameraInterface::CloseDevice() {
#ifdef DEBUG_THIS
    std::cout << "DEBUG: CameraInterface::CloseDevice()\n" << std::flush;
#endif

    StopAcquisition();

    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != this->m_camHandle) {
        std::cout << "Closing device...";
        stat = xiCloseDevice(this->m_camHandle);
        HandleResult(stat, "xiCloseDevice");
        //this->m_camHandle = INVALID_HANDLE_VALUE;
        std::cout << "Done!\n" << std::flush;
    }
}


HANDLE CameraInterface::GetHandle() {
    return this->m_camHandle;
}


CameraInterface::CameraInterface() :
        m_camHandle(INVALID_HANDLE_VALUE) {
    int stat = XI_OK;
    DWORD numberDevices;
    stat = xiGetNumberDevices(&numberDevices);
    HandleResult(stat, "xiGetNumberDevices");
    std::cout << "number of ximea devices found: " << numberDevices << "\n" << std::flush;

}


CameraInterface::~CameraInterface() {
#ifdef DEBUG_THIS
    std::cout << "DEBUG: CameraInterface::~CameraInterface()\n" << std::flush;
#endif
    this->CloseDevice();
}


QStringList CameraInterface::GetAvailableCameraModels() {
    QStringList cameraModels;
    QStringList cameraSNs;
    // DWORD and HANDLE are defined by xiAPI
    DWORD dwCamCount = 0;
    xiGetNumberDevices(&dwCamCount);

    for (DWORD i = 0; i < dwCamCount; i++) {
        HANDLE hDevice = INVALID_HANDLE_VALUE;
        xiOpenDevice(i, &hDevice);

        char camera_model[256] = {0};
        xiGetParamString(hDevice, XI_PRM_DEVICE_NAME, camera_model, sizeof(camera_model));

        cameraModels.append(QString::fromUtf8(camera_model));
        m_availableCameras[QString::fromUtf8(camera_model)] = i;

        xiCloseDevice(hDevice);
    }

    return cameraModels;
}
