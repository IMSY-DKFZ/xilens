/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef DISPLAY_H
#define DISPLAY_H

#include <xiApi.h>

#include <QObject>
#include <QString>

class Displayer : public QObject
{
    Q_OBJECT

  public:
    explicit Displayer();

    ~Displayer();

    QString m_cameraType;

    virtual void SetCameraProperties(QString cameraModel) = 0;

    /**
     *  Blocks the display of images
     */
    void StopDisplayer();

    /**
     * Allows to start or continue displaying images
     */
    void StartDisplayer();

  protected:
    bool m_stop = false;

  public slots:

    virtual void Display(XI_IMG &image) = 0;
};

#endif // DISPLAY_H
