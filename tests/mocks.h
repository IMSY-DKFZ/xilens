/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/

#ifndef SUSICAM_MOCKS_H
#define SUSICAM_MOCKS_H

#include <xiApi.h>

#include "src/xiAPIWrapper.h"
#include "src/displayFunctional.h"
#include "src/displayRaw.h"
#include "src/mainwindow.h"

/**
 * Mock class for the XIMEA API, return values that are dummy variables to remove the need for the camera when doing
 * unittests.
 */
class MockXiAPIWrapper : public XiAPIWrapper{
public:
    int xiGetParamString(IN HANDLE hDevice, const char* prm, void* val, DWORD size) override {
        return 0;
    }

    int xiGetParamInt(IN HANDLE hDevice, const char* prm, int* val) override {
        return 0;
    }

    int xiGetParamFloat(IN HANDLE hDevice, const char* prm, float* val) override {
        return 0;
    }

    int xiSetParamInt(IN HANDLE hDevice, const char* prm, const int val) override {
        return 0;
    }

    int xiSetParamFloat(IN HANDLE hDevice, const char* prm, const float val) override {
        return 0;
    }

    int xiOpenDevice(IN DWORD DevId, OUT PHANDLE hDevice) override {
        return 0;
    }

    int xiCloseDevice(IN HANDLE hDevice) override {
        return 0;
    }

    int xiGetNumberDevices(OUT PDWORD pNumberDevices) override {
        return 0;
    }

    int xiStartAcquisition(IN HANDLE hDevice) override {
        return 0;
    }

    int xiStopAcquisition(IN HANDLE hDevice) override {
        return 0;
    }

};


/**
 * Mock class of the main window to be able to test the displayer functions without needing the camera
 */
class MockMainWindow: public MainWindow{
public:
    explicit MockMainWindow(QWidget *parent = nullptr) : MainWindow(parent, std::make_shared<MockXiAPIWrapper>()){

    }

    unsigned GetBand() const override {
        return 8;
    }
};


/**
 * Mock of the Functional displayer class to be able to test it without needing the camera
 */
class MockDisplayerFunctional: public DisplayerFunctional{
public:
    MockDisplayerFunctional() : DisplayerFunctional() {
        m_mainWindow = new MockMainWindow();
    };

    ~MockDisplayerFunctional() override = default;

};


/**
 * Mock of the Functional displayer class to be able to test it without needing the camera
 */
class MockDisplayerRaw: public DisplayerRaw{
public:
    MockDisplayerRaw() : DisplayerRaw() {
        m_mainWindow = new MockMainWindow();
    };

    ~MockDisplayerRaw() override = default;

};

#endif //SUSICAM_MOCKS_H
