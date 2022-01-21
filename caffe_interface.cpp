

#include <caffe_interface.h>

#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#if __has_include(<opencv2/contrib/contrib.hpp>)
#  include <opencv2/contrib/contrib.hpp>
#else
#  define opencv2_has_contrib 0
#endif


#include <boost/log/trivial.hpp>

#include <algorithm>
#include <iosfwd>
#include <utility>
#include <iostream>
#include <stdexcept>
#include "util.h"
#include "time.h"

using namespace caffe;  // NOLINT(build/namespaces)
using std::string;

#define NR_INPUT_IMAGES (3)


void WrapFloatpInMat(float* float_p, cv::Mat& retImg, unsigned height, unsigned width)
{
    cv::Mat channel(height, width, CV_32FC1, float_p);
    retImg = channel;
}


inline bool file_exists (const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}


Network::Network(const string& network_spec_file,
                 const string& model_file)
{
    this->Intialize(network_spec_file, model_file);
}


Network::Network() :
    m_initialized(false), m_model_initialized(false)
{
}


bool Network::NetworkReady()
{
    return this->m_initialized;
}


void Network::Intialize(const string& network_spec_file,
                 const string& model_file)
{
    if (!file_exists(network_spec_file))
        throw std::runtime_error("network specification file " + network_spec_file + " not found");
    if (!file_exists(model_file))
        throw std::runtime_error("model file " + model_file + " not found");

#ifdef CPU_ONLY
    Caffe::set_mode(Caffe::CPU);
#else
    Caffe::set_mode(Caffe::GPU);
#endif

    /* Load the network. */
    net_.reset(new Net<dtype>(network_spec_file, TEST));
    net_->CopyTrainedLayersFrom(model_file);
    m_model_initialized = true;

    CHECK_EQ(net_->num_inputs(), NR_INPUT_IMAGES) << "Network should have exactly NR_INPUT_IMAGES inputs.";
    //CHECK_EQ(net_->num_outputs(), 17) << "Network should have 17 outputs.";

    // TODO: here a check for equal size of the input layers would be nice

    // set white and dark to standard values
    // white: ones
    BOOST_LOG_TRIVIAL(info) << "initialize white and dark for geometry: " << this->GetGeometry() << "\n";
    m_white = cv::Mat::ones(this->GetGeometry(), CV_32FC1);
    // dark: zeros
    m_dark = cv::Mat::zeros(this->GetGeometry(), CV_32FC1);


    // create streams for pushing the data
    for (int i=0; i < NR_INPUT_IMAGES; i++)
    {
        cudaStream_t stream_i;
        CUDA_CHECK(cudaStreamCreate(&stream_i));
        streams.push_back(stream_i);
    }
    m_initialized = true;
    LOG(INFO) << "network initialization successful";
}


void Network::SetImage(float* img, enum InputImage i)
{
    if (NetworkReady())
    {
        cv::Mat temp;
        WrapFloatpInMat(img, temp, GetGeometry().height, GetGeometry().width);
        if (i == Network::DARK)
        {
            temp.copyTo(m_dark);
            temp = -temp;
        }
        if (i == Network::WHITE)
        {
            temp.copyTo(m_white);
            temp = 1. / (m_white - m_dark);
        }

        // first get input blob:
        Blob<dtype>* input_i = net_->input_blobs()[i];
        // set it to image data pointer
        input_i->set_cpu_data((float*) temp.data);
        // spawn a stream to gpu for each input
        // attention: this method was forwarded in caffe blobs by ourselves
        // so it is a potential error source.
        input_i->async_gpu_push(streams.at(i));
    }
    else
    {
    }
}

void Network::SetImage(cv::Mat& img, enum InputImage i)
{
    cv::Mat floatImg;
    img.convertTo(floatImg, CV_32FC1);
    if (NetworkReady())
    {
        this->SetImage(reinterpret_cast<float*>(floatImg.data), i);
    }
    else
    {
    }
}


void Network::SetImage(std::string image_file,  enum InputImage i)
{
    if (NetworkReady())
    {
        // read image and convert to float
        cv::Mat img = cv::imread(image_file,  CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);
        this->SetImage(img, i);
    }
    else
    {
    }
}


void Network::SetBinaryImage(std::string image_file,  enum InputImage i)
{
    if (NetworkReady())
    {
        std::ifstream infile;
        infile.open(image_file.c_str(), std::ios::binary | std::ios::in);
        cv::Size geometry = this->GetGeometry();

        // create the image to be read
        cv::Mat img_float(geometry, CV_32FC1);
        // reads image from .dat file
        infile.read((char*)img_float.data, geometry.area() * sizeof(CV_32FC1));

        this->SetImage(img_float, i);
    }
    else
    {
    }
}

void Network::RunNewImage(float* img)
{
    if (NetworkReady())
    {
        this->SetImage(img, Network::IMAGE);
        this->Run();
    }
    else
    {
    }
}


void Network::RunNewImage(std::string image_file)
{
    if (NetworkReady())
    {
        this->SetImage(image_file, Network::IMAGE);
        this->Run();
    }
    else
    {
    }
}


void Network::SetWhite(std::string image_file)
{ //
    if (NetworkReady() && file_exists(image_file))
    {
       this->SetImage(image_file, Network::WHITE);
    }
    else
    {
        throw std::runtime_error("network not initialized or white balance file does not exist");
    }
}



void Network::SetDark(std::string image_file)
{
    if (NetworkReady() && file_exists(image_file))
    {
        this->SetImage(image_file, Network::DARK);
    }
    else
    {
        throw std::runtime_error("network not initialized or dark file does not exist");
    }
}


void Network::Run()
{
    if (NetworkReady())
    {
        //struct timespec start, finish;
        //double elapsed;
        //clock_gettime(CLOCK_MONOTONIC, &start);

        // synchronize the streams
        CUDA_CHECK(cudaDeviceSynchronize());
        net_->Forward();

        //clock_gettime(CLOCK_MONOTONIC, &finish);
        //elapsed = (finish.tv_sec - start.tv_sec);
        //elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

        //std::cout << "net forward took only:: " << elapsed << "s, wowwwww!!!\n"
    }
    else
    {
    }

}



void Network::WrapLayer(std::string layer_name, std::vector<cv::Mat>& channel_images)
{
    if (NetworkReady())
    {
        channel_images.clear();
        shared_ptr<Blob<dtype> > blob = net_->blob_by_name(layer_name);

        int width = blob->width();
        int height = blob->height();
        dtype* output_data = blob->mutable_cpu_data();

        for (int i = 0; i < blob->channels(); ++i)
        {
            cv::Mat channel_i;
            WrapFloatpInMat(output_data, channel_i, height, width);
            channel_images.push_back(channel_i);
            output_data += width * height;
        }
    }
    else
    {
    }
}



void Network::GetBand(std::vector<cv::Mat>& band_image, unsigned band_nr)
{
    if (NetworkReady())
    {
        std::string band_identifier = "band" + boost::lexical_cast<std::string>(band_nr);
        this->WrapLayer(band_identifier,  band_image);
    }
    else
    {
        throw std::runtime_error("network not initialized, cannot return band image");
    }

}

void Network::GetBands(cv::Mat& rgb_image, std::vector<unsigned>& bands)
{
    if (NetworkReady())
    {
        std::vector<cv::Mat> channels;
        std::vector<cv::Mat> channel;
        bands = {3, 15, 11};
        for (std::size_t i = 0; i < bands.size(); ++i)
        {
            this ->GetBand(channel, bands.at(0));
            channels.push_back(channel.at(0));
        }
        cv::merge(channels, rgb_image);
    }
    else
    {
        throw std::runtime_error("network not initialized, cannot return band image");
    }
}



void Network::GetBGR(std::vector<cv::Mat>& bgr)
{
    if (NetworkReady())
    {
        this->WrapLayer("bgr",  bgr);
    }
    else
    {
        throw std::runtime_error("network not initialized, cannot return bgr image");
    }
}

void Network::GetPhysiologicalParameters(std::vector<cv::Mat>& physParam)
{
    if (NetworkReady())
    {
        this->WrapLayer("physiologicalParameters",  physParam);
    }
    else
    {
        throw std::runtime_error("network not initialized, cannot return physiological parameters");
    }
}


cv::Size Network::GetGeometry()
{
    if (m_model_initialized)
    {
        Blob<dtype>* input_layer = this->net_->input_blobs()[0];
        return cv::Size(input_layer->width(), input_layer->height());
    }
    else
    {
        throw std::runtime_error("network not initialized, cannot return geometry");
    }
}


Network::~Network()
{
    // destroy streams for pushing the data
    for (std::string::size_type i=0; i < streams.size(); i++)
    {
        CUDA_CHECK(cudaStreamDestroy(streams.at(i)));
    }

}



void test_save_layer(std::string output_folder, std::vector<cv::Mat>& layer_images,  std::string prefix, bool as_rgb, unsigned scale_factor)
{

    if (!as_rgb)
    {
        for (std::string::size_type i=0; i < layer_images.size(); i++)
        {
            stringstream ss;
            ss << output_folder << i << prefix << ".tif";
            cv::Mat img_uint8;
            cv::Mat im_i = layer_images.at(i);
            im_i *= scale_factor; // deprecated, now the network outputs percent directly
            im_i.convertTo(img_uint8, CV_8U);
            cv::imwrite(ss.str(), im_i);
        }
    }
    else
    {
        assert(layer_images.size()==3);
        cv::Mat bgr_composed = cv::Mat::zeros(layer_images.at(0).size(), CV_32FC3);
        cv::merge(layer_images, bgr_composed);
        cv::threshold(bgr_composed, bgr_composed, 1., 1., cv::THRESH_TRUNC);
        bgr_composed *=255;

        stringstream ss;
        ss << output_folder << prefix << ".tif";
        //cv::imshow("test", bgr_composed);
        //cv::waitKey(0);
        cv::imwrite(ss.str(), bgr_composed);
    }
}
