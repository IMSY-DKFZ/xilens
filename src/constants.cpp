/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include "constants.h"
#include "logger.h"

bool isCameraSupported(const QString &type, const QString &family)
{
    return (std::find(SUPPORTED_CAMERA_TYPES.begin(), SUPPORTED_CAMERA_TYPES.end(), type) !=
            SUPPORTED_CAMERA_TYPES.end()) &&
           (std::find(SUPPORTED_CAMERA_FAMILIES.begin(), SUPPORTED_CAMERA_FAMILIES.end(), family) !=
            SUPPORTED_CAMERA_FAMILIES.end());
}

QMap<QString, CameraData> loadCameraMapperFromJson(const QString &fileName)
{
    QDir dir;
    auto appDir = QCoreApplication::applicationDirPath();
    if ((appDir == QDir::cleanPath(QDir::fromNativeSeparators("/usr/local/bin"))) ||
        (appDir == QDir::cleanPath(QDir::fromNativeSeparators("/usr/bin"))))
    {
        dir.setPath(QDir::fromNativeSeparators("/etc/xilens"));
    }
    else
    {
        dir.setPath(QDir::currentPath());
    }

    QFile file(dir.filePath(fileName));
    if (!file.exists())
    {
        LOG_XILENS(error) << "File does not exist: " << file.fileName().toStdString();
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG_XILENS(error) << "Cannot open file";
        throw std::runtime_error("Cannot open file");
    }
    LOG_XILENS(info) << "loading camera properties from: " << QFileInfo(file).absoluteFilePath().toStdString();
    QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (document.isNull())
    {
        LOG_XILENS(error) << "INVALID JSON format";
        throw std::runtime_error("Invalid JSON format");
    }

    QMap<QString, CameraData> cameraMapper;
    QJsonObject jsonObject = document.object();

    for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
    {
        CameraData cameraData = CameraData::fromJson(it.value().toObject());
        if (!isCameraSupported(cameraData.cameraType, cameraData.cameraFamily))
        {
            LOG_XILENS(error) << "Unsupported camera - Type: " << cameraData.cameraType.toStdString()
                              << ", Family: " << cameraData.cameraFamily.toStdString();
            throw std::runtime_error("Unsupported camera type or family");
        }
        cameraMapper.insert(it.key(), cameraData);
    }

    return cameraMapper;
}

QMap<QString, CameraData> &getCameraMapper()
{
    static QMap<QString, CameraData> cameraMapper = loadCameraMapperFromJson("XiLensCameraProperties.json");
    return cameraMapper;
}
