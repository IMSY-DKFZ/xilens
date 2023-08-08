/*
 * ===================================================================
 * Surgical Spectral Imaging Library (SuSI)
 *
 * Copyright (c) German Cancer Research Center,
 * Division of Medical and Biological Informatics.
 * All rights reserved.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 *
 * See LICENSE.txt for details.
 * ===================================================================
 */


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
#include <QtCore>


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

  int OpenDevice(const char* camera_sn);
  int StartAcquisition(QString camera_name);
  int StopAcquisition();
  void CloseDevice();

  HANDLE GetHandle();

  QMap<QString, const char*> m_availableCameras;
  QStringList GetAvailableCameraModels();


private:
  int InitializeCamera();


  HANDLE m_camHandle;
};

#endif // CAMERA_INTERFACE_H
