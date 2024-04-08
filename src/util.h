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
#include <b2nd.h>
#include <msgpack.hpp>

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


/**
 * Handles the result after BLOSC2 operations when return code is != 0
 * @throws runtime
 */
#define HandleBLOSCResult(res, place) \
    if (res != 0) { \
        std::stringstream errormsg; \
        errormsg << "Error after " << place << " " << res << "\n"; \
        throw std::runtime_error(errormsg.str()); \
    }


class FileImage {
public:
    /**
     * exposure time in microseconds
     */
    std::vector<int> m_exposureMetadata;

    /**
     * number of frame acquired by the camera
     */
    std::vector<int> m_acqNframeMetadata;

    /**
     * string determining the type of filter array of an RGB camera
     */
    std::vector<std::string> m_colorFilterArray;

    /**
     * path to file location
     */
    char *filePath;

    /**
     * Storage context
     */
    b2nd_context_t* ctx;

    /**
     * Array storage created temporarily for BLOSC
     */
    b2nd_array_t *src;  // New member to store array

    /**
     * Opens a file and throws runtime error when opening fails
     * @param filePath path to file to open
     */
    FileImage(const char *filePath, unsigned int imageHeight, unsigned int imageWidth);

    /**
     * Closes file when object is destructed
     */
    ~FileImage();

    /**
     * Writes the content of an image into a file in UINT16 format
     * @param image Ximea image where data is stored
     */
    void write(XI_IMG image);

    void AppendMetadata();
};

// variables where git repo variables are stored
extern "C"
{
extern const char *GIT_TAG;
extern const char *GIT_REV;
extern const char *GIT_BRANCH;
}


/**
 * Appends variable length metadata to a BLOSC n-dimensional array
 *
 * @param src BLOSC n-dimensional array where the metadata will be added
 * @param key string to be used as a key for naming the medata data variable
 * @param newData data package with `Message Pack <https://msgpack.org/>`_.
 */
void AppendBLOSCVLMetadata(b2nd_array_t *src, const char *key, msgpack::sbuffer &newData);

/**
 *
 * @tparam T
 * @param src
 * @param key
 * @param data
 */
template<typename T>
void PackAndAppendMetadata(b2nd_array_t *src, const char *key, const std::vector<T>& metadata);

/**
 *
 * @param colorFilterArray
 * @return
 */
std::string colorFilterToString(XI_COLOR_FILTER_ARRAY colorFilterArray);

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
