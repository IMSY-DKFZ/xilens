/*
 * ===================================================================
 * Surgical Spectral Imaging Library (SuSI)
 *
 * Copyright (c) German Cancer Research Center,
 * Division of Medical and Biological Informatics.
 * All rights reserved.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 *
 * See LICENSE.txt for details.
 * ===================================================================
 */


#include "util.h"
#include "default_defines.h"

#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <opencv2/imgproc.hpp>

FileImage::FileImage(const char* filename, const char* mode){
    file = fopen(filename, mode);
    if (!file)
        throw std::runtime_error("Could not open the file.");
}

FileImage::~FileImage(){
    fclose(file);
}

void FileImage::write(XI_IMG image) {
    fwrite(image.bp, image.width*image.height, sizeof(UINT16), file);
}

const char* libfive_git_version(void)
{
    return GIT_TAG;
}

const char* libfive_git_revision(void)
{
    return GIT_REV;
}

const char* libfive_git_branch(void)
{
    return GIT_BRANCH;
}

float clip(float n, float lower, float upper) {
    return std::max(lower, std::min(n, upper));
}

void rescale(cv::Mat& mat, float high)
{
    double min, max;
    cv::minMaxLoc(mat, &min, &max);
    mat = (mat - ((float) min)) * high/((float) max - min);
}

void clamp(cv::Mat& mat, cv::Range bounds) {
    min(max(mat, bounds.start), bounds.end, mat);
}


void wait(int milliseconds)
{
  boost::this_thread::sleep_for(boost::chrono::milliseconds(milliseconds));
}


void initLogging(enum boost::log::trivial::severity_level severity)
{
    boost::log::core::get()->set_filter
    (
        boost::log::trivial::severity >= severity
    );
}

cv::Mat CreateLut(cv::Vec3b saturation_color, cv::Vec3b dark_color)
{
    cv::Mat Lut(1, 256, CV_8UC3);
    for (uint i=0; i<256; ++i)
    {
        Lut.at<cv::Vec3b>(0, i) = cv::Vec3b(i,i,i);
        if (i>225)
        {
            Lut.at<cv::Vec3b>(0, i) = saturation_color;
        }
        if (i<10)
        {
            Lut.at<cv::Vec3b>(0, i) = dark_color;
        }
    }
    return Lut;
}

struct CommandLineArguments g_commandLineArguments;
