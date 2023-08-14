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

extern const QMap<QString, QString> CAMERA_TYPE_MAPPER;
const QString SPECTRAL_CAMERA = "spectral";
const QString GRAY_CAMERA = "gray";

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

  HANDLE GetHandle();

  QMap<QString, DWORD> m_availableCameras;
  QStringList GetAvailableCameraModels();
  QString m_cameraModel;


private:
  int InitializeCamera();


  HANDLE m_camHandle;
};

#endif // CAMERA_INTERFACE_H
