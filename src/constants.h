/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVariant>
#include <opencv2/opencv.hpp>

/**
 *  \name OPENCV Display Image Resolution
 * @{
 */
const int MAX_WIDTH_DISPLAY_WINDOW = 1024;
const int MAX_HEIGHT_DISPLAY_WINDOW = 544;
///@}

/**
 *  \name Color Definitions for Saturation and Dark Pixels
 * @{
 */
const cv::Vec3b SATURATION_COLOR = cv::Vec3b(180, 105, 255);
const cv::Vec3b DARK_COLOR = cv::Vec3b(0, 0, 255);
///@}

/**
 *  \name Log File Names
 * @{
 */
const QString LOG_FILE_NAME = "logFile.txt";
///@}

/**
 *  \name Camera Temperature Locations
 * @{
 */
const QString CHIP_TEMP = "temperature_chip";
const QString HOUSE_TEMP = "temperature_house";
const QString HOUSE_BACK_TEMP = "temperature_house_back_side";
const QString SENSOR_BOARD_TEMP = "temperature_sensor_board";
const int TEMP_LOG_INTERVAL = 5;
///@}

/**
 *  \name GUI Item Colors
 * @{
 */
const QString FIELD_ORIGINAL_STYLE = "QLineEdit {background-color: rgba(35, 38, 41, 0.75);}";
const QString FIELD_EDITED_STYLE = "QLineEdit {background-color: rgba(117, 52, 134, 1);}";
///@}

/**
 *  \name Camera Frame Rate Limit
 * @{
 */
const int FRAMERATE_MAX = 80;
///@}

/**
 *  \name Exposure Boundaries
 *  These constants are used for under-saturation and over-saturation pixel
 * color assignments.
 * @{
 */
/// Over-exposure pixel boundary value.
const int OVEREXPOSURE_PIXEL_BOUNDARY_VALUE = 225;
/// Under-exposure pixel boundary value.
const int UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE = 10;
///@}

/**
 *  \name Camera Variables
 *  These variables are used for camera types and families.
 * @{
 */
/// Camera type: Spectral.
const QString CAMERA_TYPE_SPECTRAL = "spectral";
/// Camera type: Gray.
const QString CAMERA_TYPE_GRAY = "gray";
/// Camera type: RGB.
const QString CAMERA_TYPE_RGB = "rgb";
/// Camera family: Xispec.
const QString CAMERA_FAMILY_XISPEC = "xiSpec";
/// Camera family: Xic.
const QString CAMERA_FAMILY_XIC = "xiC";
/// Camera family: XiQ.
const QString CAMERA_FAMILY_XIQ = "xiQ";
/// Camera family: xiB.
const QString CAMERA_FAMILY_XIB = "xiB";
/// Camera family: xiB-64.
const QString CAMERA_FAMILY_XIB64 = "xiB-64";
/// Camera family: xiRAY.
const QString CAMERA_FAMILY_XIRAY = "xiRAY";
/// Camera family: xiX.
const QString CAMERA_FAMILY_XIX = "xiX";
///@}

/**
 * Supported camera types
 */
const std::vector<QString> SUPPORTED_CAMERA_TYPES = {CAMERA_TYPE_SPECTRAL, CAMERA_TYPE_GRAY, CAMERA_TYPE_RGB};

/**
 * Supported camera families
 */
const std::vector<QString> SUPPORTED_CAMERA_FAMILIES = {CAMERA_FAMILY_XISPEC, CAMERA_FAMILY_XIC,   CAMERA_FAMILY_XIQ,
                                                        CAMERA_FAMILY_XIB,    CAMERA_FAMILY_XIB64, CAMERA_FAMILY_XIRAY,
                                                        CAMERA_FAMILY_XIX};

/**
 *  \name Image Recording Constants
 *  These constants are used for image recording configurations.
 * @{
 */
/// Number of reference images to record.
const int NR_REFERENCE_IMAGES_TO_RECORD = 100;
///@}

/**
 * Structure to hold camera meta information such as type, family and mosaic
 * shape.
 */
struct CameraData
{
    QString cameraType;
    QString cameraFamily;
    std::vector<int> mosaicShape;
    std::vector<int> bgrChannels;

    static CameraData fromJson(const QJsonObject &jsonObject)
    {
        CameraData data;
        data.cameraType = jsonObject.value("cameraType").toString();
        data.cameraFamily = jsonObject.value("cameraFamily").toString();
        data.mosaicShape.push_back(jsonObject.value("mosaicWidth").toInt());
        data.mosaicShape.push_back(jsonObject.value("mosaicHeight").toInt());
        if (jsonObject.contains("bgrChannels") && jsonObject["bgrChannels"].isArray())
        {
            QJsonArray array = jsonObject["bgrChannels"].toArray();
            for (const auto &entry : array)
            {
                data.bgrChannels.push_back(entry.toInt());
            }
        }
        else
        {
            data.bgrChannels = std::vector<int>(); // Optional, but explicitly sets it as empty
        }
        return data;
    }
};

/**
 * Loads a camera mapper configuration from a JSON file.
 *
 * This function reads a JSON configuration file specified by the fileName
 * and initializes a camera mapper based on the data within the file. The
 * JSON file must adhere to the expected schema for this operation to
 * succeed. The camera mapper configuration typically involves parameters
 * such as camera mosaic shape, camera family, etc.
 *
 * @param fileName The path to the JSON file containing the camera mapper configuration.
 *
 * @return Returns true if the camera mapper configuration was successfully loaded,
 *         false otherwise.
 */
QMap<QString, CameraData> loadCameraMapperFromJson(const QString &fileName);

/**
 * Checks that the camera type and family are supported by the application
 *
 * @param type The type of camera, e.g. spectral, rgb or gray.
 * @param family The camera family, e.g. xiQ, xiC or xiU.
 * @return true if both camera type and family are supported
 */
bool isCameraSupported(const QString &type, const QString &family);

/**
 * Loads the camera mapper on first call and returns it.
 * This mapper is represented as a constant map with camera models as keys and
 * camera types as values.
 *
 * @return A mapper that maps camera models to their corresponding type and
 * family, e.g. (spectral, xiSpec), (gray, xiC), etc.
 */
QMap<QString, CameraData> &getCameraMapper();

/**
 * name of key to be used to store exposure time in the metadata of the arrays.
 */
constexpr const char *EXPOSURE_KEY = "exposure_us";

/**
 * name of key to be used to store frame number in the metadata of the arrays.
 */
constexpr const char *FRAME_NUMBER_KEY = "acq_nframe";

/**
 * name of key to be used to store filter array format in the metadata of the arrays.
 */
constexpr const char *COLOR_FILTER_ARRAY_FORMAT_KEY = "color_filter_array";

/**
 * name of key to be used to store time stamp in the metadata of the arrays.
 */
constexpr const char *TIME_STAMP_KEY = "time_stamp";

/**
 * Maximum number of frames used to compute the frames per second at which recordings happen.
 */
const int MAX_FRAMES_TO_COMPUTE_FPS = 10;

/**
 * Rate in milliseconds at which the frames per second display in the UI is updated.
 */
const int UPDATE_RATE_MS_FPS_TIMER = 2000;

#endif
