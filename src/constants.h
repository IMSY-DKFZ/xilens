#ifndef CONSTANTS_H
#define CONSTANTS_H


// opencv window display image resolution
const int MAX_WIDTH_DISPLAY_WINDOW = 1024;
const int MAX_HEIGHT_DISPLAY_WINDOW = 544;

// camera types and mapper
const QString SPECTRAL_CAMERA = "spectral";
const QString GRAY_CAMERA = "gray";
extern const QMap<QString, QString> CAMERA_TYPE_MAPPER;

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
const QString BUTTON_PRESSED_STYLE = "background-color: rgb(255, 0, 0)";
const QString FIELD_ORIGINAL_STYLE = "background-color: rgb(255, 255, 255)";
const QString FIELD_EDITED_STYLE = "background-color: rgb(255, 105, 180)";

// camera frame rate limit
const int FRAMERATE_MAX = 30;

// low exposure recordings integration times
const std::vector<int> LOW_EXPOSURE_INTEGRATION_TIMES = {5, 10, 20, 40, 60, 80, 100, 150};

// value ranges for blood volume fraction and oxygenation
const unsigned MAX_SAO2 = 100;
const unsigned MAX_VHB = 30;
const unsigned MIN_SAO2 = 0;
const unsigned MIN_VHB = 0;

#endif