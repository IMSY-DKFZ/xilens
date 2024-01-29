/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <opencv2/imgproc.hpp>

#include "util.h"


/**
 * Opens a file and throws runtime error when opening fails
 * @param filename path to file to open
 * @param mode mode in which the file should be open, e.g. "r"
 */
FileImage::FileImage(const char *filename, const char *mode) {
    file = fopen(filename, mode);
    if (!file) {
        throw std::runtime_error("Could not open the file.");
    }
}

/**
 * Closes file when object is destructed
 */
FileImage::~FileImage() {
    fclose(file);
}

/**
 * Writes the content of an image into a file in UINT16 format
 * @param image Ximea image where data is stored
 */
void FileImage::write(XI_IMG image) {
    fwrite(image.bp, image.width * image.height, sizeof(UINT16), file);
}

/**
 * Queries Git tag
 * @return git tag
 */
const char *libfiveGitVersion(void) {
    return GIT_TAG;
}

/**
 * Queries Git revision number
 * @return git revision
 */
const char *libfiveGitRevision(void) {
    return GIT_REV;
}

/**
 * Queries Git branch name
 * @return git branch name
 */
const char *libfiveGitBranch(void) {
    return GIT_BRANCH;
}

/**
 * Rescales values to a range defined by high
 * @param mat matrix values to rescale
 * @param high maximum value that defines the range
 */
void rescale(cv::Mat &mat, float high) {
    double min, max;
    cv::minMaxLoc(mat, &min, &max);
    mat = (mat - ((float) min)) * high / ((float) max - min);
}

/**
 * Restricts the values in a matrix to the range defined by bounds
 * @param mat matrix of values to restrict
 * @param bounds range of values
 */
void clamp(cv::Mat &mat, cv::Range bounds) {
    cv::min(cv::max(mat, bounds.start), bounds.end, mat);
}

/**
 * waits a certain amount of milliseconds on a boost thread
 * @param milliseconds amount of time to wait
 */
void wait(int milliseconds) {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(milliseconds));
}

/**
 * Initializes the logging by setting a severity
 * @param severity level of logging to set
 */
void initLogging(enum boost::log::trivial::severity_level severity) {
    boost::log::core::get()->set_filter
            (
                    boost::log::trivial::severity >= severity
            );
}

/**
 * Created a look up table (LUT) that can be used to define the colors of pixels in an image that are over-saturated or
 * under-exposed.
 * @param saturation_color color of pixels that are over-saturated
 * @param dark_color color of pixels that are under-exposed
 * @return matrix with LUT
 */
cv::Mat CreateLut(cv::Vec3b saturation_color, cv::Vec3b dark_color) {
    cv::Mat Lut(1, 256, CV_8UC3);
    for (uint i = 0; i < 256; ++i) {
        Lut.at<cv::Vec3b>(0, i) = cv::Vec3b(i, i, i);
        if (i > 225) {
            Lut.at<cv::Vec3b>(0, i) = saturation_color;
        }
        if (i < 10) {
            Lut.at<cv::Vec3b>(0, i) = dark_color;
        }
    }
    return Lut;
}

/**
 * Contains the CLI arguments that can be used through a terminal
 */
struct CommandLineArguments g_commandLineArguments;
