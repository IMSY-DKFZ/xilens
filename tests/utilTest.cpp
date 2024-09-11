/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include <blosc2.h>
#include <gtest/gtest.h>

#include "src/constants.h"
#include "src/util.h"

TEST(UtilTest, HandleResultTest)
{
    int cam_status = 0;
    ASSERT_NO_THROW(HandleResult(cam_status, "test camera status"));
    cam_status = 1;
    ASSERT_THROW(HandleResult(cam_status, "test camera status"), std::runtime_error);
}

TEST(CreateLutTest, VerifyLutColorValues)
{
    cv::Vec3b test_saturation_color(255, 255, 255);
    cv::Vec3b test_dark_color(0, 0, 0);

    cv::Mat result_lut = CreateLut(test_saturation_color, test_dark_color);

    ASSERT_EQ(result_lut.cols, 256);
    ASSERT_EQ(result_lut.type(), CV_8UC3);

    for (uint i = 0; i < 256; ++i)
    {
        cv::Vec3b expected_color;
        if (i > OVEREXPOSURE_PIXEL_BOUNDARY_VALUE)
        {
            expected_color = test_saturation_color;
        }
        else if (i < UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE)
        {
            expected_color = test_dark_color;
        }
        else
        {
            expected_color = cv::Vec3b(i, i, i);
        }
        ASSERT_EQ(result_lut.at<cv::Vec3b>(0, i), expected_color);
    }
}

TEST(XIIMGtoMatTest, MatDimensionsEqualToXIIMG)
{
    XI_IMG xi_img;
    xi_img.width = 640;
    xi_img.height = 480;
    xi_img.bp = malloc(static_cast<size_t>(xi_img.width) * static_cast<size_t>(xi_img.height));
    cv::Mat mat_img;

    XIIMGtoMat(xi_img, mat_img);

    ASSERT_EQ(mat_img.cols, xi_img.width);
    ASSERT_EQ(mat_img.rows, xi_img.height);

    free(xi_img.bp);
}

class FileImageWriteTest : public ::testing::Test
{
  protected:
    // This function runs after each test
    ~FileImageWriteTest() override = default;
};

TEST_F(FileImageWriteTest, CheckContentsAfterWriting)
{
    uint32_t nrImages = 10;
    XI_IMG xiImage;
    xiImage.width = 64;
    xiImage.height = 64;
    xiImage.exposure_time_us = 40000;
    xiImage.bp = malloc(static_cast<size_t>(xiImage.width) * static_cast<size_t>(xiImage.height) * sizeof(uint16_t));
    std::fill_n((uint16_t *)xiImage.bp, xiImage.width * xiImage.height, 12345);
    const char *urlpath = strdup("test_image.b2nd");

    blosc2_init();
    blosc2_remove_urlpath(urlpath);

    FileImage fileImage(urlpath, xiImage.height, xiImage.width);
    QMap<QString, float> additionalMetadata = {{"extraMetadata", 1.0}};
    for (int i = 0; i < nrImages; i++)
    {
        fileImage.WriteImageData(xiImage, additionalMetadata);
    }
    fileImage.AppendMetadata();

    b2nd_array_t *src;
    b2nd_open(urlpath, &src);

    size_t total_size = static_cast<uint64_t>(nrImages) * xiImage.width * xiImage.height * sizeof(uint16_t);
    if (total_size > static_cast<uint64_t>(INT64_MAX))
    {
        throw std::overflow_error("Array size exceeds the maximum value of int64_t.");
    }
    auto array_size = static_cast<int64_t>(total_size);
    auto data_back = (uint16_t *)malloc(array_size);
    int rval = b2nd_to_cbuffer(src, data_back, array_size);
    if (rval < 0)
    {
        free(xiImage.bp);
        free(data_back);
        FAIL() << "Failed to load data from b2nd array.";
    }

    for (int i = 0; i < xiImage.width * xiImage.height * nrImages; i++)
    {
        if (data_back[i] != 12345)
        {
            free(xiImage.bp);
            free(data_back);
            FAIL() << "Data stored in B2NDArray does not match original created data.";
        }
    }

    // check metadata exists
    std::vector<std::string> keys = {"exposure_us", "acq_nframe", "color_filter_array", "time_stamp", "extraMetadata"};
    const size_t NAME_BUFFER_SIZE = src->sc->nvlmetalayers * sizeof(char *);
    char **names = (char **)malloc(NAME_BUFFER_SIZE);
    int nkeys = blosc2_vlmeta_get_names(src->sc, names);
    for (const auto &key : keys)
    {
        bool found = false;
        for (int i = 0; i < nkeys; i++)
        {
            if (key == std::string(names[i]))
            {
                found = true;
                uint8_t *content = nullptr;
                int32_t content_len = 0;
                int result = blosc2_vlmeta_get(src->sc, names[i], &content, &content_len);

                if (result < 0 || content == nullptr)
                {
                    FAIL() << "Failed to retrieve metadata content for key " << key << std::endl;
                }
                else
                {
                    msgpack::unpacker pac;
                    pac.reserve_buffer(content_len);
                    memcpy(pac.buffer(), content, content_len);
                    pac.buffer_consumed(content_len);

                    msgpack::object_handle oh;
                    while (pac.next(oh))
                    {
                        const msgpack::object &obj = oh.get();

                        std::cout << "Content for key " << key << ": ";

                        if (obj.type == msgpack::type::ARRAY && obj.via.array.size > 0)
                        {
                            for (uint32_t j = 0; j < obj.via.array.size; ++j)
                            {
                                const msgpack::object &entry = obj.via.array.ptr[j];

                                switch (entry.type)
                                {
                                case msgpack::type::FLOAT32:
                                case msgpack::type::FLOAT64: {
                                    float value = entry.as<float>();
                                    std::cout << value << " ";
                                    if (value != 1.0)
                                    {
                                        FAIL() << "Invalid floating stored metadata: " << value;
                                    }
                                    break;
                                }
                                case msgpack::type::POSITIVE_INTEGER:
                                case msgpack::type::NEGATIVE_INTEGER: {
                                    int value = entry.as<int>();
                                    std::cout << value << " ";
                                    break;
                                }
                                case msgpack::type::STR: {
                                    std::string value = entry.as<std::string>();
                                    std::cout << "\"" << value << "\" ";
                                    break;
                                }
                                default: {
                                    std::cerr << "Unhandled MsgPack type for key " << key << std::endl;
                                    break;
                                }
                                }
                            }
                        }
                        else
                        {
                            std::cerr << "Unexpected metadata type or empty array for key " << key << std::endl;
                        }

                        std::cout << std::endl;
                    }
                }
                break;
            }
        }

        if (!found)
        {
            free(xiImage.bp);
            free(data_back);
            for (int i = 0; i < nkeys; i++)
            {
                free(names[i]);
            }
            free(names);
            blosc2_remove_urlpath(urlpath);
            b2nd_free(src);
            FAIL() << "Failed: key " << key << " is not present in the variable length metadata";
        }
    }

    free(xiImage.bp);
    free(data_back);
    for (int i = 0; i < nkeys; i++)
    {
        free(names[i]);
    }
    free(names);
    blosc2_remove_urlpath(urlpath);
    blosc2_destroy();
}

TEST_F(FileImageWriteTest, AppendMetadataTwice)
{
    uint32_t nrImages = 10;
    XI_IMG xiImage;
    xiImage.width = 64;
    xiImage.height = 64;
    xiImage.exposure_time_us = 40000;
    xiImage.bp = malloc(static_cast<size_t>(xiImage.width) * static_cast<size_t>(xiImage.height) * sizeof(uint16_t));
    std::fill_n((uint16_t *)xiImage.bp, xiImage.width * xiImage.height, 12345);
    const char *urlpath = strdup("test_image.b2nd");

    blosc2_init();
    blosc2_remove_urlpath(urlpath);

    FileImage fileImage(urlpath, xiImage.height, xiImage.width);
    QMap<QString, float> additionalMetadata = {{"extraMetadata", 1.0}};
    for (int i = 0; i < nrImages; i++)
    {
        fileImage.WriteImageData(xiImage, additionalMetadata);
    }
    fileImage.AppendMetadata();

    // append more images to the file
    for (int i = 0; i < nrImages; i++)
    {
        fileImage.WriteImageData(xiImage, additionalMetadata);
    }
    fileImage.AppendMetadata();
    blosc2_destroy();
}
