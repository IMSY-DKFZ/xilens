/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#include <QGuiApplication>
#include <QScreen>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string>
#if CV_VERSION_MAJOR == 3
#include <opencv2/videoio.hpp>
#endif
#if __has_include(<opencv2/contrib/contrib.hpp>)
#include <opencv2/contrib/contrib.hpp>
#else
#define opencv2_has_contrib 0
#endif

#include "constants.h"
#include "displayRaw.h"
#include "logger.h"
#include "mainwindow.h"
#include "util.h"

DisplayerRaw::DisplayerRaw(MainWindow *mainWindow) : Displayer(), m_mainWindow(mainWindow)
{
    CreateWindows();
}

DisplayerRaw::~DisplayerRaw()
{
    DestroyWindows();
}

void DrawHistogram(cv::Mat roiImg)
{
    std::vector<cv::Mat> bands;
    cv::split(roiImg, bands);
    int histSize = 256;
    float range[] = {0, 256}; // the upper boundary is exclusive
    const float *histRange = {range};
    bool uniform = true, accumulate = false;
    cv::Mat hist;
    calcHist(&roiImg, 1, nullptr, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);
    int hist_w = 512, hist_h = 200;
    int bin_w = cvRound((double)hist_w / histSize);
    cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar(0, 0, 0));
    normalize(hist, hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat());
    for (int i = 1; i < histSize; i++)
    {
        line(histImage, cv::Point(bin_w * (i - 1), hist_h - cvRound(hist.at<float>(i - 1))),
             cv::Point(bin_w * (i), hist_h - cvRound(hist.at<float>(i))), cv::Scalar(255, 255, 255), 2, 8, 0);
    }
    imshow("ROI histogram", histImage);
    cv::waitKey();
}

void mouseHandler(int event, int x, int y, int flags, void *param)
{
    static cv::Point point1, point2; /* vertical points of the bounding box */
    static int drag;
    cv::Rect rect; /* bounding box */
    cv::Mat displayedRoi;
    static cv::Mat roiImg; /* roiImg - the part of the image in the bounding box */

    cv::Mat image = *((cv::Mat *)param);

    if (event == CV_EVENT_LBUTTONDOWN && !drag)
    {
        /* left button clicked. ROI selection begins */
        point1 = cv::Point(x, y);
        drag = 1;
    }

    if (event == CV_EVENT_LBUTTONUP && drag)
    {
        point2 = cv::Point(x, y);

        unsigned x1, x2, y1, y2;
        if (point1.x < point2.x)
        {
            x1 = point1.x;
            x2 = point2.x;
        }
        else
        {
            x1 = point2.x;
            x2 = point1.x;
        }

        if (point1.y < point2.y)
        {
            y1 = point1.y;
            y2 = point2.y;
        }
        else
        {
            y1 = point2.y;
            y2 = point1.y;
        }

        if (point1.x == point2.x & point1.y == point2.y)
        {
            x1 = point1.x;
            x2 = point1.x + 20;
            y1 = point1.y;
            y2 = point1.y + 20;
        }

        rect = cv::Rect(x1, y1, x2 - x1, y2 - y1);
        drag = 0;
        roiImg = image(rect);

        double min, max;
        cv::minMaxLoc(roiImg, &min, &max);

        std::stringstream message;
        message << "ROI MIN: " << (unsigned int)min << " -- MAX: " << (unsigned int)max << std::endl;
        LOG_SUSICAM(info) << message.str();

        cv::resize(roiImg, roiImg, roiImg.size() * 5, 0., 0., cv::INTER_NEAREST);
    }

    if (event == CV_EVENT_LBUTTONUP)
    {
        /* ROI selected */
        drag = 0;
        displayedRoi = roiImg.clone();
        displayedRoi /= 4;
        (displayedRoi).convertTo(displayedRoi, CV_8UC1);
        imshow("ROI", displayedRoi); /* show the image bounded by the box */
        DrawHistogram(roiImg / 4);
    }
}

void DisplayerRaw::DownsampleImageIfNecessary(cv::Mat &image)
{
    // Check if the image exceeds the maximum dimensions
    if (image.cols > MAX_WIDTH_DISPLAY_WINDOW || image.rows > MAX_HEIGHT_DISPLAY_WINDOW)
    {
        double scale =
            std::min((double)MAX_WIDTH_DISPLAY_WINDOW / image.cols, (double)MAX_HEIGHT_DISPLAY_WINDOW / image.rows);
        cv::resize(image, image, cv::Size(), scale, scale, cv::INTER_AREA);
    }
}

cv::Mat DisplayerRaw::PrepareImageToDisplay(cv::Mat &image)
{
    image = image.clone();
    image /= 4;
    image.convertTo(image, CV_8UC1);
    return image;
}

void GetDesktopResolution(int &width, int &height)
{
    // get the screen
    QScreen *screen = QGuiApplication::primaryScreen();

    // get the screen geometry
    QRect screenGeometry = screen->geometry();
    height = screenGeometry.height();
    width = screenGeometry.width();
}

void DisplayerRaw::DisplayImage(cv::Mat &image, const std::string windowName)
{
    m_rawImage = image.clone();
    m_ImageToDisplay = PrepareImageToDisplay(image);
    DownsampleImageIfNecessary(image);
    cv::imshow(windowName.c_str(), m_ImageToDisplay);
}

void DisplayerRaw::Display(XI_IMG &image)
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
            cv::Mat currentImage(image.height, image.width, CV_16UC1, image.bp);
            DisplayImage(currentImage, DISPLAY_WINDOW_NAME);
        }
    }
}

void DisplayerRaw::SetCameraProperties(QString cameraModel)
{
    QString cameraType = CAMERA_MAPPER.value(cameraModel).cameraType;
    this->m_cameraType = std::move(cameraType);
}

void DisplayerRaw::CreateWindows()
{
    // create windows to display result
    cv::namedWindow(DISPLAY_WINDOW_NAME, cv::WINDOW_AUTOSIZE);
    cv::moveWindow(DISPLAY_WINDOW_NAME, 900, 10);
    cv::setMouseCallback(DISPLAY_WINDOW_NAME, mouseHandler, &m_rawImage);
}

void DisplayerRaw::DestroyWindows()
{
    cv::destroyAllWindows();
}
