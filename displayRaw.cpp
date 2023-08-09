/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include "displayRaw.h"
#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#if CV_VERSION_MAJOR==3
#include <opencv2/videoio.hpp>
#endif
#if __has_include(<opencv2/contrib/contrib.hpp>)
#  include <opencv2/contrib/contrib.hpp>
#else
#  define opencv2_has_contrib 0
#endif


#include <QGuiApplication>
#include <QScreen>

#include "mainwindow.h"
#include "util.h"


const std::string DISPLAY_WINDOW_NAME = "SuSI Live Cam";


DisplayerRaw::DisplayerRaw(MainWindow* mainWindow) :
    Displayer(), m_mainWindow(mainWindow)
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
    float range[] = { 0, 256}; //the upper boundary is exclusive
    const float* histRange = { range };
    bool uniform = true, accumulate = false;
    cv::Mat hist;
    calcHist(&roiImg, 1, nullptr, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);
    int hist_w = 512, hist_h = 200;
    int bin_w = cvRound((double) hist_w/histSize);
    cv::Mat histImage(hist_h, hist_w, CV_8UC3, cv::Scalar( 0,0,0));
    normalize(hist, hist, 0, histImage.rows, cv::NORM_MINMAX, -1, cv::Mat() );
    for( int i = 1; i < histSize; i++ )
    {
        line(histImage, cv::Point( bin_w*(i-1), hist_h - cvRound(hist.at<float>(i-1))),
              cv::Point(bin_w*(i), hist_h - cvRound(hist.at<float>(i))),
              cv::Scalar( 255, 255, 255), 2, 8, 0 );
    }
    imshow("ROI histogram", histImage );
    cv::waitKey();
}

void mouseHandler(int event, int x, int y, int flags, void* param)
{

    static cv::Point point1, point2; /* vertical points of the bounding box */
    static int drag;
    cv::Rect rect; /* bounding box */
    cv::Mat displayedRoi;
    static cv::Mat  roiImg; /* roiImg - the part of the image in the bounding box */

    cv::Mat image = *((cv::Mat*) param);

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

        rect = cv::Rect(x1,y1,x2-x1,y2-y1);
        drag = 0;
        roiImg = image(rect);

        double min, max;
        cv::minMaxLoc(roiImg, &min, &max);

        std::stringstream message;
        message << "ROI MIN: " << (unsigned int) min << " -- MAX: " << (unsigned int) max << std::endl;
        std::cout << message.str() << std::flush;


        cv::resize(roiImg, roiImg, roiImg.size()*5, 0., 0., cv::INTER_NEAREST);
    }

    if (event == CV_EVENT_LBUTTONUP)
    {
       /* ROI selected */
        drag = 0;
        displayedRoi = roiImg.clone();
        displayedRoi /= 4;
        (displayedRoi).convertTo(displayedRoi, CV_8UC1);
        imshow("ROI", displayedRoi); /* show the image bounded by the box */
        DrawHistogram(roiImg/4);
    }
}


cv::Mat DisplayerRaw::PrepareImageToDisplay(cv::Mat& image)
{
    image = image.clone();
    image /= 4;
    image.convertTo(image, CV_8UC1);
    return image;
}


void GetDesktopResolution(int& width, int& height)
{
    // get the screen
    QScreen *screen = QGuiApplication::primaryScreen();

    // get the screen geometry
    QRect screenGeometry = screen->geometry();
    height = screenGeometry.height();
    width = screenGeometry.width();
}


void DisplayerRaw::DisplayImage(cv::Mat& image, const std::string windowName)
{
    m_rawImage = image.clone();
    m_displayImage = PrepareImageToDisplay(image);
    int w_heigth = 0;
    int w_width = 0;

    // resizing windows does not work properly with mouse callback
    cv::Size newSize(m_displayImage.size().width/4, m_displayImage.size().height/4);
//    cv::Size newSize(w_width / 4, w_heigth / 4);
    cv::resize(m_displayImage, m_displayImage, newSize);
    cv::imshow(windowName.c_str(), m_displayImage);
}

void DisplayerRaw::Display(XI_IMG& image)
{
    static int selected_display = 0;

    selected_display++;
    // give it some time to draw the first frame. For some reason neccessary.
    // probably has to do with missing waitkeys after imshow (these crash the application)
    if ((selected_display==1) || (selected_display>10))
    {
        // additionally, give it some chance to recover from lots of ui interaction by skipping
        // every 100th frame
        if (selected_display % 100 > 0)
        {
            cv::Mat currentImage(image.height, image.width, CV_16UC1, image.bp);
            DisplayImage(currentImage, DISPLAY_WINDOW_NAME);
        }
    }
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
