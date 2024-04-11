/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include <iostream>
#include <string>
#include <utility>

#include <boost/thread.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#if CV_VERSION_MAJOR == 3
#include <opencv2/videoio.hpp>
#endif
#if __has_include(<opencv2/contrib/contrib.hpp>)
#  include <opencv2/contrib/contrib.hpp>
#else
#  define opencv2_has_contrib 0
#endif

#include "displayFunctional.h"
#include "mainwindow.h"
#include "util.h"
#include "constants.h"
#include "logger.h"

/**
 * custom type that defines a pixel in an OpenCV image
 */
typedef cv::Point3_<uint8_t> Pixel;


/**
 * @brief The DisplayerFunctional class is responsible for displaying images.
 * It inherits from the Displayer class and uses the MainWindow class for reference.
 *
 * The DisplayerFunctional constructor initializes the DisplayerFunctional object with a pointer
 * to the MainWindow object. It calls the CreateWindows() method to create windows for displaying images.
 *
 * @param mainWindow a pointer to the MainWindow object
 */
DisplayerFunctional::DisplayerFunctional(MainWindow *mainWindow) : Displayer(), m_mainWindow(mainWindow) {
    CreateWindows();
}


/**
 * @brief Destructor for the DisplayerFunctional class.
 *
 * This destructor destroys all windows created by the DisplayerFunctional class.
 * It calls the OpenCV function destroyAllWindows() to close all windows.
 */
DisplayerFunctional::~DisplayerFunctional() {
    DestroyWindows();
}


/**
 * @brief Prepares a functional image for display.
 *
 * The PrepareFunctionalImage function prepares a functional image for display.
 * It performs scaling, clamping, rescaling, and applies a colormap to the image.
 *
 * @param functional_image The functional image that needs to be prepared for display.
 * @param displayImage The type of display image (RAW, RGB, VHB, OXY).
 * @param do_scaling A flag indicating whether scaling should be applied to the image.
 * @param bounds The range of values for clamping and scaling the image.
 * @param colormap The colormap type to be applied to the image.
 */
void PrepareFunctionalImage(cv::Mat &functional_image, [[maybe_unused]]DisplayImageType displayImage, bool do_scaling,
                            cv::Range bounds, int colormap) {
    if (do_scaling) {
        functional_image *= 100;
        // set first and second pixel to extreme values so the colormap is always scaled the same
        functional_image.at<float>(0, 0) = bounds.start;
        functional_image.at<float>(0, 1) = bounds.end;
        clamp(functional_image, bounds);
    }

    rescale(functional_image, 255.);
    functional_image.convertTo(functional_image, CV_8UC1);
    applyColorMap(functional_image, functional_image, colormap);
}


/**
 * @brief Normalize and convert an input BGR image to 8-bit unsigned integer format.
 *
 * This function takes an input BGR (Blue-Green-Red) image represented by a cv::Mat object
 * and applies normalization and conversion operations. The normalization process calculates
 * the minimum and maximum values of the input image and scales the image values to fit within
 * a specified normalization range. The resulting image is then converted to 8-bit unsigned integer format,
 * such that its pixel values range from 0 to 255.
 *
 * @param bgr_image The input BGR image to be processed. The image gets modified in-place.
 * @param bgr_norm The normalization factor to adjust the image intensity range. The higher the value,
 *                 the larger the intensity range of the resulting image. Values typically range from 0 to 100.
 */
void PrepareBGRImage(cv::Mat &bgr_image, int bgr_norm) {
    double min, max;
    static double last_norm = 1.;
    cv::minMaxLoc(bgr_image, &min, &max);

    last_norm = 0.9 * last_norm + (double) bgr_norm * 0.01 * max;

    bgr_image *= 255. / last_norm;
    bgr_image.convertTo(bgr_image, CV_8UC3);
}


/**
 * @brief NormalizeBGRImage Normalizes a BGR image using the LAB color space.
 *
 * This function converts the input BGR image to the LAB color space and then applies
 * Contrast Limited Adaptive Histogram Equalization (CLAHE) to the L channel. The resulting
 * L channel is then merged with the original A and B channels to obtain the normalized LAB image.
 * Finally, the LAB image is converted back to the BGR color space and replaces the original BGR image.
 *
 * @param bgr_image The BGR image to be normalized. Note that the input image will be modified.
 */
void DisplayerFunctional::NormalizeBGRImage(cv::Mat &bgr_image) {
    cv::Mat lab_image;
    cvtColor(bgr_image, lab_image, cv::COLOR_BGR2Lab);
    //ectract L channel
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);


    //apply clahe to the L channel and save it in lab_planes
    cv::Mat dst;
    this->clahe->setClipLimit(m_mainWindow->GetBGRNorm());
    this->clahe->apply(lab_planes[0], dst);
    dst.copyTo(lab_planes[0]);

    //merge color planes back to bgr_image
    cv::merge(lab_planes, lab_image);

    //convert back to rgb
    cv::cvtColor(lab_image, bgr_image, cv::COLOR_Lab2BGR);

}


/**
 * @brief Determines if the given display image type is functional.
 *
 * A display image type is considered functional if it is either equal to 'OXY' or 'VHB'.
 *
 * @param displayImageType The display image type to check.
 * @return True if the display image type is functional, false otherwise.
 */
bool IsFunctional(DisplayImageType displayImageType) {
    return (displayImageType == OXY || displayImageType == VHB);
}


/**
 * @brief Prepares the raw image from XIMEA camera to be displayed.
 *        It applies the color mapping based on a Look-Up Table (LUT),
 *        and optionally performs histogram equalization.
 *        Each pixel is processed in parallel using C++11 lambda.
 *
 * @param raw_image The raw image to be processed.
 * @param equalize_hist Flag indicating whether to perform histogram equalization or not.
 */
void DisplayerFunctional::PrepareRawImage(cv::Mat &raw_image, bool equalize_hist) {
    cv::Mat mask = raw_image.clone();
    cvtColor(mask, mask, cv::COLOR_GRAY2RGB);
    cv::LUT(mask, m_lut, mask);
    if (equalize_hist) {
        this->clahe->apply(raw_image, raw_image);
    }
    cvtColor(raw_image, raw_image, cv::COLOR_GRAY2RGB);

    // Parallel execution on each pixel using C++11 lambda.
    raw_image.forEach<Pixel>([mask, this](Pixel &p, const int position[]) -> void {
        if (mask.at<cv::Vec3b>(position[0], position[1]) == SATURATION_COLOR) {
            p.x = SATURATION_COLOR[0];
            p.y = SATURATION_COLOR[1];
            p.z = SATURATION_COLOR[2];
        } else if (mask.at<cv::Vec3b>(position[0], position[1]) == DARK_COLOR) {
            p.x = DARK_COLOR[0];
            p.y = DARK_COLOR[1];
            p.z = DARK_COLOR[2];
        }
    });
}


/**
 * @brief Displays an image in a named window.
 *
 * This function displays the given image in a window with the specified name.
 * It performs channel check, image resizing, and displaying using OpenCV functions.
 *
 * @param image The image to be displayed.
 * @param windowName The name of the window for displaying.
 *
 * @throws std::runtime_error if the number of channels in the image is not 1 or 3.
 */
void DisplayerFunctional::DisplayImage(cv::Mat &image, const std::string windowName) {
    if (image.channels() != 1 && image.channels() != 3) {
        throw std::runtime_error("number of channels need to be either 1 or 3");
    }

    cv::Size newSize(image.size().width, image.size().height);
    cv::resize(image, image, newSize);
    cv::imshow(windowName.c_str(), image);
}


/**
 * @brief Function that extracts a specific band (channel) from an image
 *
 * This function computes the location of the first value of the desired band based on the mosaic shape.
 * It then selects data from the specific band and stores it in band_image. band_image is converted
 * to an 8-bit image and divided by the scaling factor to convert it from 10-bit to 8-bit.
 *
 * @param image The input image
 * @param band_image The output band image
 * @param band_nr The number of the band to extract
 */
void DisplayerFunctional::GetBand(cv::Mat &image, cv::Mat &band_image, unsigned int band_nr) {
    // compute location of first value
    int init_col = (band_nr - 1) % this->m_mosaicShape[0];
    int init_row = (band_nr - 1) / this->m_mosaicShape[1];
    // select data from specific band
    int row = 0;
    for (int i = init_row; i < image.rows; i += this->m_mosaicShape[0]) {
        int col = 0;
        for (int j = init_col; j < image.cols; j += this->m_mosaicShape[1]) {
            band_image.at<ushort>(row, col) = image.at<ushort>(i, j);
            col++;
        }
        row++;
    }
    band_image /= m_scaling_factor; // 10 bit to 8 bit
    band_image.convertTo(band_image, CV_8UC1);
}


/**
 * @brief  Downsample the image if it exceeds the maximum display window dimensions.
 *
 * If the width or height of the image exceeds the maximum display window dimensions,
 * the image will be downsampled to fit within the window.
 *
 * @param image [in,out] The image to be downsampled
 */
void DisplayerFunctional::DownsampleImageIfNecessary(cv::Mat &image) {
    // Check if the image exceeds the maximum dimensions
    if (image.cols > MAX_WIDTH_DISPLAY_WINDOW || image.rows > MAX_HEIGHT_DISPLAY_WINDOW) {
        double scale = std::min(
                (double) MAX_WIDTH_DISPLAY_WINDOW / image.cols,
                (double) MAX_HEIGHT_DISPLAY_WINDOW / image.rows);
        cv::resize(image, image, cv::Size(), scale, scale, cv::INTER_AREA);
    }
}


/**
 * @brief Displays the given image.
 *
 * This method displays the provided image using OpenCV functions.
 * A mutex is used to ensure thread safety during the display operation.
 * The image is processed depending on the camera type, and then displayed.
 * If the camera type is 'spectral', only a particular band of the image is displayed.
 * If the camera type is not 'spectral', the image is displayed as it is, after converting to 8-bit format.
 * Every 100th frame is skipped to provide the system a chance to recover from intensive UI interaction.
 *
 * The image is normalized before displaying it if specified in the GUI. Pixels that have values out of range are
 * colored differently to identify them in the display. These colors and boundaries are defined in constants.h.
 *
 * @param image The image to be displayed. It should be format CV_16UC1 and will be converted to CV_8UC1 or CV_8UC3 for display.
 *
 * @see constants.h
 */
void DisplayerFunctional::Display(XI_IMG &image) {
    if (m_stop){
        return;
    }
    static int selected_display = 0;
    selected_display++;
    // give it some time to draw the first frame. For some reason neccessary.
    // probably has to do with missing waitkeys after imshow (these crash the application)
    if ((selected_display == 1) || (selected_display > 10)) {
        // additionally, give it some chance to recover from lots of ui interaction by skipping
        // every 100th frame
        if (selected_display % 100 > 0) {
            boost::lock_guard<boost::mutex> guard(mtx_);

            cv::Mat currentImage(image.height, image.width, CV_16UC1, image.bp);
            cv::Mat rawImage;
            static cv::Mat bgrImage;

            if (m_cameraType == CAMERA_TYPE_SPECTRAL) {
                rawImage = cv::Mat::zeros(currentImage.rows / this->m_mosaicShape[0],currentImage.cols / this->m_mosaicShape[1], CV_16UC1);
                this->GetBand(currentImage, rawImage, m_mainWindow->GetBand());
                bgrImage = cv::Mat::zeros(currentImage.rows / this->m_mosaicShape[0], currentImage.cols / this->m_mosaicShape[1], CV_8UC3);
                this->GetBGRImage(currentImage, bgrImage);
            } else if (m_cameraType == CAMERA_TYPE_GRAY) {
                rawImage = currentImage.clone();
                rawImage /= m_scaling_factor; // 10 bit to 8 bit
                rawImage.convertTo(rawImage, CV_8UC1);
                cv::cvtColor(rawImage, bgrImage, cv::COLOR_GRAY2BGR);
            } else if (m_cameraType == CAMERA_TYPE_RGB){
                rawImage = currentImage.clone();
                rawImage /= m_scaling_factor; // 10 bit to 8 bit
                rawImage.convertTo(rawImage, CV_8UC3);

                bgrImage = currentImage.clone();
                if (image.color_filter_array == XI_CFA_BAYER_GBRG){
                cv::cvtColor(bgrImage, bgrImage, cv::COLOR_BayerGB2RGB);
                } else {
                    LOG_SUSICAM(error) << "Could not interpret filter array of type: " << image.color_filter_array;
                }

                bgrImage.convertTo(bgrImage, CV_8UC3, 1.0 / m_scaling_factor);
            } else {
                LOG_SUSICAM(error) << "Could not recognize camera type: " << m_cameraType.toStdString();
                throw std::runtime_error("Could not recognize camera type: " + m_cameraType.toStdString());
            }
            cv::Mat raw_image_to_display = rawImage.clone();
            DownsampleImageIfNecessary(raw_image_to_display);
            this->PrepareRawImage(raw_image_to_display, m_mainWindow->GetNormalize());
            DisplayImage(raw_image_to_display, DISPLAY_WINDOW_NAME);

            // display BGR image
            DownsampleImageIfNecessary(bgrImage);
            if (m_mainWindow->GetNormalize()) {
                NormalizeBGRImage(bgrImage);
            } else {
                PrepareBGRImage(bgrImage, m_mainWindow->GetBGRNorm());
            }
            DisplayImage(bgrImage, BGR_WINDOW_NAME);
            // update saturation displays
            m_mainWindow->UpdateSaturationPercentageLCDDisplays(rawImage);
        }
    }
}


/**
 * @brief Get the BGR image from the input image by splitting it into separate channels,
 * applying band filters and merging the channels.
 *
 * @param image The input image.
 * @param bgr_image The output BGR image.
 */
void DisplayerFunctional::GetBGRImage(cv::Mat &image, cv::Mat &bgr_image) {
    std::vector<cv::Mat> channels;
    for (int i: m_bgr_channels) {
        cv::Mat band_image = cv::Mat::zeros(image.rows / this->m_mosaicShape[0], image.cols / this->m_mosaicShape[1], CV_16UC1);
        this->GetBand(image, band_image, i);
        channels.push_back(band_image);
    }
    // Merge the images
    try {
        cv::merge(channels, bgr_image);
    } catch (const cv::Exception &e) {
        LOG_SUSICAM(error) << "OpenCV error: " << e.what();
    }
}


/**
 * @brief Sets the camera type for the DisplayerFunctional instance.
 *
 * @param cameraModel A QString that represents the type of the camera.
 * This string is moved to the m_cameraType member variable.
 */
void DisplayerFunctional::SetCameraProperties(QString cameraModel) {
    QString cameraType = CAMERA_MAPPER.value(cameraModel).cameraType;
    this->m_cameraType = std::move(cameraType);
    auto mosaicShape = CAMERA_MAPPER.value(cameraModel).mosaicShape;
    this->m_mosaicShape = std::move(mosaicShape);
}


/**
  * @brief Creates windows to display the result
  *
  * This function creates windows to display the result of different images including the raw image, RGB estimate,
  * blood volume fraction, and oxygenation.
  * The windows are created using OpenCV's namedWindow function and are set to be of a normal size.
  * The windows are then moved to specific positions on the screen using OpenCV's moveWindow function.
  */
void DisplayerFunctional::CreateWindows() {
    // create windows to display result
    cv::namedWindow(DISPLAY_WINDOW_NAME, cv::WINDOW_NORMAL);
    cv::namedWindow(BGR_WINDOW_NAME, cv::WINDOW_NORMAL);

    cv::moveWindow(DISPLAY_WINDOW_NAME, 900, 10);
    cv::moveWindow(BGR_WINDOW_NAME, 2024 + 11, 10);
}


/**
 * Destroy all OpenCV generated windows.
 */
void DisplayerFunctional::DestroyWindows() {
    cv::destroyAllWindows();
}

