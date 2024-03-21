#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>
#include <opencv2/opencv.hpp>

// opencv window display image resolution
const int MAX_WIDTH_DISPLAY_WINDOW = 1024;
const int MAX_HEIGHT_DISPLAY_WINDOW = 544;

// camera types and mapper
const QString SPECTRAL_CAMERA = "spectral";
const QString GRAY_CAMERA = "gray";
extern const QMap<QString, QMap<QString, QString>> CAMERA_MAPPER;

// opencv window names
const std::string DISPLAY_WINDOW_NAME = "RAW image";
const std::string VHB_WINDOW_NAME = "Blood volume fraction";
const std::string SAO2_WINDOW_NAME = "Oxygenation";
const std::string BGR_WINDOW_NAME = "RGB estimate";

// saturation and dark pixel colors to show on displays
const cv::Vec3b SATURATION_COLOR = cv::Vec3b(180, 105, 255);
const cv::Vec3b DARK_COLOR = cv::Vec3b(0, 0, 255);

// log file names
const QString LOG_FILE_NAME = "logFile.txt";
const QString TEMP_LOG_FILE_NAME = "temperatureLogFile.txt";

// camera temperature locations
const QString CHIP_TEMP = "chip";
const QString HOUSE_TEMP = "house";
const QString HOUSE_BACK_TEMP = "house_back_side";
const QString SENSOR_BOARD_TEMP = "sensor_board";
const int TEMP_LOG_INTERVAL = 5;

// GUI item colors
const QString BUTTON_PRESSED_STYLE = "QLineEdit {background-color: rgb(255, 0, 0);}";
const QString FIELD_ORIGINAL_STYLE = "QLineEdit {background-color: rgba(35, 38, 41, 0.75);}";
const QString FIELD_EDITED_STYLE = "QLineEdit {background-color: rgba(117, 52, 134, 1);}";

// camera frame rate limit
const int FRAMERATE_MAX = 30;

// value ranges for blood volume fraction and oxygenation
const unsigned MAX_SAO2 = 100;
const unsigned MAX_VHB = 30;
const unsigned MIN_SAO2 = 0;
const unsigned MIN_VHB = 0;

// boundaries for under-saturation and over-saturation pixel color assignments
const int OVEREXPOSURE_PIXEL_BOUNDARY_VALUE = 225;
const int UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE = 10;

// camera types and families
const QString CAMERA_TYPE_KEY_NAME = "cameraType";
const QString CAMERA_TYPE_SPECTRAL = "spectral";
const QString CAMERA_TYPE_GRAY = "gray";
const QString CAMERA_TYPE_RGB = "rgb";
const QString CAMERA_FAMILY_KEY_NAME = "cameraFamily";
const QString CAMERA_FAMILY_XISPEC = "xiSpec";
const QString CAMERA_FAMILY_XIC = "xiC";

// Image recording constants
const int NR_REFERENCE_IMAGES_TO_RECORD = 100;

#endif