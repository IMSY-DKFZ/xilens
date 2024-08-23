/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include "stdafx.h"
#include <m3api/xiApi.h>

#define HandleResult(res, place)                                                                                       \
    if (res != XI_OK)                                                                                                  \
    {                                                                                                                  \
        printf("Error after %s (%d)\n", place, res);                                                                   \
        goto finish;                                                                                                   \
    }

int main(int argc, char *argv[])
{
    void *xiH = NULL;
    char *manifestContent = NULL;
    int MANIFEST_MAX_SIZE = 2 * 1024 * 1024;
    char filename[100] = "sens_calib.dat";
    int MAX_CALIBRATION_FILE_SIZE = 1000 * 1000; // 1MB
    char *fileCalibrationContent = NULL;
    fileCalibrationContent = (char *)calloc(1, MAX_CALIBRATION_FILE_SIZE);
    manifestContent = (char *)malloc(MANIFEST_MAX_SIZE);
    if (manifestContent == NULL || fileCalibrationContent == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for manifest_data\n");
        return EXIT_FAILURE;
    }
    FILE *fileManifest = fopen("manifestData.xml", "w");
    FILE *fileCalibration = fopen("manifestDataCalibration.xml", "w");

    printf("Opening first camera...\n");
    auto stat = xiOpenDevice(0, &xiH);
    HandleResult(stat, "xiOpenDevice");

    stat = xiGetParamString(xiH, XI_PRM_DEVICE_MANIFEST, manifestContent, MANIFEST_MAX_SIZE);
    HandleResult(stat, "xiGetParamString");

    stat = xiSetParamString(xiH, XI_PRM_FFS_FILE_NAME, filename, sizeof(filename));
    HandleResult(stat, "xiSetParamString");

    stat = xiGetParamString(xiH, XI_PRM_READ_FILE_FFS, fileCalibrationContent, MAX_CALIBRATION_FILE_SIZE);
    HandleResult(stat, "xiGetParamString (XI_PRM_READ_FILE_FFS)");

    if (fileManifest == NULL || fileCalibration == NULL)
    {
        fprintf(stderr, "Failed to open file for writing\n");
        goto finish;
    }
    if (fprintf(fileManifest, "%s", manifestContent) < 0)
    {
        fprintf(stderr, "Failed to write manifest data to file\n");
    }
    else
    {
        printf("Manifest data written to manifestData.xml successfully\n");
    }
    if (fprintf(fileCalibration, "%s", fileCalibrationContent) < 0)
    {
        fprintf(stderr, "Failed to write manifest data to file\n");
    }
    else
    {
        printf("Manifest data written to manifestDataCalibration.xml successfully\n");
    }

    fclose(fileManifest);
    fclose(fileCalibration);

finish:
    if (xiH)
        xiCloseDevice(xiH);
    printf("Camera closed...\n");
    free(manifestContent);
    free(fileCalibrationContent);
    return 0;
}
