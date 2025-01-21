/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef DISPLAY_H
#define DISPLAY_H

#include <QImage>
#include <QObject>
#include <QString>
#include <boost/thread.hpp>
#include <xiApi.h>

/**
 * @brief Base class used to display images queried from each camera.
 *
 * This class implements several `Qt` signals that are triggered when new images are ready to be displayed.
 * For example, RGB, raw, and saturation percentages for each image.
 *
 * Whether images are to be displayed or not are also controlled through the variable Displayer::m_stop.
 */
class Displayer : public QObject
{
    Q_OBJECT

  public:
    explicit Displayer(QObject *parent = nullptr);

    ~Displayer() override;

    /**
     * @brief Stores the type or model of the camera currently used.
     */
    QString m_cameraType;

    /**
     * @brief Configures the camera properties based on the provided camera model.
     *
     * This pure virtual method must be implemented by derived classes to initialize
     * or modify display-related behaviors or settings according to the specified camera model.
     *
     * @param cameraModel A string representing the model of the camera, used
     */
    virtual void SetCameraProperties(QString cameraModel) = 0;

    /**
     *  @brief Blocks the display of images
     */
    void StopDisplayer();

    /**
     * @brief Allows to start or continue displaying images
     */
    void StartDisplayer();

    /**
     * @brief Updates the lookup table (LUT) with the specified minimum and maximum values.
     *
     * This method adjusts the LUT range to enhance or modify the image display settings based
     * on the given values. It allows flexibility in defining the lower and upper bounds of the LUT.
     *
     * @param minValue The minimum value for the LUT.
     * @param maxValue The maximum value for the LUT.
     */
    virtual void UpdateLut(int minValue, int maxValue);

  signals:
    /**
     * @brief Qt signal emitted when an RGB image is ready to be displayed in the UI.
     */
    void ImageReadyToUpdateRGB(QImage);

    /**
     * @brief Qt signal emitted when a raw image is ready to be displayed in the UI.
     */
    void ImageReadyToUpdateRaw(QImage);

    /**
     * @brief Qt signal emitted when the saturation values are ready to be displayed in the UI.
     */
    void SaturationPercentageReady(double undersaturation, double oversaturation);

  protected:
    /**
     * @brief Indicate that process should stop displaying images.
     */
    bool m_stop = false;

    /**
     * @brief Condition variable used to wait until a new image is available to be processed.
     */
    boost::condition_variable m_displayCondition;

  public slots:

    /**
     * @brief Qt slot in charge of displaying images.
     *
     * @param image
     */
    virtual void Display(XI_IMG &image) = 0;
};

#endif // DISPLAY_H
