/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include "displayFunctional.h"

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

#include <boost/thread.hpp>


#include "mainwindow.h"
#include "util.h"


const std::string DISPLAY_WINDOW_NAME = "SuSI Live Cam";
const std::string VHB_WINDOW_NAME = "Blood volume fraction";
const std::string SAO2_WINDOW_NAME = "Oxygenation";
const std::string BGR_WINDOW_NAME = "RGB estimate";
typedef cv::Point3_<uint8_t> Pixel;


DisplayerFunctional::DisplayerFunctional(MainWindow* mainWindow) :
    Displayer(), m_mainWindow(mainWindow)
{
    CreateWindows();
}


DisplayerFunctional::~DisplayerFunctional()
{
    DestroyWindows();
}


void PrepareFunctionalImage(cv::Mat& functional_image, [[maybe_unused]]DisplayImageType displayImage, bool do_scaling, cv::Range bounds, int colormap)
{
    if (do_scaling)
    {
        functional_image *= 100;
        // set first and second pixel to extreme values so the colormap is always scaled the same
        functional_image.at<float>(0,0) = bounds.start;
        functional_image.at<float>(0,1) = bounds.end;
        clamp(functional_image, bounds);
    }

    rescale(functional_image, 255.);
    functional_image.convertTo(functional_image, CV_8UC1);
    applyColorMap(functional_image, functional_image, colormap);
}


void PrepareRGBImage(cv::Mat& bgr_image, int rgb_norm, int scaling_factor)
{
    bgr_image /= scaling_factor; // 10 bit to 8 bit
    bgr_image.convertTo(bgr_image, CV_8UC1);
    double min, max;
    static double last_norm = 1.;
    cv::minMaxLoc(bgr_image, &min, &max);

    last_norm = 0.9*last_norm + (double)rgb_norm*0.01*max;

    bgr_image*=255./last_norm;
    bgr_image.convertTo(bgr_image, CV_8UC3);
}

/**
 * @brief DisplayerFunctional::NormalizeRGBImage normalizes the bgr_image using clahe.
 * @param bgr_image input
 * We define the matrix lab_images and convert the color bgr to lab.
 * We define a vector lab_planes and split the lab_image into lab_planes.
 * We define a pointer clahe, create clahe and set a treshold for constant limiting.
 * We define a matrix dest, set the clip limit by calling the method GetRGBNorm, apply clahe to the vector lab_planes
 * and save it in the matrix dest.
 * Then we copy the matrix dest to lab_planes and merge lab_planes into lab_image.
 * We convert the color lab to bgr and save it in bgr_image.
 */
void DisplayerFunctional::NormalizeRGBImage(cv::Mat& bgr_image)
{
    cv::Mat lab_image;
    cvtColor(bgr_image, lab_image, cv::COLOR_BGR2Lab);
    //ectract L channel
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);


    //apply clahe to the L channel and save it in lab_planes
    cv::Mat dst;
    this->clahe->setClipLimit(m_mainWindow->GetRGBNorm());
    this->clahe->apply(lab_planes[0], dst);
    dst.copyTo(lab_planes[0]);

    //merge color planes back to bgr_image
    cv::merge(lab_planes, lab_image);

    //convert back to rgb
    cv::cvtColor(lab_image, bgr_image, cv::COLOR_Lab2BGR);

}


bool IsFunctional(DisplayImageType displayImageType)
{
    return (displayImageType == OXY || displayImageType == VHB);
}



void DisplayerFunctional::PrepareRawImage(cv::Mat& raw_image, int scaling_factor, bool equalize_hist)
{
    raw_image /= scaling_factor; // 10 bit to 8 bit
    raw_image.convertTo(raw_image, CV_8UC1);
    cv::Mat mask = raw_image.clone();
    cvtColor(mask, mask, cv::COLOR_GRAY2RGB);
    cv::Mat lut = CreateLut(this->saturation_color, this->dark_color);
    cv::LUT(mask, lut, mask);
    if (equalize_hist)
    {
        this->clahe->apply(raw_image, raw_image);
    }
    cvtColor(raw_image, raw_image, cv::COLOR_GRAY2RGB);

    // Parallel execution on each pixel using C++11 lambda.
    raw_image.forEach<Pixel>([mask, this](Pixel &p, const int position[]) -> void {
        if (mask.at<cv::Vec3b>(position[0], position[1]) == this->saturation_color){
            p.x = this->saturation_color[0];
            p.y = this->saturation_color[1];
            p.z = this->saturation_color[2];
        }
        else if(mask.at<cv::Vec3b>(position[0], position[1]) == this->dark_color){
            p.x = this->dark_color[0];
            p.y = this->dark_color[1];
            p.z = this->dark_color[2];
        }
    });
}



void DisplayerFunctional::DisplayImage(cv::Mat& image, const std::string windowName)
{
    if (image.channels() != 1 && image.channels() != 3)
    {
        throw std::runtime_error("number of channels need to be either 1 or 3");
    }

    cv::Size newSize(image.size().width, image.size().height);
    cv::resize(image, image, newSize);
    cv::imshow(windowName.c_str(), image);
}

void DisplayerFunctional::GetBand(cv::Mat& image, cv::Mat& band_image, int band_nr){
    // compute location of first value
    int init_col = (band_nr - 1) % MOSAIC_SHAPE[0];
    int init_row = (band_nr - 1) / MOSAIC_SHAPE[1];
    // select data from specific band
    int row = 0;
    for (int i = init_row; i <= image.rows; i += MOSAIC_SHAPE[0]){
        int col = 0;
        for (int j = init_col; j <= image.cols; j += MOSAIC_SHAPE[1]){
            band_image.at<ushort>(row,col) = image.at<ushort>(i,j);
            col ++;
        }
        row ++;
    }
}


/**
 * @brief DisplayerFunctional::Display calls methods and shows images on the display.
 * @param image
 * We define a static int selected_display, set it's value zero and increment.
 * If the value equals one or is bigger than 10, if we skip every 100th frame we run the network.
 * We define vectors band_image, physParam and bgr_image which we when the calling the methods GetBands, GetPhysiologicalParameters
 * and GetGBRfrom the network.
 * We prepare the raw image from XIMEA camera and calling the method GetBands then we display the image.
 * We define a matrix, vector and int to be used in the method calling from the network.
 * Else we call the method GetBands and normalize the rgb_image by calling the method NormalizeRGBImage if GetNormalize is checked.
 * Then we display the image.
 * We also display the functional image of VHB and OXY.
 */
void DisplayerFunctional::Display(XI_IMG& image)
{
    static int selected_display = 0;
    cv::Mat bgr_image;

    selected_display++;
    // give it some time to draw the first frame. For some reason neccessary.
    // probably has to do with missing waitkeys after imshow (these crash the application)
    if ((selected_display==1) || (selected_display>10))
    {
        // additionally, give it some chance to recover from lots of ui interaction by skipping
        // every 100th frame
        if (selected_display % 100 > 0)
        {
            boost::lock_guard<boost::mutex> guard(mtx_);

            cv::Mat currentImage(image.height, image.width, CV_16UC1, image.bp);
            cv::Mat band_image(image.height / MOSAIC_SHAPE[0], image.width / MOSAIC_SHAPE[1], CV_16UC1, image.bp);

            this->GetBand(currentImage, band_image, m_mainWindow->GetBand());

            PrepareRawImage(band_image, 4, m_mainWindow->GetNormalize());
            DisplayImage(band_image, DISPLAY_WINDOW_NAME);

            this->GetBGRImage(currentImage, bgr_image);
            if (m_mainWindow->GetNormalize()){
                NormalizeRGBImage(bgr_image);
            }
            else {
                PrepareRGBImage(bgr_image, m_mainWindow->GetRGBNorm(), 4);
            }
            DisplayImage(bgr_image, BGR_WINDOW_NAME);
        }
    }
}


void DisplayerFunctional::GetBGRImage(cv::Mat &image, cv::Mat &bgr_image)
{
    std::vector<cv::Mat> channels;
    cv::Mat band_image;
    for (int i : m_bgr_channels)
    {
        this->GetBand(image, band_image, m_mainWindow->GetBand());
        channels.push_back(band_image);
    }
    // Merge the images
    cv::merge(channels, bgr_image);
}

void DisplayerFunctional::CreateWindows()
{
    // create windows to display result
    cv::namedWindow(DISPLAY_WINDOW_NAME, cv::WINDOW_NORMAL);
    cv::namedWindow(BGR_WINDOW_NAME, cv::WINDOW_NORMAL);
    cv::namedWindow(VHB_WINDOW_NAME, cv::WINDOW_NORMAL);
    cv::namedWindow(SAO2_WINDOW_NAME, cv::WINDOW_NORMAL);

    cv::moveWindow(DISPLAY_WINDOW_NAME, 900, 10);
    cv::moveWindow(BGR_WINDOW_NAME, 2024+11, 10);
    cv::moveWindow(VHB_WINDOW_NAME, 900, 10+626);
    cv::moveWindow(SAO2_WINDOW_NAME, 2024+11, 10+626);
}


void DisplayerFunctional::DestroyWindows()
{
    cv::destroyAllWindows();
}

