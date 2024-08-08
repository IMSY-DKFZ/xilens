/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#include <boost/thread.hpp>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string>
#include <utility>
#if CV_VERSION_MAJOR == 3
#include <opencv2/videoio.hpp>
#endif
#if __has_include(<opencv2/contrib/contrib.hpp>)
#include <opencv2/contrib/contrib.hpp>
#else
#define opencv2_has_contrib 0
#endif

#include "constants.h"
#include "displayFunctional.h"
#include "logger.h"
#include "mainwindow.h"
#include "util.h"

typedef cv::Point3_<uint8_t> Pixel;

DisplayerFunctional::DisplayerFunctional(MainWindow *mainWindow) : Displayer(), m_mainWindow(mainWindow)
{
}

DisplayerFunctional::~DisplayerFunctional()
{
}

void PrepareBGRImage(cv::Mat &bgr_image, int bgr_norm)
{
    double min, max;
    static double last_norm = 1.;
    cv::minMaxLoc(bgr_image, &min, &max);

    last_norm = 0.9 * last_norm + (double)bgr_norm * 0.01 * max;

    bgr_image *= 255. / last_norm;
    bgr_image.convertTo(bgr_image, CV_8UC3);
}

void DisplayerFunctional::NormalizeBGRImage(cv::Mat &bgr_image)
{
    cv::Mat lab_image;
    cvtColor(bgr_image, lab_image, cv::COLOR_BGR2Lab);
    // ectract L channel
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);

    // apply clahe to the L channel and save it in lab_planes
    cv::Mat dst;
    this->clahe->setClipLimit(m_mainWindow->GetBGRNorm());
    this->clahe->apply(lab_planes[0], dst);
    dst.copyTo(lab_planes[0]);

    // merge color planes back to bgr_image
    cv::merge(lab_planes, lab_image);

    // convert back to rgb
    cv::cvtColor(lab_image, bgr_image, cv::COLOR_Lab2BGR);
}

void DisplayerFunctional::PrepareRawImage(cv::Mat &raw_image, bool equalize_hist)
{
    cv::Mat mask = raw_image.clone();
    cvtColor(mask, mask, cv::COLOR_GRAY2RGB);
    cv::LUT(mask, m_lut, mask);
    if (equalize_hist)
    {
        this->clahe->apply(raw_image, raw_image);
    }
    cvtColor(raw_image, raw_image, cv::COLOR_GRAY2RGB);

    // Parallel execution on each pixel using C++11 lambda.
    raw_image.forEach<Pixel>([mask, this](Pixel &p, const int position[]) -> void {
        if (mask.at<cv::Vec3b>(position[0], position[1]) == SATURATION_COLOR)
        {
            p.x = SATURATION_COLOR[0];
            p.y = SATURATION_COLOR[1];
            p.z = SATURATION_COLOR[2];
        }
        else if (mask.at<cv::Vec3b>(position[0], position[1]) == DARK_COLOR)
        {
            p.x = DARK_COLOR[0];
            p.y = DARK_COLOR[1];
            p.z = DARK_COLOR[2];
        }
    });
}

void DisplayerFunctional::GetBand(cv::Mat &image, cv::Mat &band_image, unsigned int band_nr)
{
    // compute location of first value
    int init_col = (band_nr - 1) % this->m_mosaicShape[0];
    int init_row = (band_nr - 1) / this->m_mosaicShape[1];
    // select data from specific band
    int row = 0;
    for (int i = init_row; i < image.rows; i += this->m_mosaicShape[0])
    {
        int col = 0;
        for (int j = init_col; j < image.cols; j += this->m_mosaicShape[1])
        {
            band_image.at<ushort>(row, col) = image.at<ushort>(i, j);
            col++;
        }
        row++;
    }
    band_image /= m_scaling_factor; // 10 bit to 8 bit
    band_image.convertTo(band_image, CV_8UC1);
}

void DisplayerFunctional::DownsampleImageIfNecessary(cv::Mat &image)
{
    // Check if the image exceeds the maximum dimensions
    if (image.cols > MAX_WIDTH_DISPLAY_WINDOW || image.rows > MAX_HEIGHT_DISPLAY_WINDOW)
    {
        double scale =
            std::min((double)MAX_WIDTH_DISPLAY_WINDOW / image.cols, (double)MAX_HEIGHT_DISPLAY_WINDOW / image.rows);
        cv::resize(image, image, cv::Size(), scale, scale, cv::INTER_AREA);
    }
}

void DisplayerFunctional::Display(XI_IMG &image)
{
    if (m_stop)
    {
        return;
    }
    static int selected_display = 0;
    selected_display++;
    // give it some time to draw the first frame. For some reason neccessary.
    // probably has to do with missing waitkeys after imshow (these crash the
    // application)
    if ((selected_display == 1) || (selected_display > 10))
    {
        // additionally, give it some chance to recover from lots of ui interaction
        // by skipping every 100th frame
        if (selected_display % 100 > 0)
        {
            boost::lock_guard<boost::mutex> guard(mtx_);

            cv::Mat currentImage(image.height, image.width, CV_16UC1, image.bp);
            cv::Mat rawImage;
            static cv::Mat bgrImage;

            if (m_cameraType == CAMERA_TYPE_SPECTRAL)
            {
                rawImage = cv::Mat::zeros(currentImage.rows / this->m_mosaicShape[0],
                                          currentImage.cols / this->m_mosaicShape[1], CV_16UC1);
                this->GetBand(currentImage, rawImage, m_mainWindow->GetBand());
                bgrImage = cv::Mat::zeros(currentImage.rows / this->m_mosaicShape[0],
                                          currentImage.cols / this->m_mosaicShape[1], CV_8UC3);
                this->GetBGRImage(currentImage, bgrImage);
            }
            else if (m_cameraType == CAMERA_TYPE_GRAY)
            {
                rawImage = currentImage.clone();
                rawImage /= m_scaling_factor; // 10 bit to 8 bit
                rawImage.convertTo(rawImage, CV_8UC1);
                cv::cvtColor(rawImage, bgrImage, cv::COLOR_GRAY2BGR);
            }
            else if (m_cameraType == CAMERA_TYPE_RGB)
            {
                rawImage = currentImage.clone();
                rawImage /= m_scaling_factor; // 10 bit to 8 bit
                rawImage.convertTo(rawImage, CV_8UC3);

                bgrImage = currentImage.clone();
                if (image.color_filter_array == XI_CFA_BAYER_GBRG)
                {
                    cv::cvtColor(bgrImage, bgrImage, cv::COLOR_BayerGB2RGB);
                }
                else
                {
                    LOG_SUSICAM(error) << "Could not interpret filter array of type: " << image.color_filter_array;
                }

                bgrImage.convertTo(bgrImage, CV_8UC3, 1.0 / m_scaling_factor);
            }
            else
            {
                LOG_SUSICAM(error) << "Could not recognize camera type: " << m_cameraType.toStdString();
                throw std::runtime_error("Could not recognize camera type: " + m_cameraType.toStdString());
            }
            cv::Mat raw_image_to_display = rawImage.clone();
            DownsampleImageIfNecessary(raw_image_to_display);
            this->PrepareRawImage(raw_image_to_display, m_mainWindow->GetNormalize());
            // display BGR image
            DownsampleImageIfNecessary(bgrImage);
            if (m_mainWindow->GetNormalize())
            {
                NormalizeBGRImage(bgrImage);
            }
            else
            {
                PrepareBGRImage(bgrImage, m_mainWindow->GetBGRNorm());
            }
            // update saturation displays
            m_mainWindow->UpdateSaturationPercentageLCDDisplays(rawImage);

            // display images for RGB and Raw
            m_mainWindow->UpdateRGBImage(bgrImage);

            m_mainWindow->UpdateRawImage(raw_image_to_display);
        }
    }
}

void DisplayerFunctional::GetBGRImage(cv::Mat &image, cv::Mat &bgr_image)
{
    std::vector<cv::Mat> channels;
    for (int i : m_bgr_channels)
    {
        cv::Mat band_image =
            cv::Mat::zeros(image.rows / this->m_mosaicShape[0], image.cols / this->m_mosaicShape[1], CV_16UC1);
        this->GetBand(image, band_image, i);
        channels.push_back(band_image);
    }
    // Merge the images
    try
    {
        cv::merge(channels, bgr_image);
    }
    catch (const cv::Exception &e)
    {
        LOG_SUSICAM(error) << "OpenCV error: " << e.what();
    }
}

void DisplayerFunctional::SetCameraProperties(QString cameraModel)
{
    QString cameraType = getCameraMapper().value(cameraModel).cameraType;
    this->m_cameraType = std::move(cameraType);
    auto mosaicShape = getCameraMapper().value(cameraModel).mosaicShape;
    this->m_mosaicShape = std::move(mosaicShape);
}
