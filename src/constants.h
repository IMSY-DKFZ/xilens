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
 * @brief Maximum width of image to display.
 */
const int MAX_WIDTH_DISPLAY_WINDOW = 1024;
/**
 * @brief Maximum height of image to display.
 */
const int MAX_HEIGHT_DISPLAY_WINDOW = 544;

/**
 * @brief Color used to represent saturation pixels.
 *
 * This color is represented as a BGR vector with values (180, 105, 255).
 */
const cv::Vec3b SATURATION_COLOR = cv::Vec3b(180, 105, 255);

/**
 * @brief Color used to represent dark pixels.
 *
 * This color is represented as a BGR vector with values (0, 0, 255).
 */
const cv::Vec3b DARK_COLOR = cv::Vec3b(0, 0, 255);

/**
 * @brief File name where logs are stored.
 */
const QString LOG_FILE_NAME = "logFile.txt";

/**
 * @brief Variable used to identify camera chip temperature.
 */
const QString CHIP_TEMP = "temperature_chip";
/**
 * @brief Variable used to identify camera housing temperature.
 */
const QString HOUSE_TEMP = "temperature_house";
/**
 * @brief Variable used to identify camera housing (back) temperature.
 */
const QString HOUSE_BACK_TEMP = "temperature_house_back_side";
/**
 * @brief Variable used to identify camera board temperature.
 */
const QString SENSOR_BOARD_TEMP = "temperature_sensor_board";
/**
 * @brief Variable used to identify how ofter temperature is queried from the camera.
 */
const int TEMP_LOG_INTERVAL = 5;

/**
 * @brief Original style of input component.
 */
const QString FIELD_ORIGINAL_STYLE = "QLineEdit {background-color: rgba(35, 38, 41, 0.75);}";
/**
 * @brief Style of input component when edited.
 */
const QString FIELD_EDITED_STYLE = "QLineEdit {background-color: rgba(117, 52, 134, 1);}";

/**
 * @brief Maximum framerate at which images are polled from camera.
 */
const int FRAMERATE_MAX = 80;

/**
 * @brief maximum value in range [0, 255] above which pixels are considered over-saturated.
 *
 * The maximum value is `225` in the range `[0,255]`, which corresponds to `900` in the range `[0, 1024]`.
 */
const int OVEREXPOSURE_PIXEL_BOUNDARY_VALUE = 225;
/**
 * @brief minimum value in range [0, 255] below which pixels are considered under-saturated.
 *
 * The minimum value is `10` in the range `[0,255]`, which corresponds to `40` in the range `[0, 1024]`.
 */
const int UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE = 10;

/**
 * @brief Name of spectral camera type.
 */
const QString CAMERA_TYPE_SPECTRAL = "spectral";
/**
 * @brief Name of gray camera type.
 */
const QString CAMERA_TYPE_GRAY = "gray";
/**
 * @brief Name of RGB camera type.
 */
const QString CAMERA_TYPE_RGB = "rgb";
/**
 * @brief Name of xiSpec camera family.
 */
const QString CAMERA_FAMILY_XISPEC = "xiSpec";
/**
 * @brief Name of xiC camera family.
 */
const QString CAMERA_FAMILY_XIC = "xiC";
/**
 * @brief Name of xiQ camera family.
 */
const QString CAMERA_FAMILY_XIQ = "xiQ";
/**
 * @brief Name of xiB camera family.
 */
const QString CAMERA_FAMILY_XIB = "xiB";
/**
 * @brief Name of xiB-64 camera family.
 */
const QString CAMERA_FAMILY_XIB64 = "xiB-64";
/**
 * @brief Name of xiRay camera family.
 */
const QString CAMERA_FAMILY_XIRAY = "xiRAY";
/**
 * @brief Name of xiX camera family.
 */
const QString CAMERA_FAMILY_XIX = "xiX";

/**
 * @brief Vector of supported camera types.
 */
const std::vector<QString> SUPPORTED_CAMERA_TYPES = {CAMERA_TYPE_SPECTRAL, CAMERA_TYPE_GRAY, CAMERA_TYPE_RGB};

/**
 * @brief Vector of supported camera families.
 */
const std::vector<QString> SUPPORTED_CAMERA_FAMILIES = {CAMERA_FAMILY_XISPEC, CAMERA_FAMILY_XIC,   CAMERA_FAMILY_XIQ,
                                                        CAMERA_FAMILY_XIB,    CAMERA_FAMILY_XIB64, CAMERA_FAMILY_XIRAY,
                                                        CAMERA_FAMILY_XIX};

/**
 * @brief Number of images to record for reference images for `white` and `dark`.
 */
const int NR_REFERENCE_IMAGES_TO_RECORD = 100;

/**
 * @brief Structure to hold metadata from camera such as type, family, etc.
 *
 * Structure to hold camera meta information such as type, family and mosaic
 * shape. This can be initialized from a `QJsonObject` using CameraData.fromJson .
 */
struct CameraData
{
    QString cameraType;
    QString cameraFamily;
    std::vector<int> mosaicShape;
    std::vector<int> bgrChannels;

    /**
     * Method to initialize camera metadata object from a QJsonObject.
     *
     * @param jsonObject Object containing camera metadata: `cameraType, cameraFamily, mosaicWidth, mosaicHeight,
     * bgrChannels`.
     * @return structure containing the camera metadata.
     */
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
 * @brief Loads a camera mapper configuration from a JSON file.
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
 * @brief Checks that the camera type and family are supported by the application
 *
 * @param type The type of camera, e.g. spectral, rgb or gray.
 * @param family The camera family, e.g. xiQ, xiC or xiU.
 * @return true if both camera type and family are supported
 */
bool isCameraSupported(const QString &type, const QString &family);

/**
 * @brief Loads the camera mapper on first call and returns it.
 * This mapper is represented as a constant map with camera models as keys and
 * camera types as values.
 *
 * @return A mapper that maps camera models to their corresponding type and
 * family, e.g. (spectral, xiSpec), (gray, xiC), etc.
 */
QMap<QString, CameraData> &getCameraMapper();

/**
 * @brief Name of key to be used to store exposure time in the metadata of the arrays.
 */
constexpr const char *EXPOSURE_KEY = "exposure_us";

/**
 * @brief Name of key to be used to store frame number in the metadata of the arrays.
 */
constexpr const char *FRAME_NUMBER_KEY = "acq_nframe";

/**
 * @brief Name of key to be used to store filter array format in the metadata of the arrays.
 */
constexpr const char *COLOR_FILTER_ARRAY_FORMAT_KEY = "color_filter_array";

/**
 * @brief Name of key to be used to store time stamp in the metadata of the arrays.
 */
constexpr const char *TIME_STAMP_KEY = "time_stamp";

/**
 * @brief Maximum number of frames used to compute the frames per second at which recordings happen.
 */
const int MAX_FRAMES_TO_COMPUTE_FPS = 10;

/**
 * @brief Rate in milliseconds at which the frames per second display in the UI is updated.
 */
const int UPDATE_RATE_MS_FPS_TIMER = 2000;

#endif
