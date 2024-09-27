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
    auto result = QObject::connect(&m_displayTimer, &QTimer::timeout, this, &DisplayerFunctional::OnDisplayTimeout);
    if (!result)
    {
        LOG_XILENS(error) << "Error while connecting displayer to timer";
    }
    m_displayTimer.setInterval(m_displayIntervalMilliseconds);
    m_displayTimer.start();
    m_displayThread = boost::thread(&DisplayerFunctional::ProcessImageOnThread, this);
}

DisplayerFunctional::~DisplayerFunctional()
{
    if (m_displayTimer.isActive())
    {
        m_displayTimer.stop();
    }
    {
        boost::lock_guard<boost::mutex> guard(m_mutexImageDisplay);
        m_stop = true;
    }
    m_displayCondition.notify_all();
    m_displayThread.interrupt();
    if (m_displayThread.joinable())
    {
        m_displayThread.join();
    }
    QObject::disconnect();
    m_clahe.release();
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
    // extract L channel
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);

    // apply m_clahe to the L channel and save it in lab_planes
    cv::Mat dst;
    this->m_clahe->setClipLimit(m_mainWindow->GetBGRNorm());
    this->m_clahe->apply(lab_planes[0], dst);
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
        this->m_clahe->apply(raw_image, raw_image);
    }
    cvtColor(raw_image, raw_image, cv::COLOR_GRAY2RGB);

    if (m_mainWindow->IsSaturationButtonChecked())
    {
        // Parallel execution on each pixel using C++11 lambda.
        raw_image.forEach<Pixel>([mask](Pixel &p, const int position[]) -> void {
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
}

void DisplayerFunctional::GetBand(cv::Mat &image, cv::Mat &band_image, unsigned int band_nr)
{
    if (band_nr < 1 || band_nr > (this->m_mosaicShape[0] * this->m_mosaicShape[1]))
    {
        throw std::out_of_range("Band number is out of the expected range.");
    }
    // compute location of first value
    int init_col = static_cast<int>(band_nr - 1) % this->m_mosaicShape[0];
    int init_row = static_cast<int>(band_nr - 1) / this->m_mosaicShape[1];
    // select data from the specific band
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
    boost::lock_guard<boost::mutex> guard(m_mutexImageDisplay);
    m_nextImage = image;
    m_hasPendingImage = true;
}

void DisplayerFunctional::OnDisplayTimeout()
{
    if (m_hasPendingImage)
    {
        m_displayCondition.notify_one();
    }
}

[[noreturn]] void DisplayerFunctional::ProcessImageOnThread()
{
    while (true)
    {
        XI_IMG image;
        {
            boost::unique_lock<boost::mutex> lock(m_mutexImageDisplay);
            boost::this_thread::interruption_point();
            m_displayCondition.wait(lock, [this] { return m_hasPendingImage; });

            if (m_hasPendingImage)
            {
                image = m_nextImage;
                m_hasPendingImage = false;
            }
        }
        ProcessImage(image);
    }
}

void DisplayerFunctional::ProcessImage(XI_IMG &image)
{
    if (m_stop)
    {
        return;
    }
    cv::Mat currentImage;
    int filterArrayType;
    {
        boost::lock_guard<boost::mutex> guard(m_mutexImageDisplay);
        currentImage =
            cv::Mat(static_cast<int>(image.height), static_cast<int>(image.width), CV_16UC1, image.bp).clone();
        filterArrayType = image.color_filter_array;
    }
    cv::Mat rawImage;
    static cv::Mat bgrImage;

    if (m_cameraType == CAMERA_TYPE_SPECTRAL)
    {
        rawImage = InitializeBandImage(currentImage);
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
        if (filterArrayType == XI_CFA_BAYER_GBRG)
        {
            cv::cvtColor(bgrImage, bgrImage, cv::COLOR_BayerGB2BGR);
        }
        else
        {
            LOG_XILENS(error) << "Could not interpret filter array of type: " << filterArrayType;
        }

        bgrImage.convertTo(bgrImage, CV_8UC3, 1.0 / m_scaling_factor);
    }
    else
    {
        LOG_XILENS(error) << "Could not recognize camera type: " << m_cameraType.toStdString();
        throw std::runtime_error("Could not recognize camera type: " + m_cameraType.toStdString());
    }
    cv::Mat rawImageToDisplay = rawImage.clone();
    DownsampleImageIfNecessary(rawImageToDisplay);
    this->PrepareRawImage(rawImageToDisplay, m_mainWindow->GetNormalize());
    // display BGR image
    DownsampleImageIfNecessary(bgrImage);
    if (m_mainWindow->GetNormalize())
    {
        NormalizeBGRImage(bgrImage);
    }
    else
    {
        PrepareBGRImage(bgrImage, static_cast<int>(m_mainWindow->GetBGRNorm()));
    }
    // Update saturation display and display images through the main thread
    auto bgrQImage = GetQImageFromMatrix(bgrImage, QImage::Format_RGB888);
    auto rawQImage = GetQImageFromMatrix(rawImageToDisplay, QImage::Format_BGR888);
    auto saturationValues = GetSaturationPercentages(rawImage);
    emit ImageReadyToUpdateRGB(bgrQImage);
    emit ImageReadyToUpdateRaw(rawQImage);
    emit SaturationPercentageReady(saturationValues.first, saturationValues.second);
}

void DisplayerFunctional::GetBGRImage(cv::Mat &image, cv::Mat &bgr_image)
{
    if (!getCameraMapper().contains(m_cameraModel))
    {
        LOG_XILENS(error) << "Could not find camera model in Mapper: " << m_cameraModel.toStdString();
        throw std::runtime_error("Could not find camera in Mapper");
    }
    auto bgrChannels = getCameraMapper().value(m_cameraModel).bgrChannels;
    if (bgrChannels.empty())
    {
        LOG_XILENS(error) << "Empty BGR channel indices";
        throw std::runtime_error("Empty RGB channel indices");
    }
    std::vector<cv::Mat> channels;
    for (int i : bgrChannels)
    {
        cv::Mat band_image = InitializeBandImage(image);
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
        LOG_XILENS(error) << "OpenCV error: " << e.what();
    }
}

cv::Mat DisplayerFunctional::InitializeBandImage(cv::Mat &image)
{
    int band_rows = (image.rows + m_mosaicShape[0] - 1) / m_mosaicShape[0]; // Using ceiling division
    int band_cols = (image.cols + m_mosaicShape[1] - 1) / m_mosaicShape[1]; // Using ceiling division
    cv::Mat band_image = cv::Mat::zeros(band_rows, band_cols, CV_16UC1);
    return band_image;
}

void DisplayerFunctional::SetCameraProperties(QString cameraModel)
{
    if (!getCameraMapper().contains(cameraModel))
    {
        LOG_XILENS(error) << "Could not find camera model in Mapper: " << cameraModel.toStdString();
        throw std::runtime_error("Could not find camera in Mapper");
    }
    this->m_cameraType = getCameraMapper().value(cameraModel).cameraType;
    this->m_cameraModel = cameraModel;
    this->m_mosaicShape = getCameraMapper().value(cameraModel).mosaicShape;
}

QImage GetQImageFromMatrix(cv::Mat &image, QImage::Format format)
{
    QImage qtImage((uchar *)image.data, image.cols, image.rows, static_cast<long>(image.step), format);
    return qtImage;
}

std::pair<double, double> GetSaturationPercentages(cv::Mat &image)
{
    if (image.empty() || image.type() != CV_8UC1)
    {
        throw std::invalid_argument("Invalid input matrix. It must be non-empty and of type CV_8UC1, "
                                    "got: " +
                                    cv::typeToString(image.type()));
    }

    int aboveThresholdCount = cv::countNonZero(image > OVEREXPOSURE_PIXEL_BOUNDARY_VALUE);
    auto totalPixels = static_cast<double>(image.total()); // Total number of pixels in the matrix
    double percentageAboveThreshold = (static_cast<double>(aboveThresholdCount) / totalPixels) * 100.0;

    int belowThresholdCount = cv::countNonZero(image < UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE);
    double percentageBelowThreshold = (static_cast<double>(belowThresholdCount) / totalPixels) * 100.0;
    return std::make_pair(percentageBelowThreshold, percentageAboveThreshold);
}
