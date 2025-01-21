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
#include <msgpack.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <string>

/**
 * @brief Handles the result from the XiAPI, shows an error message and throws a
 * runtime error if not XI_OK
 *
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
 * @brief Handles the result after BLOSC2 operations when return code is != 0
 *
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
     * @brief exposure time in microseconds.
     */
    std::vector<int> m_exposureMetadata;

    /**
     * @brief number of frame acquired by the camera.
     */
    std::vector<int> m_acqNframeMetadata;

    /**
     * @brief string determining the type of filter array of an RGB camera.
     */
    std::vector<std::string> m_colorFilterArray;

    /**
     * @brief string determining the time stamp when images where acquired.
     */
    std::vector<std::string> m_timeStamp;

    /**
     * @brief additional metadata to append to the NDArrays. Each vector will be appended to the vl metadata of the
     * array using the key of the map as identifier.
     */
    QMap<QString, std::vector<float>> m_additionalMetadata;

    /**
     * @brief path to file location.
     */
    char *m_filePath;

    /**
     * @brief Storage context.
     */
    b2nd_context_t *m_ctx;

    /**
     * @brief Array storage created temporarily for BLOSC.
     */
    b2nd_array_t *m_src; // New member to store array

    /**
     * @brief Opens a file and throws runtime error when opening fails.
     *
     * @param filePath path to file to open
     * @param imageHeight height of image to store in file
     * @param imageWidth width of image to store in file
     * @throws XiLensError when initializing file fails
     */
    FileImage(const char *filePath, unsigned int imageHeight, unsigned int imageWidth);

    /**
     * @brief Frees blosc2 context and releases the resources associated with the file.
     */
    ~FileImage();

    /**
     * @brief Writes the content of an image into a file in UINT16 format.
     *
     * @param image Ximea image where data is stored
     * @param additionalMetadata Additional metadata to be stored in the array
     */
    void WriteImageData(XI_IMG image, QMap<QString, float> additionalMetadata);

    /**
     *@brief Appends metadata to BLOSC ND array. This method should be called before
     * closing the file.
     */
    void AppendMetadata();

    /**
     * @brief Checks for each expected metadata key, that the length matches the number of images
     * in the file. Returns false if metadata has inconsistent shape or if it does not exist for any key.
     *
     * @param src BLOSC ND-Array to check.
     * @return `true` if the metadata is consistent and `false` if the metadata shape is missing or shape is
     * inconsistent.
     */
    static bool CheckFileMetadata(const b2nd_array_t *src);
};

/**
 * @brief Appends variable length metadata to a BLOSC n-dimensional array
 *
 * @param src BLOSC n-dimensional array where the metadata will be added
 * @param key string to be used as a key for naming the medata data variable
 * @param newData data package with `Message Pack <https://msgpack.org/>`_.
 */
void AppendBLOSCVLMetadata(b2nd_array_t *src, const char *key, msgpack::sbuffer &newData);

/**
 * @brief Unpacks the metadata in array and computes the number of elements corresponding to the specified key.
 *
 * @param src BLOSC ND-Array from which the metadata should be analyzed.
 * @param key Identifier of the metadata layer from which the number of elements is desired.
 * @return number of elements in metadata layer or negative value if an error occurred.
 */
int GetBLOSCVLMetadataLength(const b2nd_array_t *src, const char *key);

/**
 * @brief Packs and appends the metadata associated with a BLOSC NDarray
 *
 * @tparam T data type of the metadata
 * @param src pointer to BLOSC array where the metadata will be appended
 * @param key string that will be used to identify the metadata inside the array
 * @param metadata content to be stored in the metadata
 */
template <typename T> void PackAndAppendMetadata(b2nd_array_t *src, const char *key, const std::vector<T> &metadata);

/**
 * @brief Converts the XIMEA color filter array identifier to a string representation
 *
 * @param colorFilterArray XIMEA color filter array representation
 * @return string representing the color filter array
 */
std::string ColorFilterToString(XI_COLOR_FILTER_ARRAY colorFilterArray);

/**
 * @brief waits a certain amount of milliseconds on a boost thread.
 * @param milliseconds amount of time to WaitMilliseconds
 */
void WaitMilliseconds(int milliseconds);

/**
 * @brief Created a look up table (LUT) that can be used to define the colors of pixels
 * in an image that are over-saturated or under-exposed.
 *
 * @param saturation_color color of pixels that are over-saturated
 * @param dark_color color of pixels that are under-exposed
 * @param minValue saturation minimum value.
 * @param maxValue saturation maximum value.
 * @return matrix with LUT.
 */
cv::Mat CreateLut(const cv::Vec3b &saturation_color, const cv::Vec3b &dark_color, int minValue, int maxValue);

/**
 * @brief Structure used to store command line arguments parsed by the user.
 *
 * Contains the CLI arguments that can be parsed through the terminal by the user.
 */
struct CommandLineArguments
{
    bool test_mode;
    bool version;
};

/**
 * @brief Helper function which wraps a ximea image in a cv::Mat
 *
 * @param xi_img input ximea image
 * @param mat_img output cv::Mat image
 */
void XIIMGtoMat(const XI_IMG &xi_img, cv::Mat &mat_img);

/**
 * @brief Generates a timestamp with the format `yyyyMMdd_hh-mm-ss-zzz`.
 */
QString GetTimeStamp();

/**
 * @brief Contains the CLI arguments that can be parsed through the terminal by the user. This is initialized at start
 * of the program, before the `Qt` application is initialized.
 */
extern CommandLineArguments g_commandLineArguments;

#endif // UTIL_H
