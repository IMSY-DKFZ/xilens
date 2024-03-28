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
/**
 * Variable definitions for newer versions of OpenCV
 */
enum {
    CV_LOAD_IMAGE_ANYDEPTH = cv::IMREAD_ANYDEPTH,
    CV_LOAD_IMAGE_ANYCOLOR = cv::IMREAD_COLOR,
    CV_EVENT_LBUTTONDOWN = cv::EVENT_LBUTTONDOWN,
    CV_EVENT_LBUTTONUP = cv::EVENT_LBUTTONUP,
};
#endif

/**
 * Handles the result from the XiAPI, shows an error message and throws a runtime error if not XI_OK
 * @throw runtime
 */
#define HandleResult(res, place) \
    if (res != XI_OK) { \
        std::stringstream errormsg; \
        errormsg << "Error after " << place << " " << res << "\n"; \
        throw std::runtime_error(errormsg.str()); \
    }


class FileImage {
    FILE *file;
public:
    /**
     * Opens a file and throws runtime error when opening fails
     * @param filename path to file to open
     * @param mode mode in which the file should be open, e.g. "r"
     */
    FileImage(const char *filename, const char *mode);

    /**
     * Closes file when object is destructed
     */
    ~FileImage();

    /**
     * Writes the content of an image into a file in UINT16 format
     * @param image Ximea image where data is stored
     */
    void write(XI_IMG image);
};

// variables where git repo variables are stored
extern "C"
{
extern const char *GIT_TAG;
extern const char *GIT_REV;
extern const char *GIT_BRANCH;
}

/**
 * Queries Git tag
 * @return git tag
 */
const char *libfiveGitVersion(void);

/**
 * Queries Git revision number
 * @return git revision
 */
const char *libfiveGitRevision(void);

/**
 * Queries Git branch name
 * @return git branch name
 */
const char *libfiveGitBranch(void);

/**
 * Initializes the logging by setting a severity
 * @param severity level of logging to set
 */
void initLogging(enum boost::log::trivial::severity_level severity);

/**
 * waits a certain amount of milliseconds on a boost thread
 * @param milliseconds amount of time to wait
 */
void wait(int milliseconds);

/**
 * Restricts the values in a matrix to the range defined by bounds
 * @param mat matrix of values to restrict
 * @param bounds range of values
 */
void clamp(cv::Mat &mat, cv::Range bounds);

/**
 * Rescales values to a range defined by high, lower bound is always 0
 * @param mat matrix values to rescale
 * @param high maximum value that defines the range
 */
void rescale(cv::Mat &mat, float high);

/**
 * Created a look up table (LUT) that can be used to define the colors of pixels in an image that are over-saturated or
 * under-exposed.
 * @param saturation_color color of pixels that are over-saturated
 * @param dark_color color of pixels that are under-exposed
 * @return matrix with LUT
 */
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

/**
 * @brief Helper function which wraps a ximea image in a cv::Mat
 *
 * @param xi_img input ximea image
 * @param mat_img output cv::Mat image
 */
void XIIMGtoMat(XI_IMG &xi_img, cv::Mat &mat_img);

/**
 * Contains the CLI arguments that can be used through a terminal
 */
extern struct CommandLineArguments g_commandLineArguments;

#endif // UTIL_H
