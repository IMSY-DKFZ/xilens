/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include "camera.h"

#include "logger.h"
#include "util.h"

void CameraFamily::UpdateCameraTemperature()
{
}

QMap<QString, float> CameraFamily::getCameraTemperature()
{
    return this->m_cameraTemperature;
}

int Camera::InitializeCamera()
{
    return 0;
}

int Camera::InitializeCameraCommonParameters()
{
    int current_max_framerate;
    int stat = XI_OK;

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_IMAGE_DATA_FORMAT, XI_RAW16);
    HandleResult(stat, "xiSetParam (data format raw16)");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_RECENT_FRAME, 1);
    HandleResult(stat, "xiSetParam (set to acquire most recent frame)");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_AUTO_BANDWIDTH_CALCULATION, XI_ON);
    HandleResult(stat, "xiSetParam (set auto bandwidth calc to on)");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_GAIN, XI_GAIN_SELECTOR_ALL);
    HandleResult(stat, "xiSetParam (set gain selector to all)");

    stat = this->m_apiWrapper->xiSetParamFloat(*m_cameraHandle, XI_PRM_GAIN, 0.);
    HandleResult(stat, "xiSetParam (set gain to zero)");

    stat = this->m_apiWrapper->xiGetParamInt(*m_cameraHandle, XI_PRM_FRAMERATE XI_PRM_INFO_MAX, &current_max_framerate);
    HandleResult(stat, "get current maximum frame rate");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_FRAMERATE,
                                             std::min(FRAMERATE_MAX, current_max_framerate));
    HandleResult(stat, "set maximum frame rate for ultra-fast cameras");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_DOWNSAMPLING, 1);
    HandleResult(stat, "xiSetParam (no downsampling)");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_COUNTER_SELECTOR,
                                             XI_CNT_SEL_TRANSPORT_SKIPPED_FRAMES);
    HandleResult(stat, "skipping frames on transport layer");

    // check if this creates a problem, I don't think so if buffer is large enough
    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_BUFFER_POLICY, XI_BP_UNSAFE);
    HandleResult(stat, "set unsafe buffuring policy");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_LUT_EN, 0);
    HandleResult(stat, "switch off lut");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_OUTPUT_DATA_PACKING, XI_OFF);
    HandleResult(stat, "disable bit packing");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_ACQ_BUFFER_SIZE, 70 * 1000 * 1000);
    HandleResult(stat, "set acquistion buffer to 70MB. This should give us a buffer of "
                       "about 1s");

    stat = this->m_apiWrapper->xiSetParamFloat(*m_cameraHandle, XI_PRM_EXP_PRIORITY, 1.);
    HandleResult(stat, "if autoexposure is used: only change exposure, not gain");

    this->SetExposure(40000);
    return stat;
}

int SpectralCamera::InitializeCamera()
{
    int stat = XI_OK;

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_ACQ_TIMING_MODE, XI_ACQ_TIMING_MODE_FRAME_RATE);
    HandleResult(stat, "set acquisition timing mode to framerate");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_DOWNSAMPLING_TYPE, XI_BINNING);
    HandleResult(stat, "xiSetParam (downsampling mode set to binning)");

    stat = this->InitializeCameraCommonParameters();
    HandleResult(stat, "set camera common parameters");

    return stat;
}

int GrayCamera::InitializeCamera()
{
    int stat = XI_OK;

    stat =
        this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_ACQ_TIMING_MODE, XI_ACQ_TIMING_MODE_FRAME_RATE_LIMIT);
    HandleResult(stat, "set acquisition timing mode to framerate");

    // XIMEA xiC camera models only allow skipping mode
    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_DOWNSAMPLING_TYPE, XI_SKIPPING);
    HandleResult(stat, "xiSetParam (downsampling mode set to skipping)");

    stat = this->InitializeCameraCommonParameters();
    HandleResult(stat, "set camera common parameters");

    return stat;
}

int RGBCamera::InitializeCamera()
{
    int stat = XI_OK;

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_ACQ_TIMING_MODE, XI_ACQ_TIMING_MODE_FRAME_RATE);
    HandleResult(stat, "set acquisition timing mode to framerate");

    stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_DOWNSAMPLING_TYPE, XI_BINNING);
    HandleResult(stat, "xiSetParam (downsampling mode set to binning)");

    stat = this->InitializeCameraCommonParameters();
    HandleResult(stat, "set camera common parameters");

    return stat;
}

void XiSpecFamily::UpdateCameraTemperature()
{
    float chipTemp, houseTemp, houseBackSideTemp, sensorBoardTemp;

    this->m_apiWrapper->xiGetParamFloat(*m_cameraHandle, XI_PRM_CHIP_TEMP, &chipTemp);
    this->m_apiWrapper->xiGetParamFloat(*m_cameraHandle, XI_PRM_HOUS_TEMP, &houseTemp);
    this->m_apiWrapper->xiGetParamFloat(*m_cameraHandle, XI_PRM_HOUS_BACK_SIDE_TEMP, &houseBackSideTemp);
    this->m_apiWrapper->xiGetParamFloat(*m_cameraHandle, XI_PRM_SENSOR_BOARD_TEMP, &sensorBoardTemp);
    this->m_cameraTemperature[CHIP_TEMP] = chipTemp;
    this->m_cameraTemperature[HOUSE_TEMP] = houseTemp;
    this->m_cameraTemperature[HOUSE_BACK_TEMP] = houseBackSideTemp;
    this->m_cameraTemperature[SENSOR_BOARD_TEMP] = sensorBoardTemp;
}

void XiCFamily::UpdateCameraTemperature()
{
    float sensorBoardTemp;
    this->m_apiWrapper->xiGetParamFloat(*m_cameraHandle, XI_PRM_SENSOR_BOARD_TEMP, &sensorBoardTemp);
    this->m_cameraTemperature[SENSOR_BOARD_TEMP] = sensorBoardTemp;
}

void XiQFamily::UpdateCameraTemperature()
{
    float chipTemp, houseTemp, houseBackSideTemp, sensorBoardTemp;
    if (*m_cameraHandle != INVALID_HANDLE_VALUE)
    {
        this->m_apiWrapper->xiGetParamFloat(*m_cameraHandle, XI_PRM_CHIP_TEMP, &chipTemp);
        this->m_apiWrapper->xiGetParamFloat(*m_cameraHandle, XI_PRM_HOUS_TEMP, &houseTemp);
        this->m_apiWrapper->xiGetParamFloat(*m_cameraHandle, XI_PRM_HOUS_BACK_SIDE_TEMP, &houseBackSideTemp);
        this->m_apiWrapper->xiGetParamFloat(*m_cameraHandle, XI_PRM_SENSOR_BOARD_TEMP, &sensorBoardTemp);
        this->m_cameraTemperature[CHIP_TEMP] = chipTemp;
        this->m_cameraTemperature[HOUSE_TEMP] = houseTemp;
        this->m_cameraTemperature[HOUSE_BACK_TEMP] = houseBackSideTemp;
        this->m_cameraTemperature[SENSOR_BOARD_TEMP] = sensorBoardTemp;
    }
}

void Camera::SetExposure(int exp)
{
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != *m_cameraHandle)
    {
        // Setting "exposure" parameter (10ms=10000us)
        stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_EXPOSURE, exp);
        HandleResult(stat, "xiSetParam (exposure set)");
        LOG_XILENS(info) << "set exposure to " << exp / 1000 << "ms\n" << std::flush;
    }
    else
    {
        LOG_XILENS(warning) << "exposure not set: camera not initialized";
    }
}

void Camera::SetExposureMs(int exp)
{
    this->SetExposure(exp * 1000);
}

int Camera::GetExposure()
{
    int stat = XI_OK;
    int exp = 40000;
    if (INVALID_HANDLE_VALUE != *m_cameraHandle)
    {
        // Setting "exposure" parameter (10ms=10000us)
        stat = this->m_apiWrapper->xiGetParamInt(*m_cameraHandle, XI_PRM_EXPOSURE, &exp);
        HandleResult(stat, "xiGetParam (exposure get)");
    }
    else
    {
        LOG_XILENS(warning) << "exposure not determined, camera not initalized. "
                               "Return standard value.";
    }

    return exp;
}

int Camera::GetExposureMs()
{
    return (this->GetExposure() + 5) / 1000;
}

void Camera::AutoExposure(bool on)
{
    int stat = XI_INVALID_HANDLE;
    if (INVALID_HANDLE_VALUE != *m_cameraHandle)
    {
        stat = this->m_apiWrapper->xiSetParamInt(*m_cameraHandle, XI_PRM_AEAG, on);
        HandleResult(stat, "xiSetParam (autoexposure on/off)");
    }
    else
    {
        LOG_XILENS(warning) << "autoexposure not set: camera not initialized";
    }
}
