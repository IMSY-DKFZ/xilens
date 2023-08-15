/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#ifndef DISPLAYFUNCTIONAL_H
#define DISPLAYFUNCTIONAL_H

#include <string>

#include <boost/thread.hpp>
#include <QObject>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>

#include "display.h"
#include "xiApi.h"
#include "constants.h"
#include "util.h"

class MainWindow;

enum DisplayImageType {RAW=0, RGB=1, VHB=2, OXY=3};

class DisplayerFunctional : public Displayer
{
    Q_OBJECT

public:

    explicit DisplayerFunctional(MainWindow* mainWindow);

    ~DisplayerFunctional();

    QString m_cameraType = SPECTRAL_CAMERA;

    cv::Mat m_lut = CreateLut(SATURATION_COLOR, DARK_COLOR);

    void SetCameraType(QString camera_type);

    void DownsampleImageIfNecessary(cv::Mat& image);

protected:

    void CreateWindows();
    void DestroyWindows();


public slots:

    virtual void Display(XI_IMG& image);


private:
    std::vector<int> m_bgr_channels = {11, 15, 3};
    int m_scaling_factor = 4;

    // run the deep net to be able to display results
    void RunNetwork(XI_IMG& image);

    /**
     * @brief DisplayImage General purpose display image function
     *
     * @param image the image to be displayed
     * @param windowName the name of the window for displaying
     */
    void DisplayImage(cv::Mat& image, const std::string windowName);

    // TODO: needs rework
    friend void OcvCallBackFunc(int event, int x, int y, int flags, void* userdata);

    // reference to mainwindow, necessary to detect if normalization is turned on / which band to display
    const MainWindow  * const m_mainWindow;

    boost::mutex mtx_; // explicit mutex declaration

    // cv::CLAHE class to do histogram normalization
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();

    /**
     * @brief prepares raw image from XIMEA camera to be displayed, it does histogram normalization in case it is specified
     *
     * @param raw_image, the image to be processed
     */
    void PrepareRawImage(cv::Mat& raw_image, bool equalize_hist);

    void NormalizeBGRImage(cv::Mat& bgr_image);

    void GetBand(cv::Mat& image, cv::Mat& band_image, int band_nr);

    void GetBGRImage(cv::Mat &image, cv::Mat &rgb_image);

};

// helper function to do proper formatting of functional image
void PrepareFunctionalImage(cv::Mat& functional_image, DisplayImageType displayImage, bool do_scaling, cv::Range bounds, int colormap);

void PrepareBGRImage(cv::Mat& bgr_image, int bgr_norm);

void PrepareRawImage(cv::Mat& raw_image, bool equalize_hist);

#endif // DISPLAY_H
