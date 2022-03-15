/*
 * ===================================================================
 * Surgical Spectral Imaging Library (SuSI)
 *
 * Copyright (c) German Cancer Research Center,
 * Division of Medical and Biological Informatics.
 * All rights reserved.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 *
 * See LICENSE.txt for details.
 * ===================================================================
 */


#ifndef CAFFE_INTERFACE_H
#define CAFFE_INTERFACE_H

#include <QObject>

#include <caffe/caffe.hpp>

#include <opencv2/core/core.hpp>

#include <string>
#include <vector>
#include <memory>

typedef float dtype;

class Network : public QObject
{
    Q_OBJECT

signals:

    void NewImageProcessed();

public:

    enum InputImage {IMAGE =0, WHITE=1, DARK=2};

    Network(const std::string& network_spec_file,
            const std::string& model_file);

    Network();

    bool NetworkReady();

    void Intialize(const std::string& network_spec_file,
                   const std::string& model_file);

    void SetDark(std::string image_file);

    void SetWhite(std::string image_file);

    void SetImage(float* img, enum InputImage i);

    void SetImage(cv::Mat& img, enum InputImage i);

    void SetImage(std::string image_file, enum InputImage i);

    void SetBinaryImage(std::string image_file, enum InputImage i);

    void GetBand(std::vector<cv::Mat>& band_image, unsigned band_nr);

    void GetBands(cv::Mat& rgb_image);

    std::vector<unsigned>bands = {3, 15, 11};

    void GetBGR(std::vector<cv::Mat>& rgb);

    void GetPhysiologicalParameters(std::vector<cv::Mat>& physParam);


    /**
     * @brief Run the deep network
     */
    void Run();

    void RunNewImage(std::string image_file);

    void RunNewImage(float* img);

    cv::Size GetGeometry();

    ~Network();

    void WrapLayer(std::string layer_name, std::vector<cv::Mat>& channel_images);

private:
    // evalutates true after this classes initialize method was called
    bool m_initialized;
    // evaluates true after the caffe model is initialized (during this classes initialization)
    bool m_model_initialized;
    caffe::shared_ptr<caffe::Net<dtype> > net_;

    // holds all the streams for pushing the data
    std::vector<cudaStream_t> streams;

    cv::Mat m_dark;
    cv::Mat m_white;


};


void test_save_layer(std::string output_folder, std::vector<cv::Mat>& layer_images,  std::string prefix, bool as_rgb=false, unsigned scale_factor=1);


#endif
