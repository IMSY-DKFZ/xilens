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


#include <caffe_interface.h>
#include <util.h>
#include <boost/filesystem.hpp>

#ifdef USE_OPENCV

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#if __has_include(<opencv2/contrib/contrib.hpp>)
#else
#  define opencv2_has_contrib 0
#endif


using namespace std;


std::ofstream out_gpu_push("timings_gpu_push.bin",std::ios_base::binary);
std::ofstream out_net_run("timings_net_run.bin",std::ios_base::binary);




/**
  test if a string has a certain ending. Taken from here:
  http://stackoverflow.com/questions/874134/find-if-string-ends-with-another-string-in-c

 * @brief hasEnding
 * @param fullString
 * @param ending
 * @return
 */
bool hasEnding (std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

void save_time(std::ofstream& bin_file, float time)
{
    if(bin_file.good())
    {
        bin_file.write((char *)&time,sizeof(float));
    }
}

void RunNetForFile(Network& net, std::string file)
{
    struct timespec start, finish;
    double elapsed;
    if (boost::filesystem::is_regular_file(file))
    {
        if (hasEnding(file, std::string(".tif")));
        {
            BOOST_LOG_TRIVIAL(info) << "doing regression for " << file;

            // read image from file system
            cv::Mat img = cv::imread(file,  CV_LOAD_IMAGE_ANYDEPTH | CV_LOAD_IMAGE_ANYCOLOR);

            // convert to float and set into network (upload data to GPU)
            clock_gettime(CLOCK_MONOTONIC, &start);
            net.SetImage(reinterpret_cast<float*>(img.data), Network::IMAGE);
            clock_gettime(CLOCK_MONOTONIC, &finish);
            elapsed = (finish.tv_sec - start.tv_sec);
            elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000.0;
            save_time(out_gpu_push, elapsed);
            BOOST_LOG_TRIVIAL(info) <<  "time for pushing data to GPU:: " << elapsed << "ms\n" << std::flush;

            // run the network
            clock_gettime(CLOCK_MONOTONIC, &start);
            net.Run();
            clock_gettime(CLOCK_MONOTONIC, &finish);
            elapsed = (finish.tv_sec - start.tv_sec);
            elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000.0;
            save_time(out_net_run, elapsed);
            BOOST_LOG_TRIVIAL(info) <<  "time for running network:: " << elapsed << "ms\n" << std::flush;


        }
    }
}


/**
 * @brief SaveResults saves results for file of the network evaluation
 * @param net the network which ran on file
 * @param file the input filename (without path), will be used to generate an output filename prefix
 */
void SaveResults(Network& net, std::string folder, std::string file)
{
    // display output here
    // first wrap output in cv::Mat s
    std::vector<cv::Mat> layer_images;
    net.GetPhysiologicalParameters(layer_images);
    test_save_layer(folder, layer_images, "_physiological_parameter_" + file );
    net.GetBGR(layer_images);
    test_save_layer(folder, layer_images, "bgr_" + file + "_", true);
}



int main(int argc, char** argv)
{
    if (argc != 7)
    {
        std::cerr << "Usage: " << argv[0]
                  << " deploy.prototxt network.caffemodel"
                  << " white_file dark_file output_folder img.tif_or_folder"
                  << std::endl;
        return 1;
    }

    initLogging(boost::log::trivial::info);

    string model_file = argv[1];
    string trained_file = argv[2];
    string white_file = argv[3];
    string dark_file = argv[4];
    string output_folder = argv[5];
    string file_or_folder = argv[6];


    BOOST_LOG_TRIVIAL(info) << "Loading caffe model...\n" << std::flush;

    Network network(model_file, trained_file);

    BOOST_LOG_TRIVIAL(info) << "Succcessfully initialized network, now loading white and dark...\n" << std::flush;

    network.SetDark(dark_file);
    network.SetWhite(white_file);

    BOOST_LOG_TRIVIAL(info) <<  "Successfully set white and dark files. Now run net once to acquire memory...\n" << std::flush;

    // once run net to acquire memory
    network.Run();

    BOOST_LOG_TRIVIAL(info) <<  "Ran net once to acquire memory. Now run on the input image...\n" << std::flush;

    // next loop inspired by:
    // http://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
    // now iterate over each image file in folder or just the input file
    if ( boost::filesystem::is_directory(file_or_folder) )
    { // if its a directory: run (non-iteratively) over all files
        boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
        for ( boost::filesystem::directory_iterator itr( file_or_folder );
              itr != end_itr;
              ++itr )
        {
            boost::filesystem::path found_file = itr->path();
            RunNetForFile(network, found_file.string());
            SaveResults(network, output_folder, found_file.filename().string());
        }
    } // if its a file, just run it for this
    else if (boost::filesystem::is_regular_file(file_or_folder))
    {
        RunNetForFile(network, file_or_folder);
        SaveResults(network, output_folder, boost::filesystem::path(file_or_folder).filename().string());
    }

    // TODO not nice, use RAII for file management
    out_gpu_push.close();
    out_net_run.close();

}

#else
int
main(int argc, char** argv)
{
    BOOST_LOG_TRIVIAL(fatal) <<  "This program requires OpenCV; compile with USE_OPENCV.";
}
#endif  // USE_OPENCV
