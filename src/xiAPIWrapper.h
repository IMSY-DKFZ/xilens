/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/

#ifndef SUSICAM_XIAPIWRAPPER_H
#define SUSICAM_XIAPIWRAPPER_H

#include <xiApi.h>

/**
 * @class XiAPIWrapper
 * @brief This class provides a wrapper for the XiAPI library functions from XIMEA. It can also be overwritten with
 * other implementations such as the Mocks created for unit testing this application.
 */
class XiAPIWrapper
{
public:
    virtual int xiGetParamString(IN HANDLE hDevice, const char* prm, void* val, DWORD size) {
        return ::xiGetParamString(hDevice, prm, val, size);
    }

    virtual int xiGetParamInt(IN HANDLE hDevice, const char* prm, int* val) {
        return ::xiGetParamInt(hDevice, prm, val);
    }

    virtual int xiGetParamFloat(IN HANDLE hDevice, const char* prm, float* val){
        return ::xiGetParamFloat(hDevice, prm, val);
    }

    virtual int xiSetParamInt(IN HANDLE hDevice, const char* prm, const int val){
        return ::xiSetParamInt(hDevice, prm, val);
    }

    virtual int xiSetParamFloat(IN HANDLE hDevice, const char* prm, const float val){
        return ::xiSetParamFloat(hDevice, prm, val);
    }

    virtual int xiOpenDevice(IN DWORD DevId, OUT PHANDLE hDevice){
        return ::xiOpenDevice(DevId, hDevice);
    }

    virtual int xiCloseDevice(IN HANDLE hDevice){
        return ::xiCloseDevice(hDevice);
    }

    virtual int xiGetNumberDevices(OUT PDWORD pNumberDevices){
        return ::xiGetNumberDevices(pNumberDevices);
    }

    virtual int xiStartAcquisition(IN HANDLE hDevice){
        return ::xiStartAcquisition(hDevice);
    }

    virtual int xiStopAcquisition(IN HANDLE hDevice){
        return ::xiStopAcquisition(hDevice);
    }

};

#endif //SUSICAM_XIAPIWRAPPER_H
