/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef CONSTANTS_H
#define CONSTANTS_H

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
 *  \name OpenCV Window Names
 * @{
 */
const std::string DISPLAY_WINDOW_NAME = "RAW image";
const std::string VHB_WINDOW_NAME = "Blood volume fraction";
const std::string SAO2_WINDOW_NAME = "Oxygenation";
const std::string BGR_WINDOW_NAME = "RGB estimate";
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
const QString TEMP_LOG_FILE_NAME = "temperatureLogFile.txt";
///@}

/**
 *  \name Camera Temperature Locations
 * @{
 */
const QString CHIP_TEMP = "chip";
const QString HOUSE_TEMP = "house";
const QString HOUSE_BACK_TEMP = "house_back_side";
const QString SENSOR_BOARD_TEMP = "sensor_board";
const int TEMP_LOG_INTERVAL = 5;
///@}

/**
 *  \name GUI Item Colors
 * @{
 */
const QString FIELD_ORIGINAL_STYLE =
    "QLineEdit {background-color: rgba(35, 38, 41, 0.75);}";
const QString FIELD_EDITED_STYLE =
    "QLineEdit {background-color: rgba(117, 52, 134, 1);}";
///@}

/**
 *  \name Camera Frame Rate Limit
 * @{
 */
const int FRAMERATE_MAX = 80;
///@}

/**
 *  \name Value Ranges for Blood Volume Fraction and Oxygenation
 * @{
 */
const unsigned MAX_SAO2 = 100;
const unsigned MAX_VHB = 30;
const unsigned MIN_SAO2 = 0;
const unsigned MIN_VHB = 0;
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
///@}

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
struct CameraData {
  QString cameraType;
  QString cameraFamily;
  std::vector<int> mosaicShape;
};

/**
 * @brief A mapper that maps camera models to their corresponding type and
 * family, e.g. (spectral, xiSpec), (gray, xiC), etc.
 *
 * This mapper is represented as a constant map with camera models as keys and
 * camera types as values.
 */
const QMap<QString, CameraData> CAMERA_MAPPER = {
    {"MQ022HG-IM-SM4X4-VIS",
     CameraData{CAMERA_TYPE_SPECTRAL, CAMERA_FAMILY_XISPEC, {4, 4}}},
    {"MQ022HG-IM-SM4X4-VIS3",
     CameraData{CAMERA_TYPE_SPECTRAL, CAMERA_FAMILY_XISPEC, {4, 4}}},
    {"MC050MG-SY-UB", CameraData{CAMERA_TYPE_GRAY, CAMERA_FAMILY_XIC, {0, 0}}},
    {"MQ042CG-CM", CameraData{CAMERA_TYPE_RGB, CAMERA_FAMILY_XIQ, {0, 0}}}};

#endif
