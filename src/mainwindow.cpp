/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QTextStream>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <string>
#include <utility>

#if CV_VERSION_MAJOR == 3
#include <opencv2/core/types_c.h>
#include <opencv2/imgproc/imgproc_c.h>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio.hpp>
#endif

#include "constants.h"
#include "displayFunctional.h"
#include "imageContainer.h"
#include "logger.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include "xiAPIWrapper.h"

MainWindow::MainWindow(QWidget *parent, std::shared_ptr<XiAPIWrapper> xiAPIWrapper)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_IOService(), m_temperatureIOService(),
      m_temperatureIOWork(new boost::asio::io_service::work(m_temperatureIOService)), m_cameraInterface(),
      m_recordedCount(0), m_testMode(g_commandLineArguments.test_mode), m_imageCounter(0), m_skippedCounter(0),
      m_elapsedTimeTextStream(&m_elapsedTimeText), m_elapsedTime(0)
{
    this->m_xiAPIWrapper = xiAPIWrapper == nullptr ? this->m_xiAPIWrapper : xiAPIWrapper;
    m_cameraInterface.Initialize(this->m_xiAPIWrapper);
    m_imageContainer.Initialize(this->m_xiAPIWrapper);
    ui->setupUi(this);

    // Display needs to be instantiated before changing camera list because
    // calling setCurrentIndex on the list.
    m_display = new DisplayerFunctional(this);

    // populate available cameras
    QStringList cameraList = m_cameraInterface.GetAvailableCameraModels();
    ui->cameraListComboBox->addItem("select camera to enable UI...");
    ui->cameraListComboBox->addItems(cameraList);
    ui->cameraListComboBox->setCurrentIndex(0);

    // set the base folder loc
    m_baseFolderLoc = QDir::cleanPath(QDir::homePath());

    ui->baseFolderLineEdit->insert(this->GetBaseFolder());

    // synchronize slider and exposure checkbox
    QSlider *slider = ui->exposureSlider;
    QLineEdit *expEdit = ui->exposureLineEdit;
    // set default values and ranges
    int slider_min = slider->minimum();
    int slider_max = slider->maximum();
    expEdit->setValidator(new QIntValidator(slider_min, slider_max, this));
    QString initialExpString = QString::number(slider->value());
    expEdit->setText(initialExpString);

    LOG_SUSICAM(info) << "test mode (recording everything to same file) is set to: " << m_testMode << "\n";

    EnableUi(false);
}

void MainWindow::StartImageAcquisition(QString camera_identifier)
{
    try
    {
        this->m_display->StartDisplayer();
        m_cameraInterface.StartAcquisition(std::move(camera_identifier));
        this->StartPollingThread();
        this->StartTemperatureThread();

        // when a new image arrives, display it
        QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display);
    }
    catch (std::runtime_error &error)
    {
        LOG_SUSICAM(warning) << "could not start camera, got error " << error.what();
        throw std::runtime_error(error.what());
    }
}

void MainWindow::StopImageAcquisition()
{
    this->m_display->StopDisplayer();
    this->StopPollingThread();
    this->StopTemperatureThread();
    m_cameraInterface.StopAcquisition();
    // disconnect slots for image display
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display);
    LOG_SUSICAM(info) << "Stopped Image Acquisition";
}

void MainWindow::EnableWidgetsInLayout(QLayout *layout, bool enable)
{
    for (int i = 0; i < layout->count(); ++i)
    {
        QLayout *subLayout = layout->itemAt(i)->layout();
        QWidget *widget = layout->itemAt(i)->widget();
        if (widget)
        {
            widget->setEnabled(enable);
        }
        if (subLayout)
        {
            EnableWidgetsInLayout(subLayout, enable);
        }
    }
}

void MainWindow::EnableUi(bool enable)
{
    QLayout *layout = ui->mainUiVerticalLayout->layout();
    EnableWidgetsInLayout(layout, enable);
    SetGraphicsViewScene();
    this->ui->exposureSlider->setEnabled(enable);
    this->ui->logTextLineEdit->setEnabled(enable);
    QLayout *layoutExtras = ui->extrasVerticalLayout->layout();
    EnableWidgetsInLayout(layoutExtras, enable);
}

void MainWindow::Display()
{
    static boost::posix_time::ptime last = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();

    // display new images with at most every 35ms
    bool display_new = (now - last).total_milliseconds() > 35;

    if (display_new)
    {
        // first get the pointer to the image to display
        XI_IMG image = m_imageContainer.GetCurrentImage();
        this->m_display->Display(image);
        last = now;
    }
}

MainWindow::~MainWindow()
{
    m_IOService.stop();
    m_temperatureIOService.stop();
    m_threadGroup.join_all();

    this->StopTemperatureThread();
    this->StopSnapshotsThread();
    this->StopReferenceRecordingThread();

    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display);

    delete ui;
}

void MainWindow::RecordSnapshots()
{
    static QString recordButtonOriginalColour = ui->recordButton->styleSheet();
    int nr_images = ui->nSnapshotsSpinBox->value();
    QMetaObject::invokeMethod(ui->nSnapshotsSpinBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    QMetaObject::invokeMethod(ui->filePrefixExtrasLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    QMetaObject::invokeMethod(ui->subFolderExtrasLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));

    std::string filePrefix = ui->filePrefixExtrasLineEdit->text().toUtf8().constData();
    std::string subFolder = ui->subFolderExtrasLineEdit->text().toUtf8().constData();

    if (filePrefix.empty())
    {
        filePrefix = m_recPrefixlineEdit.toUtf8().constData();
    }
    if (subFolder.empty())
    {
        subFolder = m_subFolder.toStdString();
    }
    QString filePath = GetFullFilenameStandardFormat(std::move(filePrefix), ".b2nd", std::move(subFolder));
    auto image = m_imageContainer.GetCurrentImage();
    FileImage snapshotsFile(filePath.toStdString().c_str(), image.height, image.width);

    for (int i = 0; i < nr_images; i++)
    {
        int exp_time = m_cameraInterface.m_camera->GetExposureMs();
        int waitTime = 2 * exp_time;
        wait(waitTime);
        image = m_imageContainer.GetCurrentImage();
        snapshotsFile.write(image);
        int progress = static_cast<int>((static_cast<float>(i + 1) / nr_images) * 100);
        QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
    }
    QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
    QMetaObject::invokeMethod(ui->nSnapshotsSpinBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    QMetaObject::invokeMethod(ui->filePrefixExtrasLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    QMetaObject::invokeMethod(ui->subFolderExtrasLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
}

void MainWindow::on_snapshotButton_clicked()
{
    m_snapshotsThread = boost::thread(&MainWindow::RecordSnapshots, this);
}

void MainWindow::LogCameraTemperature()
{
    QString message;
    m_cameraInterface.m_camera->family->get()->UpdateCameraTemperature();
    auto cameraTemperature = m_cameraInterface.m_camera->family->get()->m_cameraTemperature;
    for (const QString &key : cameraTemperature.keys())
    {
        float temp = m_cameraInterface.m_cameraTemperature.value(key);
        message = QString("\t%1\t%2\t%3\t%4")
                      .arg(key)
                      .arg(temp)
                      .arg(this->m_cameraInterface.m_cameraModel)
                      .arg(this->m_cameraInterface.m_cameraSN);
        this->LogMessage(message, TEMP_LOG_FILE_NAME, true);
    }
}

void MainWindow::DisplayCameraTemperature()
{
    double temp = m_cameraInterface.m_camera->family->get()->m_cameraTemperature.value(SENSOR_BOARD_TEMP);
    QMetaObject::invokeMethod(ui->temperatureLCDNumber, "display", Qt::QueuedConnection, Q_ARG(double, temp));
}

void MainWindow::ScheduleTemperatureThread()
{
    m_temperatureIOWork = std::make_unique<boost::asio::io_service::work>(m_temperatureIOService);
    m_temperatureThreadTimer = std::make_shared<boost::asio::steady_timer>(m_temperatureIOService);
    m_temperatureThreadTimer->expires_after(std::chrono::seconds(TEMP_LOG_INTERVAL));
    m_temperatureThreadTimer->async_wait(
        boost::bind(&MainWindow::HandleTemperatureTimer, this, boost::asio::placeholders::error));
}

void MainWindow::HandleTemperatureTimer(const boost::system::error_code &error)
{
    if (error == boost::asio::error::operation_aborted)
    {
        LOG_SUSICAM(warning) << "Timer cancelled. Error: " << error;
        return;
    }

    this->LogCameraTemperature();
    this->DisplayCameraTemperature();

    // Reset timer
    m_temperatureThreadTimer->expires_after(std::chrono::seconds(TEMP_LOG_INTERVAL));
    m_temperatureThreadTimer->async_wait(
        boost::bind(&MainWindow::HandleTemperatureTimer, this, boost::asio::placeholders::error));
}

void MainWindow::StartTemperatureThread()
{
    if (m_temperatureThread.joinable())
    {
        StopTemperatureThread();
    }
    QString log_filename = QDir::cleanPath(ui->baseFolderLineEdit->text() + QDir::separator() + TEMP_LOG_FILE_NAME);
    QFile file(log_filename);
    QFileInfo fileInfo(file);
    if (fileInfo.size() == 0)
    {
        this->LogMessage("time\tsensor_location\ttemperature\tcamera_model\tcamera_sn", TEMP_LOG_FILE_NAME, false);
    }
    file.close();
    m_temperatureThread = boost::thread([&]() {
        ScheduleTemperatureThread();
        m_temperatureIOService.reset();
        m_temperatureIOService.run();
    });
    LOG_SUSICAM(info) << "Started temperature thread";
}

void MainWindow::StopTemperatureThread()
{
    if (m_temperatureThread.joinable())
    {
        if (m_temperatureThreadTimer)
        {
            m_temperatureThreadTimer->cancel();
            m_temperatureThreadTimer = nullptr;
        }
        m_temperatureIOWork.reset();
        m_temperatureThread.join();
        this->ui->temperatureLCDNumber->display(0);
        LOG_SUSICAM(info) << "Stopped temperature thread";
    }
}

void MainWindow::StopSnapshotsThread()
{
    if (m_snapshotsThread.joinable())
    {
        m_snapshotsThread.join();
    }
}

void MainWindow::StopReferenceRecordingThread()
{
    if (m_referenceRecordingThread.joinable())
    {
        m_referenceRecordingThread.join();
    }
}

void MainWindow::on_exposureSlider_valueChanged(int value)
{
    m_cameraInterface.m_camera->SetExposureMs(value);
    UpdateExposure();
}

void MainWindow::UpdateExposure()
{
    int exp_ms = m_cameraInterface.m_camera->GetExposureMs();
    int n_skip_frames = ui->skipFramesSpinBox->value();
    ui->exposureLineEdit->setText(QString::number((int)exp_ms));
    ui->hzLabel->setText(QString::number((double)(1000.0 / (exp_ms * (n_skip_frames + 1))), 'g', 2));

    // need to block the signals to make sure the event is not immediately
    // thrown back to label_exp.
    // could be done with a QSignalBlocker from Qt5.3 on for exception safe
    // treatment. see: http://doc.qt.io/qt-5/qsignalblocker.html
    const QSignalBlocker blocker_slider(ui->exposureSlider);
    ui->exposureSlider->setValue(exp_ms);
}

void MainWindow::on_exposureLineEdit_returnPressed()
{
    m_labelExp = ui->exposureLineEdit->text();
    m_cameraInterface.m_camera->SetExposureMs(m_labelExp.toInt());
    UpdateExposure();
    RestoreLineEditStyle(ui->exposureLineEdit);
}

void MainWindow::on_exposureLineEdit_textEdited(const QString &arg1)
{
    UpdateComponentEditedStyle(ui->exposureLineEdit, arg1, m_labelExp);
}

void MainWindow::on_recordButton_clicked(bool clicked)
{
    static QString original_colour;
    static QString original_button_text;

    if (clicked)
    {
        this->LogMessage(" SUSICAM RECORDING STARTS", LOG_FILE_NAME, true);
        this->LogMessage(QString(" camera selected: %1 %2")
                             .arg(this->m_cameraInterface.m_cameraModel, this->m_cameraInterface.m_cameraSN),
                         LOG_FILE_NAME, true);
        this->m_elapsedTimer.start();
        this->StartRecording();
        this->HandleElementsWhileRecording(clicked);
        original_colour = ui->recordButton->styleSheet();
        original_button_text = ui->recordButton->text();
        // button text seems to be an object property and cannot be changed by using
        // QMetaObject::invokeMethod
        ui->recordButton->setText(" Stop recording");
    }
    else
    {
        this->LogMessage(" SUSICAM RECORDING ENDS", LOG_FILE_NAME, true);
        this->StopRecording();
        this->HandleElementsWhileRecording(clicked);
        ui->recordButton->setText(original_button_text);
    }
}

void MainWindow::HandleElementsWhileRecording(bool recordingInProgress)
{
    if (recordingInProgress)
    {
        QMetaObject::invokeMethod(ui->baseFolderButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->subFolderLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->filePrefixLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->cameraListComboBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->whiteBalanceButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->darkCorrectionButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    }
    else
    {
        QMetaObject::invokeMethod(ui->baseFolderButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->subFolderLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->filePrefixLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->cameraListComboBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->whiteBalanceButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->darkCorrectionButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (this->ui->recordButton->isChecked())
    {
        on_recordButton_clicked(false);
    }
    this->StopPollingThread();
    QMainWindow::closeEvent(event);
}

void MainWindow::on_baseFolderButton_clicked()
{
    bool isValid = false;
    this->StopTemperatureThread();
    while (!isValid)
    {
        QString baseFolderPath = QFileDialog::getExistingDirectory(
            this, tr("Open Directory"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (QDir(baseFolderPath).exists())
        {
            isValid = true;
            if (!baseFolderPath.isEmpty())
            {
                this->SetBaseFolder(baseFolderPath);
                ui->baseFolderLineEdit->clear();
                ui->baseFolderLineEdit->insert(this->GetBaseFolder());
                this->WriteLogHeader();
            }
        }
    }
    this->StartTemperatureThread();
}

void MainWindow::WriteLogHeader()
{
    auto version =
        QString(" SUSICAM Version: %1.%2.%3").arg(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);
    auto hash = " git hash: " + QString(GIT_COMMIT);
    this->LogMessage(hash, LOG_FILE_NAME, true);
    this->LogMessage(version, LOG_FILE_NAME, true);
}

QString MainWindow::GetLogFilePath(QString logFile)
{
    return QDir::cleanPath(ui->baseFolderLineEdit->text() + QDir::separator() + logFile);
}

QString MainWindow::LogMessage(QString message, QString logFile, bool logTime)
{
    auto timestamp = GetTimeStamp();
    QFile file(this->GetLogFilePath(logFile));
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    if (logTime)
    {
        stream << timestamp;
    }
    stream << message << "\n";
    file.close();
    return timestamp;
}

void MainWindow::on_subFolderLineEdit_returnPressed()
{
    m_subFolder = ui->subFolderLineEdit->text();
    RestoreLineEditStyle(ui->subFolderLineEdit);
}

void MainWindow::on_filePrefixLineEdit_returnPressed()
{
    m_recPrefixlineEdit = ui->filePrefixLineEdit->text();
    RestoreLineEditStyle(ui->filePrefixLineEdit);
}

bool MainWindow::GetNormalize() const
{
    return this->ui->normalizeCheckbox->isChecked();
}

unsigned MainWindow::GetBand() const
{
    return this->ui->bandSlider->value();
}

unsigned MainWindow::GetBGRNorm() const
{
    return this->ui->rgbNormSlider->value();
}

bool MainWindow::SetBaseFolder(QString baseFolderPath)
{
    if (QDir(baseFolderPath).exists())
    {
        m_baseFolderLoc = baseFolderPath;
        return true;
    }
    else
    {
        return false;
    }
}

QString MainWindow::GetBaseFolder() const
{
    return m_baseFolderLoc;
}

void MainWindow::ThreadedRecordImage()
{
    this->m_IOService.post([this] { RecordImage(false); });
}

void MainWindow::InitializeImageFileRecorder(std::string subFolder, std::string filePrefix)
{
    if (filePrefix.empty())
    {
        filePrefix = m_recPrefixlineEdit.toUtf8().constData();
    }
    if (subFolder.empty())
    {
        subFolder = m_subFolder.toStdString();
    }
    QString fullPath = GetFullFilenameStandardFormat(std::move(filePrefix), ".b2nd", std::move(subFolder));
    this->m_imageContainer.InitializeFile(fullPath.toStdString().c_str());
}

void MainWindow::RecordImage(bool ignoreSkipping)
{
    boost::this_thread::interruption_point();
    XI_IMG image = m_imageContainer.GetCurrentImage();
    boost::lock_guard<boost::mutex> guard(this->mtx_);
    static long lastImageID = image.acq_nframe;
    int nSkipFrames = ui->skipFramesSpinBox->value();
    if (this->ImageShouldBeRecorded(nSkipFrames, image.acq_nframe) || ignoreSkipping)
    {
        try
        {
            this->m_imageContainer.m_imageFile->write(image);
            m_recordedCount++;
        }
        catch (const std::runtime_error &e)
        {
            LOG_SUSICAM(error) << "Error while saving image: %s\n" << e.what();
        }
        this->DisplayRecordCount();
    }
    else
    {
        m_skippedCounter++;
    }
    lastImageID = image.acq_nframe;
}

bool MainWindow::ImageShouldBeRecorded(int nSkipFrames, long ImageID)
{
    return (nSkipFrames == 0) || (ImageID % nSkipFrames == 0);
}

void MainWindow::DisplayRecordCount()
{
    QMetaObject::invokeMethod(ui->recordedImagesLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(int, m_recordedCount));
}

void MainWindow::updateTimer()
{
    m_elapsedTime = static_cast<float>(m_elapsedTimer.elapsed()) / 1000.0;
    int totalSeconds = static_cast<int>(m_elapsedTime);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    m_elapsedTimeText.clear();
    m_elapsedTimeTextStream.seek(0);
    m_elapsedTimeTextStream.setFieldWidth(2); // Set field width to 2 or numbers and 1 for separators
    m_elapsedTimeTextStream.setPadChar('0');  // Zero-fill numbers
    m_elapsedTimeTextStream << hours;
    m_elapsedTimeTextStream.setFieldWidth(1);
    m_elapsedTimeTextStream << ":";
    m_elapsedTimeTextStream.setFieldWidth(2);
    m_elapsedTimeTextStream << minutes;
    m_elapsedTimeTextStream.setFieldWidth(1);
    m_elapsedTimeTextStream << ":";
    m_elapsedTimeTextStream.setFieldWidth(2);
    m_elapsedTimeTextStream << static_cast<int>(seconds);
    ui->timerLCDNumber->display(m_elapsedTimeText);
}

void MainWindow::stopTimer()
{
    ui->timerLCDNumber->display(0);
}

void MainWindow::CountImages()
{
    m_imageCounter++;
}

void MainWindow::StartRecording()
{
    // create thread for running the tasks posted to the IO service
    this->InitializeImageFileRecorder();
    this->m_IOService.reset();
    this->m_IOWork = std::make_unique<boost::asio::io_service::work>(this->m_IOService);
    for (int i = 0; i < 4; i++) // put 2 threads in thread pool
    {
        m_threadGroup.create_thread([&] { return m_IOService.run(); });
    }
    QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::ThreadedRecordImage);
    QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::CountImages);
    QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::updateTimer);
}

void MainWindow::StopRecording()
{
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::ThreadedRecordImage);
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::CountImages);
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::updateTimer);
    this->stopTimer();
    this->m_IOWork.reset();
    this->m_IOWork = nullptr;
    this->m_IOService.stop();
    this->m_threadGroup.interrupt_all();
    this->m_threadGroup.join_all();
    this->m_imageContainer.CloseFile();
    LOG_SUSICAM(info) << "Total of frames recorded: " << m_recordedCount;
    LOG_SUSICAM(info) << "Total of frames dropped : " << m_imageCounter - m_recordedCount;
    LOG_SUSICAM(info) << "Estimate for frames skipped: " << m_skippedCounter;
}

QString MainWindow::GetWritingFolder()
{
    QString writeFolder = GetBaseFolder();
    writeFolder += QDir::separator();
    return QDir::cleanPath(writeFolder);
}

void MainWindow::CreateFolderIfNecessary(QString folder)
{
    QDir folderDir(folder);

    if (!folderDir.exists())
    {
        if (folderDir.mkpath(folder))
        {
            LOG_SUSICAM(info) << "Directory created: " << folder.toStdString();
        }
    }
}

QString MainWindow::GetFullFilenameStandardFormat(std::string &&filePrefix, const std::string &extension,
                                                  std::string &&subFolder)
{
    QString writingFolder = GetWritingFolder() + QDir::separator() + QString::fromStdString(subFolder);
    if (!writingFolder.endsWith(QDir::separator()))
    {
        writingFolder += QDir::separator();
    }
    this->CreateFolderIfNecessary(writingFolder);

    QString fileName;
    if (!m_testMode)
    {
        fileName = QString::fromStdString(filePrefix);
    }
    else
    {
        fileName = QString("test");
    }
    fileName += QString::fromStdString(extension);

    return QDir::cleanPath(writingFolder + fileName);
}

void MainWindow::StartPollingThread()
{
    m_imageContainer.StartPolling();
    m_imageContainerThread =
        boost::thread(&ImageContainer::PollImage, &m_imageContainer, &m_cameraInterface.m_cameraHandle, 5);
}

void MainWindow::StopPollingThread()
{
    m_imageContainer.StopPolling();
    m_imageContainerThread.interrupt();
    m_imageContainerThread.join();
}

void MainWindow::on_autoexposureCheckbox_clicked(bool setAutoexposure)
{
    this->m_cameraInterface.m_camera->AutoExposure(setAutoexposure);
    ui->exposureSlider->setEnabled(!setAutoexposure);
    ui->exposureLineEdit->setEnabled(!setAutoexposure);
    UpdateExposure();
}

void MainWindow::on_whiteBalanceButton_clicked()
{
    if (m_referenceRecordingThread.joinable())
    {
        m_referenceRecordingThread.join();
    }
    m_referenceRecordingThread = boost::thread(&MainWindow::RecordReferenceImages, this, "white");
}

void MainWindow::on_darkCorrectionButton_clicked()
{
    if (m_referenceRecordingThread.joinable())
    {
        m_referenceRecordingThread.join();
    }
    m_referenceRecordingThread = boost::thread(&MainWindow::RecordReferenceImages, this, "dark");
}

void MainWindow::RecordReferenceImages(QString referenceType)
{
    QMetaObject::invokeMethod(ui->recordButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    if (referenceType == "white")
    {
        QMetaObject::invokeMethod(ui->darkCorrectionButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    }
    else if (referenceType == "dark")
    {
        QMetaObject::invokeMethod(ui->whiteBalanceButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    }

    QString baseFolder = ui->baseFolderLineEdit->text();
    QDir dir(baseFolder);
    QStringList nameFilters;
    nameFilters << referenceType + "*";
    QStringList fileNameList = dir.entryList(nameFilters, QDir::Files | QDir::NoDotAndDotDot);
    QRegularExpression re("^" + referenceType + "(\\d*)\\.[a-zA-Z0-9]+");

    int fileNum = 0;
    for (const QString &fileName : fileNameList)
    {
        QRegularExpressionMatch match = re.match(fileName);
        if (match.hasMatch())
        {
            fileNum = match.captured(1).toInt();
            ++fileNum;
        }
    }
    std::string filename;
    if (fileNum > 0)
    {
        filename = referenceType.toStdString() + std::to_string(fileNum);
    }
    else
    {
        filename = referenceType.toStdString();
    }
    this->InitializeImageFileRecorder("", filename);
    for (int i = 0; i < NR_REFERENCE_IMAGES_TO_RECORD; i++)
    {
        int exp_time = m_cameraInterface.m_camera->GetExposureMs();
        int waitTime = 2 * exp_time;
        wait(waitTime);
        this->RecordImage(true);
        int progress = static_cast<int>((static_cast<float>(i + 1) / NR_REFERENCE_IMAGES_TO_RECORD) * 100);
        QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
    }
    QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
    QMetaObject::invokeMethod(ui->recordButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    if (referenceType == "white")
    {
        QMetaObject::invokeMethod(ui->darkCorrectionButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    }
    else if (referenceType == "dark")
    {
        QMetaObject::invokeMethod(ui->whiteBalanceButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    }
}

void MainWindow::UpdateComponentEditedStyle(QLineEdit *lineEdit, const QString &newString,
                                            const QString &originalString)
{
    if (QString::compare(newString, originalString, Qt::CaseSensitive))
    {
        lineEdit->setStyleSheet(FIELD_EDITED_STYLE);
    }
    else
    {
        lineEdit->setStyleSheet(FIELD_ORIGINAL_STYLE);
    }
}

void MainWindow::RestoreLineEditStyle(QLineEdit *lineEdit)
{
    lineEdit->setStyleSheet(FIELD_ORIGINAL_STYLE);
}

void MainWindow::on_subFolderLineEdit_textEdited(const QString &newText)
{
    UpdateComponentEditedStyle(ui->subFolderLineEdit, newText, m_subFolder);
}

void MainWindow::on_filePrefixLineEdit_textEdited(const QString &newText)
{
    UpdateComponentEditedStyle(ui->filePrefixLineEdit, newText, m_recPrefixlineEdit);
}

void MainWindow::on_subFolderExtrasLineEdit_textEdited(const QString &newText)
{
    UpdateComponentEditedStyle(ui->subFolderExtrasLineEdit, newText, m_extrasSubFolder);
}

void MainWindow::on_subFolderExtrasLineEdit_returnPressed()
{
    m_extrasSubFolder = ui->subFolderExtrasLineEdit->text();
    RestoreLineEditStyle(ui->subFolderExtrasLineEdit);
}

void MainWindow::on_filePrefixExtrasLineEdit_textEdited(const QString &newText)
{
    UpdateComponentEditedStyle(ui->filePrefixExtrasLineEdit, newText, m_extrasFilePrefix);
}

void MainWindow::on_filePrefixExtrasLineEdit_returnPressed()
{
    m_extrasFilePrefix = ui->filePrefixExtrasLineEdit->text();
    RestoreLineEditStyle(ui->filePrefixExtrasLineEdit);
}

void MainWindow::on_logTextLineEdit_textEdited(const QString &newText)
{
    UpdateComponentEditedStyle(ui->logTextLineEdit, newText, m_triggerText);
}

QString MainWindow::FormatTimeStamp(QString timestamp)
{
    QDateTime dateTime = QDateTime::fromString(timestamp, "yyyyMMdd_HH-mm-ss-zzz");
    QString formattedDate = dateTime.toString("hh:mm:ss AP");
    return formattedDate;
}

void MainWindow::on_logTextLineEdit_returnPressed()
{
    QString timestamp;
    QString trigger_message = ui->logTextLineEdit->text();
    // block signals until method ends
    const QSignalBlocker triggerTextBlocker(ui->logTextLineEdit);
    const QSignalBlocker triggersTextEdit(ui->logTextEdit);
    // log message and update member variable for trigger text
    trigger_message.prepend(" ");
    timestamp = this->LogMessage(trigger_message, LOG_FILE_NAME, true);
    timestamp = FormatTimeStamp(timestamp);
    m_triggerText = QString("<span style=\"color:gray;\">%1</span>").arg(timestamp) +
                    QString("<b>%1</b>").arg(trigger_message) + "\n";

    // handle UI calls
    RestoreLineEditStyle(ui->logTextLineEdit);
    ui->logTextEdit->append(m_triggerText);
    ui->logTextEdit->show();
    ui->logTextLineEdit->clear();
}

/*
 * updates frames per second label in GUI when number of skipped frames is
 * modified
 */
void MainWindow::on_skipFramesSpinBox_valueChanged()
{
    // spin boxes do not have a returnPressed slot in Qt, which is why the value
    // is always updated upon changes
    int exp_ms = m_cameraInterface.m_camera->GetExposureMs();
    int nSkipFrames = ui->skipFramesSpinBox->value();
    const QSignalBlocker blocker_label(ui->hzLabel);
    ui->hzLabel->setText(QString::number((double)(1000.0 / (exp_ms * (nSkipFrames + 1))), 'g', 2));
}

void MainWindow::on_cameraListComboBox_currentIndexChanged(int index)
{
    boost::lock_guard<boost::mutex> guard(mtx_);
    // image acquisition should be stopped when index 0 (no camera) is selected
    // from the dropdown menu
    try
    {
        this->StopImageAcquisition();
        m_cameraInterface.CloseDevice();
    }
    catch (std::runtime_error &e)
    {
        LOG_SUSICAM(warning) << "could not stop image acquisition: " << e.what();
    }
    if (index != 0)
    {
        QString cameraModel = ui->cameraListComboBox->currentText();
        m_cameraInterface.m_cameraModel = cameraModel;
        if (CAMERA_MAPPER.contains(cameraModel))
        {
            QString cameraType = CAMERA_MAPPER.value(cameraModel).cameraType;
            QString originalCameraModel = m_cameraInterface.m_cameraModel;
            try
            {
                // set camera type needed by the camera interface initialization
                m_display->SetCameraProperties(cameraModel);
                m_cameraInterface.SetCameraProperties(cameraModel);
                this->StartImageAcquisition(ui->cameraListComboBox->currentText());
            }
            catch (std::runtime_error &e)
            {
                LOG_SUSICAM(error) << "could not start image acquisition for camera: " << cameraModel.toStdString();
                // restore camera type and index
                m_display->SetCameraProperties(originalCameraModel);
                m_cameraInterface.SetCameraProperties(originalCameraModel);
                const QSignalBlocker blocker_spinbox(ui->cameraListComboBox);
                ui->cameraListComboBox->setCurrentIndex(m_cameraInterface.m_cameraIndex);
                return;
            }
            // set new camera index
            m_cameraInterface.SetCameraIndex(index);
            this->EnableUi(true);
            if (cameraType == CAMERA_TYPE_SPECTRAL)
            {
                QMetaObject::invokeMethod(ui->bandSlider, "setEnabled", Q_ARG(bool, true));
            }
            else
            {
                QMetaObject::invokeMethod(ui->bandSlider, "setEnabled", Q_ARG(bool, false));
            }
        }
        else
        {
            LOG_SUSICAM(error) << "camera model not in CAMERA_MAPPER: " << cameraModel.toStdString();
        }
    }
    else
    {
        const QSignalBlocker blocker_spinbox(ui->cameraListComboBox);
        m_cameraInterface.SetCameraIndex(index);
        this->EnableUi(false);
    }
}

void MainWindow::UpdateSaturationPercentageLCDDisplays(cv::Mat &image) const
{
    if (image.empty() || image.type() != CV_8UC1)
    {
        throw std::invalid_argument("Invalid input matrix. It must be non-empty and of type CV_8UC1, "
                                    "got: " +
                                    cv::typeToString(image.type()));
    }

    int aboveThresholdCount = cv::countNonZero(image > OVEREXPOSURE_PIXEL_BOUNDARY_VALUE);
    double totalPixels = image.total(); // Total number of pixels in the matrix
    double percentageAboveThreshold = (static_cast<double>(aboveThresholdCount) / totalPixels) * 100.0;
    QString displayValue = QString::number(percentageAboveThreshold, 'f', 1);
    QMetaObject::invokeMethod(ui->overexposurePercentageLCDNumber, "display", Q_ARG(QString, displayValue));

    int belowThresholdCount = cv::countNonZero(image < UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE);
    double percentageBelowThreshold = (static_cast<double>(belowThresholdCount) / totalPixels) * 100.0;
    displayValue = QString::number(percentageBelowThreshold, 'f', 1);
    QMetaObject::invokeMethod(ui->underexposurePercentageLCDNumber, "display", Q_ARG(QString, displayValue));
}

void MainWindow::UpdateImage(cv::Mat &image, QImage::Format format, QGraphicsView *view,
                             std::unique_ptr<QGraphicsPixmapItem> &pixmapItem, QGraphicsScene *scene)
{
    QImage qtImage((uchar *)image.data, image.cols, image.rows, image.step, format);
    qtImage = qtImage.scaled(view->width(), view->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (pixmapItem == nullptr)
    {
        pixmapItem.reset(scene->addPixmap(QPixmap::fromImage(qtImage)));
        pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, false);
        pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    }
    else
    {
        pixmapItem->setPixmap(QPixmap::fromImage(qtImage));
    }
}

void MainWindow::UpdateRGBImage(cv::Mat &image)
{
    UpdateImage(image, QImage::Format_RGB888, this->ui->rgbImageGraphicsView, this->rgbPixMapItem,
                this->rgbScene.get());
}

void MainWindow::UpdateRawImage(cv::Mat &image)
{
    UpdateImage(image, QImage::Format_BGR888, this->ui->rawImageGraphicsView, this->rawPixMapItem,
                this->rawScene.get());
}

void MainWindow::SetGraphicsViewScene()
{
    this->ui->rgbImageGraphicsView->setScene(this->rgbScene.get());
    this->ui->rawImageGraphicsView->setScene(this->rawScene.get());
}
