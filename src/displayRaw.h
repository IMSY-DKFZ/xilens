/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#ifndef DISPLAYRAW_H
#define DISPLAYRAW_H

#include <string>

#include <QObject>
#include <xiApi.h>
#include <opencv2/core/core.hpp>
#include <boost/thread.hpp>

#include "display.h"
#include "constants.h"
#include "mainwindow.h"


class MainWindow;

class DisplayerRaw : public Displayer {
Q_OBJECT

public:

    /**
     * camera type
     */
    QString m_cameraType;

    /**
     * Constructor of raw displayer
     *
     * @param mainWindow reference to main window application
     */
    explicit DisplayerRaw(MainWindow *mainWindow);

    /**
     * Constructor of raw displayer
     *
     * @param mainWindow reference to main window application
     */
    explicit DisplayerRaw() : Displayer() {};

    /**
     * Draws histogram based of displayed image based on an region of interest (ROI)
     *
     * @param roiImg region of interest where histogram is computed
     */
    void DrawHistogram(const cv::Mat roiImg);

    /**
     * Destroys the displayer and all windows associated with it
     */
    ~DisplayerRaw() override;

    /**
     * Sets camera type
     *
     * @param cameraModel camera type to set
     */
    void SetCameraProperties(QString cameraModel) override;

    /**
     * Down-samples image in case it is bigger than maximum dimensions defined by constants::MAX_WIDTH_DISPLAY_WINDOW
     * and constants::MAX_HEIGHT_DISPLAY_WINDOW
     *
     * @param image image to be downsampled
     */
    static void DownsampleImageIfNecessary(cv::Mat &image);


protected:
    /**
     * reference to main window, necessary to detect if normalization is turned on / which band to display
     */
    MainWindow* m_mainWindow;

    /**
     * Creates OpenCV windows where images will be displayed
     *
     * @see DestroyWindows
     */
    void CreateWindows() override;

    /**
     * Destroys allOpenCV windows
     *
     * @see CreateWindows
     */
    void DestroyWindows() override;

    /**
     * Prepares images before displaying them by downsampling them if necessary and converting them to the correct bit
     * depth.
     *
     * @param image image to be processed
     * @return processed image
     */
    virtual cv::Mat PrepareImageToDisplay(cv::Mat &image);


public slots:

    /**
     * Qt slot called whenever a new image arrives from the camera
     *
     * @param image image to be displayed
     */
    void Display(XI_IMG &image) override;


private:

    /**
     * Displays an image to an OpenCV window defined by the window name
     *
     * @param image image to be displayed
     * @param windowName name of the window where image will be displayed
     */
    void DisplayImage(cv::Mat &image, const std::string windowName);

    /**
     * explicit call to mutex
     */
    boost::mutex mtx_;

    /**
     * Image to be displayed
     */
    cv::Mat m_ImageToDisplay;

    /**
     * Raw image before any pre-processing
     */
    cv::Mat m_rawImage;
};

#endif // DISPLAY_H
