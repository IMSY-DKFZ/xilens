/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#ifndef DISPLAY_H
#define DISPLAY_H

#include <QObject>
#include <QString>
#include <xiApi.h>


class Displayer : public QObject {
Q_OBJECT

public:

    explicit Displayer();

    ~Displayer();

    QString m_cameraType;

    virtual void SetCameraType(QString camera_type) = 0;


protected:

    virtual void CreateWindows() = 0;

    virtual void DestroyWindows() = 0;


public slots:

    virtual void Display(XI_IMG &image) = 0;
};

#endif // DISPLAY_H
