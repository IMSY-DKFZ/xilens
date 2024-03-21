/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/

#include <gtest/gtest.h>

#include "src/util.h"
#include "src/constants.h"

TEST(UtilTest, HandleResultTest){
    int cam_status = 0;
    ASSERT_NO_THROW(HandleResult(cam_status, "test camera status"));
    cam_status = 1;
    ASSERT_THROW(HandleResult(cam_status, "test camera status"), std::runtime_error);
}


TEST(CreateLutTest, VerifyLutColorValues) {
    cv::Vec3b test_saturation_color(255, 255, 255);
    cv::Vec3b test_dark_color(0, 0, 0);

    cv::Mat result_lut = CreateLut(test_saturation_color, test_dark_color);

    ASSERT_EQ(result_lut.cols, 256);
    ASSERT_EQ(result_lut.type(), CV_8UC3);

    for(uint i = 0; i < 256; ++i) {
        cv::Vec3b expected_color;
        if (i > OVEREXPOSURE_PIXEL_BOUNDARY_VALUE) {
            expected_color = test_saturation_color;
        } else if (i < UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE) {
            expected_color = test_dark_color;
        } else {
            expected_color = cv::Vec3b(i, i, i);
        }
        ASSERT_EQ(result_lut.at<cv::Vec3b>(0, i), expected_color);
    }
}


TEST(RescaleTest, CheckValuesAfterRescaling){
    cv::Mat mat = (cv::Mat_<float>(3,3) << 0.5, 1.2, 2.4, 3.2, 5.1, 6.3, 10.0, 20.0, 15.0);
    rescale(mat, 100.0);
    double min, max;
    cv::minMaxLoc(mat, &min, &max);
    EXPECT_GE(max, 0);
    EXPECT_LE(max, 100);
    EXPECT_GE(100 , min);
    EXPECT_LE(0 , min);
}


TEST(XIIMGtoMatTest, MatDimensionsEqualToXIIMG){
    XI_IMG xi_img;
    xi_img.width = 640;
    xi_img.height = 480;
    xi_img.bp = malloc(xi_img.width * xi_img.height * sizeof(uint16_t));
    cv::Mat mat_img;

    XIIMGtoMat(xi_img, mat_img);

    ASSERT_EQ(mat_img.cols, xi_img.width);
    ASSERT_EQ(mat_img.rows, xi_img.height);

    // Make sure to free the memory allocated for xi_img.bp
    free(xi_img.bp);
}


class FileImageWriteTest : public ::testing::Test {
protected:
    // This function runs after each test
    ~FileImageWriteTest() override {
        remove("test_image.bin");
    }
};

TEST_F(FileImageWriteTest, CheckContentsAfterWriting){
    XI_IMG xi_img;
    xi_img.width = 64;
    xi_img.height = 64;
    xi_img.bp = malloc(xi_img.width * xi_img.height * sizeof(uint16_t));
    std::fill_n((uint16_t*)xi_img.bp, xi_img.width * xi_img.height, 12345);

    FileImage fileImage("test_image.bin", "w");
    fileImage.write(xi_img);

    FILE* file = fopen("test_image.bin", "rb");
    uint16_t* data_back = (uint16_t*)malloc(xi_img.width * xi_img.height * sizeof(uint16_t));
    fread(data_back, xi_img.width * xi_img.height, sizeof(uint16_t), file);
    fclose(file);

    for(int i = 0; i < xi_img.width * xi_img.height; i++)
        ASSERT_EQ(data_back[i], 12345);

    free(xi_img.bp);
    free(data_back);
}
