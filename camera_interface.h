/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#ifndef CAMERA_INTERFACE_H
#define CAMERA_INTERFACE_H


#include "xiApi.h"
#include <opencv2/core/core.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <image_container.h>
#include <QObject>
#include <QString>
#include <QtCore>

#include "constants.h"


class CameraInterface : public QObject
{
    Q_OBJECT

public:
  CameraInterface();
  ~CameraInterface();

  /**
   * set exposure to value in us (40ms=40000us)
   */
  void SetExposure(int exp);
  void SetExposureMs(int exp);

  int GetExposure();
  int GetExposureMs();
  void AutoExposure(bool on);

  int OpenDevice(DWORD camera_sn);
  int StartAcquisition(QString camera_name);
  int StopAcquisition();
  void CloseDevice();
  void UpdateCameraTemperature();
  void SetCameraType(QString camera_type);
  QMap<QString, float> m_cameraTemperature = {
          {CHIP_TEMP,  0.},
          {HOUSE_TEMP, 0.},
          {HOUSE_BACK_TEMP, 0.},
          {SENSOR_BOARD_TEMP, 0.},
  };

    HANDLE GetHandle();

  QMap<QString, DWORD> m_availableCameras;
  QStringList GetAvailableCameraModels();
  QString m_cameraModel;
  QString m_cameraType;


private:
  int InitializeCamera();


  HANDLE m_camHandle;
};

#endif // CAMERA_INTERFACE_H
