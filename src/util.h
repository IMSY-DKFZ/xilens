/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef UTIL_H
#define UTIL_H

#include <b2nd.h>
#include <xiApi.h>

#include <QMap>
#include <QString>
#include <boost/log/trivial.hpp>
#include <cstdio>
#include <iostream>
#include <msgpack.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdexcept>
#include <string>

/**
 * Handles the result from the XiAPI, shows an error message and throws a
 * runtime error if not XI_OK
 * @throw runtime
 */
#define HandleResult(res, place)                                                                                       \
    if (res != XI_OK)                                                                                                  \
    {                                                                                                                  \
        std::stringstream errormsg;                                                                                    \
        errormsg << "Error after " << place << " " << res << "\n";                                                     \
        throw std::runtime_error(errormsg.str());                                                                      \
    }

/**
 * Handles the result after BLOSC2 operations when return code is != 0
 * @throws runtime
 */
#define HandleBLOSCResult(res, place)                                                                                  \
    if (res != 0)                                                                                                      \
    {                                                                                                                  \
        std::stringstream errormsg;                                                                                    \
        errormsg << "Error after " << place << " " << res << "\n";                                                     \
        throw std::runtime_error(errormsg.str());                                                                      \
    }

/**
 * @brief Image container responsible of writing images to a file, including metadata.
 *
 * This class manges the writing of images to a file. Writing metadata to the file needs to be triggered through the
 * method FileImage::AppendMetadata.
 */
class FileImage
{
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
     * string determining the time stamp when images where acquired
     */
    std::vector<std::string> m_timeStamp;

    /**
     * additional metadata to append to the NDArrays. Each vector will be appended to the vl metadata of the array
     * using the key of the map as identifier.
     */
    QMap<QString, std::vector<float>> m_additionalMetadata;

    /**
     * path to file location
     */
    char *m_filePath;

    /**
     * Storage context
     */
    b2nd_context_t *m_ctx;

    /**
     * Array storage created temporarily for BLOSC
     */
    b2nd_array_t *m_src; // New member to store array

    /**
     * Opens a file and throws runtime error when opening fails
     * @param filePath path to file to open
     */
    FileImage(const char *filePath, unsigned int imageHeight, unsigned int imageWidth);

    /**
     * Frees blosc2 context and releases the resources associated with the file.
     */
    ~FileImage();

    /**
     * Writes the content of an image into a file in UINT16 format
     * @param image Ximea image where data is stored
     * @param additionalMetadata Additional metadata to be stored in the array
     */
    void WriteImageData(XI_IMG image, QMap<QString, float> additionalMetadata);

    /**
     * Appends metadata to BLOSC ND array. This method should be called before
     * closing the file.
     *
     */
    void AppendMetadata();
};

/**
 * Appends variable length metadata to a BLOSC n-dimensional array
 *
 * @param src BLOSC n-dimensional array where the metadata will be added
 * @param key string to be used as a key for naming the medata data variable
 * @param newData data package with `Message Pack <https://msgpack.org/>`_.
 */
void AppendBLOSCVLMetadata(b2nd_array_t *src, const char *key, msgpack::sbuffer &newData);

/**
 * Packs and appends the metadata associated with a BLOSC NDarray
 *
 * @tparam T data type of the metadata
 * @param src pointer to BLOSC array where the metadata will be appended
 * @param key string that will be used to identify the metadata inside the array
 * @param metadata content to be stored in the metadata
 */
template <typename T> void PackAndAppendMetadata(b2nd_array_t *src, const char *key, const std::vector<T> &metadata);

/**
 * Converts the XIMEA color filter array identifier to a string representation
 *
 * @param colorFilterArray XIMEA color filter array representation
 * @return string representing the color filter array
 */
std::string ColorFilterToString(XI_COLOR_FILTER_ARRAY colorFilterArray);

/**
 * waits a certain amount of milliseconds on a boost thread
 * @param milliseconds amount of time to WaitMilliseconds
 */
void WaitMilliseconds(int milliseconds);

/**
 * Created a look up table (LUT) that can be used to define the colors of pixels
 * in an image that are over-saturated or under-exposed.
 * @param saturation_color color of pixels that are over-saturated
 * @param dark_color color of pixels that are under-exposed
 * @return matrix with LUT
 */
cv::Mat CreateLut(cv::Vec3b saturation_color, cv::Vec3b dark_color);

/**
 * @brief Structure used to store command line arguments parsed by the user.
 *
 * Contains the CLI arguments that can be parsed through the terminal by the user.
 */
struct CommandLineArguments
{
    std::string model_file;
    std::string trained_file;
    std::string white_file;
    std::string dark_file;
    std::string output_folder;
    bool test_mode;
    bool version;
};

/**
 * @brief Helper function which wraps a ximea image in a cv::Mat
 *
 * @param xi_img input ximea image
 * @param mat_img output cv::Mat image
 */
void XIIMGtoMat(XI_IMG &xi_img, cv::Mat &mat_img);

/**
 * Generates a timestamp with the format `yyyyMMdd_hh-mm-ss-zzz`
 *
 * @return
 */
QString GetTimeStamp();

/**
 * Contains the CLI arguments that can be parsed through the terminal by the user. This is initialized at start of
 * the program, before the `Qt` application is initialized.
 */
extern struct CommandLineArguments g_commandLineArguments;

#endif // UTIL_H
