#include <algorithm>
#include <m3api/xiApi.h>
#include <opencv2/core.hpp>

#include "stdafx.h"

#define HandleResult(res,place) if (res!=XI_OK) {printf("Error after %s (%d)\n",place,res);goto finish;}

int main(int argc, char* argv[])
{
    void* xiH = NULL;
    XI_RETURN stat;
    XI_IMG image;
    memset(&image, 0, sizeof(image));
    image.size = sizeof(XI_IMG);
    cv::Mat imageMat;

    printf("Opening first camera...\n");
    stat = xiOpenDevice(0, &xiH);
    HandleResult(stat, "xiOpenDevice");

    xiStartAcquisition(xiH);

    printf("Getting image ...\n");

    stat = xiGetImage(xiH, 1000, &image);
    HandleResult(stat, "xiGetImage");

    printf("Getting image finished!\n");

    xiStopAcquisition(xiH);
    printf("Stopped acquisition\n");

    imageMat = cv::Mat(image.height, image.width, CV_16UC1, image.bp);
    printf("Image created before closing device\n");

    xiCloseDevice(xiH);

    imageMat = cv::Mat(image.height, image.width, CV_16UC1, image.bp);
    printf("First value: %d\n", imageMat.at<uint16_t>(0));

finish:
    if (xiH) xiCloseDevice(xiH);
    printf("Camera closed...\n");
    return 0;
}
