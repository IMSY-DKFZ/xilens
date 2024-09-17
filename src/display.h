/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef DISPLAY_H
#define DISPLAY_H

#include <QObject>
#include <QString>
#include <boost/thread.hpp>
#include <opencv2/core.hpp>
#include <xiApi.h>

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

  signals:
    /**
     * Qt signal emitted when an RGB image is ready to be displayed in the UI.
     */
    void ImageReadyToUpdateRGB(cv::Mat &);

    /**
     * Qt signal emitted when a raw image is ready to be displayed in the UI.
     */
    void ImageReadyToUpdateRaw(cv::Mat &);

    /**
     * Qt signal emitted when a new image is ready to compute the saturation percentage for the display.
     */
    void SaturationPercentageImageReady(cv::Mat &);

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
