/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef DISPLAY_H
#define DISPLAY_H

#include <xiApi.h>

#include <QObject>
#include <QString>
#include <boost/thread.hpp>

class Displayer : public QObject
{
    Q_OBJECT

  public:
    explicit Displayer(QObject *parent = nullptr);

    ~Displayer() override;

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
    /**
     * Indicate that process should stop displaying images.
     */
    bool m_stop = false;

    /**
     * condition variable used to wait until a new image is available to be processed.
     */
    boost::condition_variable m_displayCondition;

  public slots:

    /**
     * Qt slot in charge of displaying images.
     * @param image
     */
    virtual void Display(XI_IMG &image) = 0;
};

#endif // DISPLAY_H
