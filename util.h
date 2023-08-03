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


#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <string>
#include <stdexcept>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <boost/log/trivial.hpp>

//#define HandleResult(res,place) if (res!=XI_OK) {char* errmsg; sprintf(errmsg, "Error after %s (%d)\n",place,res); std::runtime_error(std::string(errmsg));}
#define HandleResult(res,place) if (res!=XI_OK) {std::stringstream errormsg; errormsg << "Error after " << place << " " << res << "\n"; std::runtime_error(errormsg.str());}

#if (CV_VERSION_MAJOR == 4)
enum{
    CV_LOAD_IMAGE_ANYDEPTH = cv::IMREAD_ANYDEPTH,
    CV_LOAD_IMAGE_ANYCOLOR = cv::IMREAD_COLOR,
    CV_EVENT_LBUTTONDOWN = cv::EVENT_LBUTTONDOWN,
    CV_EVENT_LBUTTONUP = cv::EVENT_LBUTTONUP,
};
#endif

extern "C"
{
    extern const char* GIT_TAG;
    extern const char* GIT_REV;
    extern const char* GIT_BRANCH;
}

const char* libfive_git_version(void);

const char* libfive_git_revision(void);

const char* libfive_git_branch(void);

const std::vector<int> MOSAIC_SHAPE = {4, 4};

void initLogging(enum boost::log::trivial::severity_level severity);

float clip(float n, float lower, float upper);

void wait(int milliseconds);


void clamp(cv::Mat& mat, cv::Range bounds);

// rescales a cv::Mat image to have value ranges from 0 - high
void rescale(cv::Mat& mat, float high);

// create LUT to show saturation
cv::Mat CreateLut(cv::Vec3b saturation_color, cv::Vec3b dark_color);

// we collect the command line arguments in this global struct
struct CommandLineArguments {
    std::string model_file;
    std::string trained_file;
    std::string white_file;
    std::string dark_file;
    std::string output_folder;
    bool test_mode;
};

extern struct CommandLineArguments g_commandLineArguments;

#endif // UTIL_H
