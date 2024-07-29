#include <m3api/xiApi.h>

#include <algorithm>
#include <chrono>
#include <cstring>

#include "stdafx.h"

#define HandleResult(res, place)                 \
  if (res != XI_OK) {                            \
    printf("Error after %s (%d)\n", place, res); \
    goto finish;                                 \
  }

int main(int argc, char* argv[]) {
  void* xiH = NULL;
  XI_RETURN stat;
  const int image_count = 100;
  std::chrono::high_resolution_clock::time_point start;
  std::chrono::high_resolution_clock::time_point end;
  std::chrono::seconds duration;
  XI_IMG image;
  memset(&image, 0, sizeof(image));
  image.size = sizeof(XI_IMG);
  int framerate;

  int current_max_framerate;

  printf("Opening first camera...\n");
  stat = xiOpenDevice(0, &xiH);
  HandleResult(stat, "xiOpenDevice");

  stat =
      xiSetParamInt(xiH, XI_PRM_ACQ_TIMING_MODE, XI_ACQ_TIMING_MODE_FRAME_RATE);
  HandleResult(stat, "xiSetParamInt:XI_PRM_ACQ_TIMING_MODE");
  stat = xiGetParamInt(xiH, XI_PRM_FRAMERATE XI_PRM_INFO_MAX,
                       &current_max_framerate);
  HandleResult(stat, "xiGetParamInt:XI_PRM_FRAMERATE XI_PRM_INFO_MAX");
  stat = xiGetParamInt(xiH, XI_PRM_FRAMERATE, &framerate);
  HandleResult(stat, "xiGetParamInt:XI_PRM_FRAMERATE");
  printf("Current frame rate: %d\n", framerate);
  stat =
      xiSetParamInt(xiH, XI_PRM_FRAMERATE, std::min(80, current_max_framerate));
  HandleResult(stat, "xiSetParamInt:XI_PRM_FRAMERATE");
  stat = xiSetParamInt(xiH, XI_PRM_EXPOSURE, 40000);  // microseconds
  HandleResult(stat, "xiSetParamInt:XI_PRM_EXPOSURE");

  xiStartAcquisition(xiH);

  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < image_count; i++) {
    printf("Getting image #: %d\n", i);
    stat = xiGetImage(xiH, 1000, &image);  // get image inside here
    HandleResult(stat, "xiGetImage");
    printf("Got image #: %d\n", image.acq_nframe);
  }
  end = std::chrono::high_resolution_clock::now();
  duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  printf("Images per second: %.2f\n",
         static_cast<double>(image_count) / duration.count());

  xiStopAcquisition(xiH);
  printf("Stopped acquisition");

finish:
  if (xiH) xiCloseDevice(xiH);
  printf("Camera closed...\n");
  return 0;
}
