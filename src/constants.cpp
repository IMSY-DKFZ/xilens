/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/

#include "constants.h"

QMap<QString, QVariant> createCameraMap(const QString& type,
                                        const QString& family,
                                        const std::vector<int>& size)
{
    QMap<QString, QVariant> map;
    map.insert(CAMERA_TYPE_KEY_NAME, type);
    map.insert(CAMERA_FAMILY_KEY_NAME, family);
    map.insert(CAMERA_MOSAIC_SIZE_NAME, QVariant::fromValue(size));

    return map;
}

// Initialize the camera mapper.
const QMap<QString, QMap<QString, QVariant>> CAMERA_MAPPER = {
    {"MQ022HG-IM-SM4X4-VIS", createCameraMap(CAMERA_TYPE_SPECTRAL, CAMERA_FAMILY_XISPEC, {4, 4})},
    {"MQ022HG-IM-SM4X4-VIS3", createCameraMap(CAMERA_TYPE_SPECTRAL, CAMERA_FAMILY_XISPEC, {4, 4})},
    {"MC050MG-SY-UB", createCameraMap(CAMERA_TYPE_GRAY, CAMERA_FAMILY_XIC, {0, 0})},
    {"MQ042CG-CM", createCameraMap(CAMERA_TYPE_RGB, CAMERA_FAMILY_XIQ, {0, 0})}
};