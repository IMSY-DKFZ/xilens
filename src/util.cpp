/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#include "util.h"

#include <blosc2.h>

#include <QDateTime>
#include <boost/chrono.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <string>

#include "constants.h"
#include "logger.h"

FileImage::FileImage(const char *filePath, unsigned int imageHeight, unsigned int imageWidth)
{
    this->m_filePath = strdup(filePath);
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
    storage.urlpath = this->m_filePath;

    // Shape of the ndarray
    int64_t shape[] = {0, imageHeight, imageWidth};
    // Set Chunk shape and BlockShape as per your requirement.
    int32_t chunk_shape[] = {1, static_cast<int>(imageHeight), static_cast<int>(imageWidth)};
    int32_t block_shape[] = {1, static_cast<int>(imageHeight), static_cast<int>(imageWidth)};

    this->m_ctx = b2nd_create_ctx(&storage, 3, shape, chunk_shape, block_shape, "|u2", DTYPE_NUMPY_FORMAT, nullptr, 0);
    int result;
    if (access(this->m_filePath, F_OK) != -1)
    {
        result = b2nd_open(this->m_filePath, &m_src);
    }
    else
    {
        result = b2nd_empty(this->m_ctx, &m_src);
    }
    HandleBLOSCResult(result, "b2nd_empty || b2nd_open");
}

FileImage::~FileImage()
{
    // free BLOSC resources
    b2nd_free(this->m_src);
    b2nd_free_ctx(this->m_ctx);
}

void FileImage::AppendMetadata()
{
    // pack and append metadata
    PackAndAppendMetadata(this->m_src, EXPOSURE_KEY, this->m_exposureMetadata);
    PackAndAppendMetadata(this->m_src, FRAME_NUMBER_KEY, this->m_acqNframeMetadata);
    PackAndAppendMetadata(this->m_src, COLOR_FILTER_ARRAY_FORMAT_KEY, this->m_colorFilterArray);
    PackAndAppendMetadata(this->m_src, TIME_STAMP_KEY, this->m_timeStamp);
    for (const QString &key : m_additionalMetadata.keys())
    {
        PackAndAppendMetadata(this->m_src, key.toUtf8().constData(), this->m_additionalMetadata[key]);
    }
    LOG_XILENS(info) << "Metadata was written to file";
}

void FileImage::WriteImageData(XI_IMG image, QMap<QString, float> additionalMetadata)
{
    const size_t buffer_size = static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * sizeof(uint16_t);
    if (buffer_size > static_cast<size_t>(INT64_MAX))
    {
        throw std::overflow_error("Buffer size exceeds the maximum value of int64_t.");
    }
    int result = b2nd_append(m_src, image.bp, static_cast<int64_t>(buffer_size), 0);
    HandleBLOSCResult(result, "b2nd_append");
    // store metadata
    this->m_exposureMetadata.emplace_back(image.exposure_time_us);
    this->m_acqNframeMetadata.emplace_back(image.acq_nframe);
    this->m_colorFilterArray.emplace_back(ColorFilterToString(image.color_filter_array));
    this->m_timeStamp.emplace_back(GetTimeStamp().toStdString());
    for (const QString &key : additionalMetadata.keys())
    {
        m_additionalMetadata[key].push_back(additionalMetadata[key]);
    }
}

template <typename T> void PackAndAppendMetadata(b2nd_array_t *src, const char *key, const std::vector<T> &metadata)
{
    // pack metadata and add it to array
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, metadata);
    try
    {
        AppendBLOSCVLMetadata(src, key, sbuf);
    }
    catch (const std::runtime_error &err)
    {
        LOG_XILENS(error) << "Error while trying to add metadata for key: " << key;
        throw err;
    }
}

std::string ColorFilterToString(XI_COLOR_FILTER_ARRAY colorFilterArray)
{
    switch (colorFilterArray)
    {
    case XI_CFA_NONE:
        return "XI_CFA_NONE";
    case XI_CFA_BAYER_RGGB:
        return "XI_CFA_BAYER_RGGB";
    case XI_CFA_CMYG:
        return "XI_CFA_CMYG";
    case XI_CFA_RGR:
        return "XI_CFA_RGR";
    case XI_CFA_BAYER_BGGR:
        return "XI_CFA_BAYER_BGGR";
    case XI_CFA_BAYER_GRBG:
        return "XI_CFA_BAYER_GRBG";
    case XI_CFA_BAYER_GBRG:
        return "XI_CFA_BAYER_GBRG";
    case XI_CFA_POLAR_A_BAYER_BGGR:
        return "XI_CFA_POLAR_A_BAYER_BGGR";
    case XI_CFA_POLAR_A:
        return "XI_CFA_POLAR_A";
    default:
        return "Invalid XI_COLOR_FILTER_ARRAY value";
    }
}

void AppendBLOSCVLMetadata(b2nd_array_t *src, const char *key, msgpack::sbuffer &newData)
{
    // Get the existing data
    uint8_t *content = nullptr;
    int32_t content_len;
    int result;
    int metadataExists = blosc2_vlmeta_exists(src->sc, key);
    if (metadataExists < 0)
    {
        result = blosc2_vlmeta_add(src->sc, key, reinterpret_cast<uint8_t *>(newData.data()), newData.size(), nullptr);
        if (result < 0)
        {
            throw std::runtime_error("Error when using blosc2_vlmeta_add");
        }
        return;
    }
    else
    {
        result = blosc2_vlmeta_get(src->sc, key, &content, &content_len);
        if (result < 0)
        {
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
        if (oldoh.get().type == msgpack::type::ARRAY && oldoh.get().via.array.size > 0)
        {
            switch (oldoh.get().via.array.ptr[0].type)
            {
            case msgpack::type::STR: {
                // It's a vector of strings
                auto oldData = oldoh.get().as<std::vector<std::string>>();
                auto appendData = newoh.get().as<std::vector<std::string>>();
                oldData.insert(oldData.end(), appendData.begin(), appendData.end());
                msgpack::pack(sbuf, oldData);
                break;
            }
            case msgpack::type::POSITIVE_INTEGER:
            case msgpack::type::NEGATIVE_INTEGER: {
                // It's a vector of ints
                auto oldData = oldoh.get().as<std::vector<int>>();
                auto appendData = newoh.get().as<std::vector<int>>();
                oldData.insert(oldData.end(), appendData.begin(), appendData.end());
                msgpack::pack(sbuf, oldData);
                break;
            }
            case msgpack::type::FLOAT32:
            case msgpack::type::FLOAT: {
                // It's a vector of floats
                auto oldData = oldoh.get().as<std::vector<float>>();
                auto appendData = newoh.get().as<std::vector<float>>();
                oldData.insert(oldData.end(), appendData.begin(), appendData.end());
                msgpack::pack(sbuf, oldData);
                break;
            }
            default: {
                LOG_XILENS(error) << "Cannot handle MsgPack data type: " << oldoh.get().via.array.ptr[0].type;
                throw std::runtime_error("Unhandled MsgPack type.");
            }
            }
        }
        else
        {
            throw std::runtime_error("Unexpected metadata type or empty array.");
        }

        // Update the metadata with the new data
        result = blosc2_vlmeta_update(src->sc, key, reinterpret_cast<uint8_t *>(sbuf.data()), sbuf.size(), NULL);
        if (result < 0)
        {
            throw std::runtime_error("Error when using blosc2_vlmeta_update");
        }
    }
}

void WaitMilliseconds(int milliseconds)
{
    boost::this_thread::sleep_for(boost::chrono::milliseconds(milliseconds));
}

cv::Mat CreateLut(cv::Vec3b saturation_color, cv::Vec3b dark_color)
{
    cv::Mat Lut(1, 256, CV_8UC3);
    for (uint i = 0; i < 256; ++i)
    {
        Lut.at<cv::Vec3b>(0, i) = cv::Vec3b(i, i, i);
        if (i > OVEREXPOSURE_PIXEL_BOUNDARY_VALUE)
        {
            Lut.at<cv::Vec3b>(0, i) = saturation_color;
        }
        if (i < UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE)
        {
            Lut.at<cv::Vec3b>(0, i) = dark_color;
        }
    }
    return Lut;
}

void XIIMGtoMat(XI_IMG &xi_img, cv::Mat &mat_img)
{
    mat_img = cv::Mat(xi_img.height, xi_img.width, CV_16UC1, xi_img.bp);
}

QString GetTimeStamp()
{
    QString timestamp;
    QString curr_time = (QTime::currentTime()).toString("hh-mm-ss-zzz");
    QString date = (QDate::currentDate()).toString("yyyyMMdd_");
    timestamp = date + curr_time;
    return timestamp;
}

struct CommandLineArguments g_commandLineArguments;
