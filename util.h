/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#ifndef UTIL_H
#define UTIL_H

#include <cstdio>
#include <string>
#include <stdexcept>
#include <iostream>

#include <xiApi.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <boost/log/trivial.hpp>
#if (CV_VERSION_MAJOR == 4)
enum {
    CV_LOAD_IMAGE_ANYDEPTH = cv::IMREAD_ANYDEPTH,
    CV_LOAD_IMAGE_ANYCOLOR = cv::IMREAD_COLOR,
    CV_EVENT_LBUTTONDOWN = cv::EVENT_LBUTTONDOWN,
    CV_EVENT_LBUTTONUP = cv::EVENT_LBUTTONUP,
};
#endif

#define HandleResult(res, place) if (res!=XI_OK) {std::stringstream errormsg; errormsg << "Error after " << place << " " << res << "\n"; throw std::runtime_error(errormsg.str());}


class FileImage {
    FILE *file;
public:
    FileImage(const char *filename, const char *mode);

    ~FileImage();

    void write(XI_IMG image);
};

extern "C"
{
extern const char *GIT_TAG;
extern const char *GIT_REV;
extern const char *GIT_BRANCH;
}

const char *libfive_git_version(void);

const char *libfive_git_revision(void);

const char *libfive_git_branch(void);

const std::vector<int> MOSAIC_SHAPE = {4, 4};

void initLogging(enum boost::log::trivial::severity_level severity);

float clip(float n, float lower, float upper);

void wait(int milliseconds);


void clamp(cv::Mat &mat, cv::Range bounds);

// rescales a cv::Mat image to have value ranges from 0 - high
void rescale(cv::Mat &mat, float high);

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
