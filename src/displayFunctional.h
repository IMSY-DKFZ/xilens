/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef DISPLAYFUNCTIONAL_H
#define DISPLAYFUNCTIONAL_H

#include <xiApi.h>

#include <QObject>
#include <boost/thread.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <string>

#include "constants.h"
#include "display.h"
#include "mainwindow.h"
#include "util.h"

class MainWindow;

/**
 * @brief Enumerates the types of display images.
 *
 * The DisplayImageType enumerator represents the different types of display
 * images. It provides symbolic names for the supported image types.
 */
enum DisplayImageType
{
    RAW = 0,
    RGB = 1,
    VHB = 2,
    OXY = 3
};

/**
 * @brief The DisplayerFunctional class is responsible for displaying images.
 *
 * It inherits from the Displayer class and uses the MainWindow class for
 * reference. It displays the raw, RGB and functional estimated images based on
 * the images collected from the camera.
 */
class DisplayerFunctional : public Displayer
{
    Q_OBJECT

  public:
    /**
     * Constructor of functional displayer
     *
     * @param mainWindow reference to main window application
     */
    explicit DisplayerFunctional(MainWindow *mainWindow);

    /**
     * Constructor of functional displayer
     *
     * @param mainWindow reference to main window application
     */
    explicit DisplayerFunctional() : Displayer(){};

    /**
     * Destructor of the DisplayerFunctional class. It destroy the windows created
     * by the displayer.
     */
    ~DisplayerFunctional() override;

    /**
     * Assigns camera properties such as type and mosaic shape.
     *
     * @param cameraModel camera type to set
     */
    void SetCameraProperties(QString cameraModel) override;

    /**
     * Down-samples image in case it is bigger than maximum dimensions defined by
     * constants::MAX_WIDTH_DISPLAY_WINDOW and
     * constants::MAX_HEIGHT_DISPLAY_WINDOW
     *
     * @param image image to be downsampled
     */
    void DownsampleImageIfNecessary(cv::Mat &image);

    /**
     * type of camera being used: spectral, gray, etc.
     */
    QString m_cameraType = CAMERA_TYPE_SPECTRAL;

    /**
     * Mosaic shape, particularly used for mosaic type cameras
     */
    std::vector<int> m_mosaicShape;

    /**
     * Look up table used to assign pixel colors to undersaturated and
     * oversaturated pixels
     */
    cv::Mat m_lut = CreateLut(SATURATION_COLOR, DARK_COLOR);

  protected:
    /**
     * reference to main window, necessary to detect if normalization is turned on
     * / which band to display
     */
    MainWindow *m_mainWindow;

    /**
     * @brief Creates windows where images are displayed
     *
     * @see DisplayerFunctional::DestroyWindows
     */
    void CreateWindows() override;

    /**
     * Destroys windows created by CreateWindows
     *
     * @see DisplayerFunctional::CreateWindows
     */
    void DestroyWindows() override;

  public slots:

    /**
     * Qt slot used to display images whenever a new image is queried from the
     * camera
     *
     * @param image image to be displayed
     */
    void Display(XI_IMG &image) override;

  private:
    /**
     * Vector with channel numbers that can be used to construct an approximate
     * RGB image
     */
    std::vector<int> m_bgr_channels = {11, 15, 3};

    /**
     * Scaling factor used to convert image from 10bit to 8bit
     */
    int m_scaling_factor = 4;

    /**
     * Dummy method, not yet implemented
     *
     * @param image Image to be run through the network
     */
    void RunNetwork(XI_IMG &image);

    /**
     * @brief Displays image on screen
     *
     * @param image the image to be displayed
     * @param windowName the name of the window for displaying
     */
    void DisplayImage(cv::Mat &image, const std::string windowName);

    /**
     * explicit mutex declaration
     */
    boost::mutex mtx_;

    /**
     * Class to do histogram normalization with CLAHE
     */
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();

    /**
     * @brief prepares raw image from XIMEA camera to be displayed, it does
     * histogram normalization in case it is specified
     *
     * @param raw_image, the image to be processed
     */
    void PrepareRawImage(cv::Mat &raw_image, bool equalize_hist);

    /**
     * @brief Normalizes a BGR image using the LAB color space.
     *
     * This function converts the input BGR image to the LAB color space and then
     * applies Contrast Limited Adaptive Histogram Equalization (CLAHE) to the L
     * channel. The resulting L channel is then merged with the original A and B
     * channels to obtain the normalized LAB image. Finally, the LAB image is
     * converted back to the BGR color space and replaces the original BGR image.
     *
     * @param bgr_image The BGR image to be normalized. Note that the input image
     * will be modified.
     */
    void NormalizeBGRImage(cv::Mat &bgr_image);

    /**
     * @brief Extracts a specific band (channel) from an image
     *
     * Band_image is converted to an 8-bit image and divided by the scaling factor
     * to convert it from 10-bit to 8-bit.
     *
     * @param image The input image
     * @param band_image The output band image
     * @param band_nr The number of the band to extract
     */
    void GetBand(cv::Mat &image, cv::Mat &band_image, unsigned int band_nr);

    /**
     * @brief Get the BGR image from the input image by splitting it into separate
     * channels, applying band filters and merging the channels.
     *
     * This function takes an input image and extracts the specified channels to
     * form a BGR image. Each channel is extracted using the GetBand() function
     * and stored in a vector.
     *
     * @param image The input image from which channels will be extracted.
     * @param bgr_image The output BGR image.
     */
    void GetBGRImage(cv::Mat &image, cv::Mat &rgb_image);
};

/**
 * @brief Prepares a functional image for display.
 *
 * The PrepareFunctionalImage function prepares a functional image for display.
 * It performs scaling, clamping, rescaling, and applies a colormap to the
 * image.
 *
 * @param functional_image The functional image that needs to be prepared for
 * display.
 * @param displayImage The type of display image (RAW, RGB, VHB, OXY).
 * @param do_scaling A flag indicating whether scaling should be applied to the
 * image.
 * @param bounds The range of values for clamping and scaling the image.
 * @param colormap The colormap type to be applied to the image.
 */
void PrepareFunctionalImage(cv::Mat &functional_image, DisplayImageType displayImage, bool do_scaling, cv::Range bounds,
                            int colormap);

/**
 * @brief Normalize and convert an input BGR image to 8-bit unsigned integer
 * format.
 *
 * This function takes an input BGR (Blue-Green-Red) image represented by a
 * cv::Mat object and applies normalization and conversion operations. The
 * normalization process calculates the minimum and maximum values of the input
 * image and scales the image values to fit within a specified normalization
 * range. The resulting image is then converted to 8-bit unsigned integer
 * format, such that its pixel values range from 0 to 255.
 *
 * @param bgr_image The input BGR image to be processed. The image gets modified
 * in-place.
 * @param bgr_norm The normalization factor to adjust the image intensity range.
 * The higher the value, the larger the intensity range of the resulting image.
 */
void PrepareBGRImage(cv::Mat &bgr_image, int bgr_norm);

#endif // DISPLAY_H