/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include "logger.h"
#include <opencv2/core/core.hpp>
#include <string>
#include <iostream>
#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <blosc2.h>

#include "util.h"
#include "constants.h"


/**
 * Opens a file and throws runtime error when opening fails
 * @param filePath path to file to open
 */
FileImage::FileImage(const char *filePath, unsigned int imageHeight, unsigned int imageWidth) {
    this->filePath = strdup(filePath);
    blosc2_init();
    blosc2_cparams cparams = BLOSC2_CPARAMS_DEFAULTS;
    cparams.typesize = sizeof(uint16_t);
    cparams.compcode = BLOSC_ZSTD;
    cparams.filters[BLOSC2_MAX_FILTERS - 2] = BLOSC_BITSHUFFLE;
    cparams.filters[BLOSC2_MAX_FILTERS - 1] = BLOSC_SHUFFLE;
    cparams.clevel = 5;
    cparams.nthreads = 4;

    blosc2_storage storage = BLOSC2_STORAGE_DEFAULTS;
    storage.contiguous = true;
    storage.cparams = &cparams;
    storage.urlpath = this->filePath;

    // Shape of the ndarray
    int64_t shape[] = {0, imageHeight, imageWidth};
    // Set Chunk shape and BlockShape as per your requirement.
    int32_t chunk_shape[] = {1, static_cast<int>(imageHeight), static_cast<int>(imageWidth)};
    int32_t block_shape[] = {1, static_cast<int>(imageHeight), static_cast<int>(imageWidth)};

    this->ctx = b2nd_create_ctx(&storage, 3, shape, chunk_shape, block_shape, "|u2", DTYPE_NUMPY_FORMAT, nullptr, 0);
    int result;
    if (access(this->filePath, F_OK) != -1) {
        result = b2nd_open(this->filePath, &src);
    } else {
        result = b2nd_empty(this->ctx, &src);
    }
    HandleBLOSCResult(result, "b2nd_empty || b2nd_open");
}


/**
 * Closes file when object is destructed and add metadata
 */
FileImage::~FileImage() {
    // free BLOSC resources
    b2nd_free(this->src);
    b2nd_free_ctx(this->ctx);
    blosc2_destroy();
}


void FileImage::AppendMetadata() {
    // pack and append metadata
    PackAndAppendMetadata(this->src, "exposure_us", this->m_exposureMetadata);
    PackAndAppendMetadata(this->src, "acq_nframe", this->m_acqNframeMetadata);
    PackAndAppendMetadata(this->src, "color_filter_array", this->m_colorFilterArray);
    LOG_SUSICAM(info) << "Metadata was written to file";
}


/**
 * Writes the content of an image into a file in UINT16 format and associated metadata
 * @param image Ximea image where data is stored
 */
void FileImage::write(XI_IMG image) {
    const int64_t buffer_size = image.width * image.height * sizeof(uint16_t);
    int result = b2nd_append(src, image.bp, buffer_size, 0);
    HandleBLOSCResult(result, "b2nd_append");
    // store metadata
    this->m_exposureMetadata.emplace_back(image.exposure_time_us);
    this->m_acqNframeMetadata.emplace_back(image.acq_nframe);
    this->m_colorFilterArray.emplace_back(colorFilterToString(image.color_filter_array));
}


template<typename T>
void PackAndAppendMetadata(b2nd_array_t *src, const char *key, const std::vector<T>& metadata) {
    // pack metadata and add it to array
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, metadata);
    AppendBLOSCVLMetadata(src, key, sbuf);
}


std::string colorFilterToString(XI_COLOR_FILTER_ARRAY colorFilterArray) {
    switch (colorFilterArray)
    {
        case XI_CFA_NONE: return "XI_CFA_NONE";
        case XI_CFA_BAYER_RGGB: return "XI_CFA_BAYER_RGGB";
        case XI_CFA_CMYG: return "XI_CFA_CMYG";
        case XI_CFA_RGR: return "XI_CFA_RGR";
        case XI_CFA_BAYER_BGGR: return "XI_CFA_BAYER_BGGR";
        case XI_CFA_BAYER_GRBG: return "XI_CFA_BAYER_GRBG";
        case XI_CFA_BAYER_GBRG: return "XI_CFA_BAYER_GBRG";
        case XI_CFA_POLAR_A_BAYER_BGGR: return "XI_CFA_POLAR_A_BAYER_BGGR";
        case XI_CFA_POLAR_A: return "XI_CFA_POLAR_A";
        default: return "Invalid XI_COLOR_FILTER_ARRAY value";
    }
}


void AppendBLOSCVLMetadata(b2nd_array_t *src, const char *key, msgpack::sbuffer &newData){
    // Get the existing data
    uint8_t *content = nullptr;
    int32_t content_len;
    int result;
    int metadataExists = blosc2_vlmeta_exists(src->sc, key);
    if (metadataExists < 0){
        result = blosc2_vlmeta_add(src->sc, key,
                                   reinterpret_cast<uint8_t *>(newData.data()), newData.size(),
                                   nullptr);
        if (result < 0){
            throw std::runtime_error("Error when using blosc2_vlmeta_add");
        }
        return;
    } else {
        result = blosc2_vlmeta_get(src->sc, key, &content, &content_len);
        if (result < 0){
            throw std::runtime_error("Error when using blosc2_vlmeta_get");
        }

        // Unpack the existing data
        msgpack::unpacker oldpac;
        oldpac.reserve_buffer(content_len);
        memcpy(oldpac.buffer(), content, content_len);
        oldpac.buffer_consumed(content_len);

        // Unpack the new data
        msgpack::unpacker newpac;
        newpac.reserve_buffer(newData.size());
        memcpy(newpac.buffer(), newData.data(), newData.size());
        newpac.buffer_consumed(newData.size());

        msgpack::object_handle oldoh;
        oldpac.next(oldoh);

        msgpack::object_handle newoh;
        newpac.next(newoh);

        msgpack::sbuffer sbuf;

        // Assume that the existing and new data should have the same type
        if (oldoh.get().type == msgpack::type::ARRAY && oldoh.get().via.array.size > 0 && oldoh.get().via.array.ptr[0].type == msgpack::type::STR) {
            // It's a vector of strings
            auto oldData = oldoh.get().as<std::vector<std::string>>();
            auto appendData = newoh.get().as<std::vector<std::string>>();

            oldData.insert(oldData.end(), appendData.begin(), appendData.end());

            msgpack::pack(sbuf, oldData);
        }
        else {
            // It's a vector of ints
            auto oldData = oldoh.get().as<std::vector<int>>();
            auto appendData = newoh.get().as<std::vector<int>>();

            oldData.insert(oldData.end(), appendData.begin(), appendData.end());

            msgpack::pack(sbuf, oldData);
        }

        // Update the metadata with the new data
        result = blosc2_vlmeta_update(src->sc, key, reinterpret_cast<uint8_t *>(sbuf.data()), sbuf.size(), NULL);
        if (result < 0){
            throw std::runtime_error("Error when using blosc2_vlmeta_update");
        }
    }
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
        if (i > OVEREXPOSURE_PIXEL_BOUNDARY_VALUE) {
            Lut.at<cv::Vec3b>(0, i) = saturation_color;
        }
        if (i < UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE) {
            Lut.at<cv::Vec3b>(0, i) = dark_color;
        }
    }
    return Lut;
}

/**
 * @brief Helper function which wraps a ximea image in a cv::Mat
 *
 * @param xi_img input ximea image
 * @param mat_img output cv::Mat image
 */
void XIIMGtoMat(XI_IMG &xi_img, cv::Mat &mat_img) {
    mat_img = cv::Mat(xi_img.height, xi_img.width, CV_16UC1, xi_img.bp);
}

/**
 * Contains the CLI arguments that can be used through a terminal
 */
struct CommandLineArguments g_commandLineArguments;
