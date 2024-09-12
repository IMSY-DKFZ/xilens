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
#include <b2nd.h>
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

MainWindow::MainWindow(QWidget *parent, const std::shared_ptr<XiAPIWrapper> &xiAPIWrapper)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_IOService(), m_temperatureIOService(),
      m_temperatureIOWork(new boost::asio::io_service::work(m_temperatureIOService)), m_cameraInterface(),
      m_recordedCount(0), m_testMode(g_commandLineArguments.test_mode), m_imageCounter(0), m_skippedCounter(0),
      m_elapsedTimeTextStream(&m_elapsedTimeText), m_elapsedTime(0), m_viewerThreadRunning(true),
      m_viewerThread(&MainWindow::ViewerWorkerThreadFunc, this)
{
    this->m_xiAPIWrapper = xiAPIWrapper == nullptr ? this->m_xiAPIWrapper : xiAPIWrapper;
    m_cameraInterface.Initialize(this->m_xiAPIWrapper);
    m_imageContainer.Initialize(this->m_xiAPIWrapper);
    m_updateFPSDisplayTimer = new QTimer(this);
    ui->setupUi(this);
    this->SetUpConnections();
    this->SetUpCustomUiComponents();

    // Initialize BLOSC2
    blosc2_init();

    // Display needs to be instantiated before changing the camera list because
    // calling setCurrentIndex on the list.
    m_display = new DisplayerFunctional(this);

    // populate available cameras
    ui->cameraListComboBox->addItem("select camera to enable UI...");
    this->HandleReloadCamerasPushButtonClicked();
    ui->cameraListComboBox->setCurrentIndex(0);

    // set the base folder path
    m_baseFolderPath = QDir::cleanPath(QDir::homePath());
    ui->baseFolderLineEdit->insert(this->GetBaseFolder());

    // synchronize expSlider and exposure checkbox
    QSlider *expSlider = ui->exposureSlider;
    QSpinBox *expSpinBox = ui->exposureSpinBox;
    // set default values and ranges
    expSpinBox->setValue(expSlider->value());

    LOG_XILENS(info) << "test mode (recording everything to same file) is set to: " << m_testMode << "\n";

    EnableUi(false);
}

void MainWindow::SetUpConnections()
{
    HANDLE_CONNECTION_RESULT(
        QObject::connect(ui->snapshotButton, &QPushButton::clicked, this, &MainWindow::HandleSnapshotButtonClicked));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(ui->exposureSlider, &QSlider::valueChanged, this, &MainWindow::HandleExposureValueChanged));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(ui->exposureSpinBox, &QSpinBox::valueChanged, this, &MainWindow::HandleExposureValueChanged));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->viewerImageSlider, &QSlider::valueChanged, this,
                                              &MainWindow::HandleViewerImageSliderValueChanged));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(ui->recordButton, &QPushButton::clicked, this, &MainWindow::HandleRecordButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->baseFolderButton, &QPushButton::clicked, this,
                                              &MainWindow::HandleBaseFolderButtonClicked));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(this, &MainWindow::ViewerImageProcessingComplete, this, &MainWindow::UpdateRawViewerImage));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->viewerFileButton, &QPushButton::clicked, this,
                                              &MainWindow::HandleViewerFileButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->fileNameLineEdit, &QLineEdit::textEdited, this,
                                              &MainWindow::HandleFileNameLineEditTextEdited));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->fileNameLineEdit, &QLineEdit::returnPressed, this,
                                              &MainWindow::HandleFileNameLineEditReturnPressed));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->autoexposureCheckbox, &QCheckBox::clicked, this,
                                              &MainWindow::HandleAutoexposureCheckboxClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->whiteBalanceButton, &QPushButton::clicked, this,
                                              &MainWindow::HandleWhiteBalanceButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->darkCorrectionButton, &QPushButton::clicked, this,
                                              &MainWindow::HandleDarkCorrectionButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->logTextLineEdit, &QLineEdit::textEdited, this,
                                              &MainWindow::HandleLogTextLineEditTextEdited));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->logTextLineEdit, &QLineEdit::returnPressed, this,
                                              &MainWindow::HandleLogTextLineEditReturnPressed));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->skipFramesSpinBox, &QSpinBox::valueChanged, this,
                                              &MainWindow::HandleSkipFramesSpinBoxValueChanged));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->cameraListComboBox, &QComboBox::currentIndexChanged, this,
                                              &MainWindow::HandleCameraListComboBoxCurrentIndexChanged));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->reloadCamerasPushButton, &QPushButton::clicked, this,
                                              &MainWindow::HandleReloadCamerasPushButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->fileNameSnapshotsLineEdit, &QLineEdit::textEdited, this,
                                              &MainWindow::HandleFileNameSnapshotsLineEditTextEdited));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->fileNameSnapshotsLineEdit, &QLineEdit::returnPressed, this,
                                              &MainWindow::HandleFileNameSnapshotsLineEditReturnPressed));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->baseFolderLineEdit, &QLineEdit::returnPressed, this,
                                              &MainWindow::HandleBaseFolderLineEditReturnPressed));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->baseFolderLineEdit, &QLineEdit::textEdited, this,
                                              &MainWindow::HandleBaseFolderLineEditTextEdited));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->viewerFileLineEdit, &QLineEdit::textEdited, this,
                                              &MainWindow::HandleViewerFileLineEditTextEdited));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->viewerFileLineEdit, &QLineEdit::returnPressed, this,
                                              &MainWindow::HandleViewerFileLineEditReturnPressed));
}

void MainWindow::HandleConnectionResult(bool status, const char *file, int line, const char *func)
{
    if (!status)
    {
        LOG_XILENS(error) << "Error when connecting/disconnecting slot to/from signal in" << file << ":" << line
                          << " @ " << func;
    }
}

void MainWindow::StartImageAcquisition(QString cameraIdentifier)
{
    try
    {
        this->m_display->StartDisplayer();
        m_cameraInterface.StartAcquisition(std::move(cameraIdentifier));
        this->StartPollingThread();
        this->StartTemperatureThread();

        // when a new image arrives, display it
        HANDLE_CONNECTION_RESULT(
            QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display));
    }
    catch (std::runtime_error &error)
    {
        LOG_XILENS(warning) << "could not start camera, got error " << error.what();
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
    HANDLE_CONNECTION_RESULT(
        QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display));
    LOG_XILENS(info) << "Stopped Image Acquisition";
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
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
#pragma clang diagnostic pop

void MainWindow::EnableUi(bool enable)
{
    QLayout *layout = ui->mainUiVerticalLayout->layout();
    EnableWidgetsInLayout(layout, enable);
    SetGraphicsViewScene();
    this->ui->exposureSlider->setEnabled(enable);
    this->ui->logTextLineEdit->setEnabled(enable);
}

void MainWindow::SetUpCustomUiComponents()
{
    // reload camera list button
    QIcon reloadButtonIcon;
    reloadButtonIcon.addFile(":/icon/theme/primary/reload.svg", QSize(), QIcon::Normal);
    reloadButtonIcon.addFile(":/icon/theme/disabled/reload.svg", QSize(), QIcon::Disabled);
    reloadButtonIcon.addFile(":/icon/theme/active/reload.svg", QSize(), QIcon::Active);
    this->ui->reloadCamerasPushButton->setIcon(reloadButtonIcon);
    // contrast tool button
    QIcon saturationButtonIcon;
    saturationButtonIcon.addFile(":/icon/theme/primary/saturation.svg", QSize(), QIcon::Normal);
    saturationButtonIcon.addFile(":/icon/theme/disabled/saturation.svg", QSize(), QIcon::Disabled);
    saturationButtonIcon.addFile(":/icon/theme/active/saturation.svg", QSize(), QIcon::Active);
    this->ui->saturationToolButton->setIcon(saturationButtonIcon);
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

    {
        boost::lock_guard<boost::mutex> lock(m_mutexImageViewer);
        m_viewerThreadRunning = false;
    }
    m_viewerQueueCondition.notify_all();
    if (m_viewerThread.joinable())
    {
        m_viewerThread.join();
    }

    this->StopTemperatureThread();
    this->StopSnapshotsThread();
    this->StopReferenceRecordingThread();

    HANDLE_CONNECTION_RESULT(
        QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display));

    blosc2_destroy();
    delete ui;
}

void MainWindow::RecordSnapshots()
{
    int nr_images = ui->nSnapshotsSpinBox->value();
    QMetaObject::invokeMethod(ui->nSnapshotsSpinBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    QMetaObject::invokeMethod(ui->fileNameSnapshotsLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));

    std::string fileName = ui->fileNameSnapshotsLineEdit->text().toUtf8().constData();

    if (fileName.empty())
    {
        fileName = m_fileName.toUtf8().constData();
    }
    QString filePath = GetFullFilenameStandardFormat(std::move(fileName), ".b2nd", "");
    auto image = m_imageContainer.GetCurrentImage();
    FileImage snapshotsFile(filePath.toStdString().c_str(), image.height, image.width);

    for (int i = 0; i < nr_images; i++)
    {
        int exp_time = m_cameraInterface.m_camera->GetExposureMs();
        int waitTime = 2 * exp_time;
        WaitMilliseconds(waitTime);
        image = m_imageContainer.GetCurrentImage();
        snapshotsFile.WriteImageData(image, GetCameraTemperature());
        int progress = static_cast<int>((static_cast<float>(i + 1) / static_cast<float>(nr_images)) * 100);
        QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
    }
    snapshotsFile.AppendMetadata();
    LOG_XILENS(info) << "Closed snapshot recording file";
    QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
    QMetaObject::invokeMethod(ui->nSnapshotsSpinBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    QMetaObject::invokeMethod(ui->fileNameSnapshotsLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
}

void MainWindow::HandleSnapshotButtonClicked()
{
    m_snapshotsThread = boost::thread(&MainWindow::RecordSnapshots, this);
}

QMap<QString, float> MainWindow::GetCameraTemperature() const
{
    m_cameraInterface.m_camera->m_cameraFamily->get()->UpdateCameraTemperature();
    auto cameraTemperature = m_cameraInterface.m_camera->m_cameraFamily->get()->m_cameraTemperature;
    return cameraTemperature;
}

void MainWindow::DisplayCameraTemperature()
{
    double temp = m_cameraInterface.m_camera->m_cameraFamily->get()->m_cameraTemperature.value(SENSOR_BOARD_TEMP);
    QMetaObject::invokeMethod(ui->temperatureLCDNumber, "display", Qt::QueuedConnection, Q_ARG(double, temp));
}

void MainWindow::ScheduleTemperatureThread()
{
    m_temperatureIOWork = std::make_unique<boost::asio::io_service::work>(m_temperatureIOService);
    m_temperatureThreadTimer = std::make_shared<boost::asio::steady_timer>(m_temperatureIOService);
    m_temperatureThreadTimer->expires_after(std::chrono::seconds(TEMP_LOG_INTERVAL));
    m_temperatureThreadTimer->async_wait(
        [this](const boost::system::error_code &error) { this->HandleTemperatureTimer(error); });
}

void MainWindow::HandleTemperatureTimer(const boost::system::error_code &error)
{
    if (error == boost::asio::error::operation_aborted)
    {
        LOG_XILENS(warning) << "Timer cancelled. Error: " << error;
        return;
    }

    m_cameraInterface.m_camera->m_cameraFamily->get()->UpdateCameraTemperature();
    this->DisplayCameraTemperature();

    // Reset timer
    m_temperatureThreadTimer->expires_after(std::chrono::seconds(TEMP_LOG_INTERVAL));
    m_temperatureThreadTimer->async_wait(
        [this](const boost::system::error_code &error) { this->HandleTemperatureTimer(error); });
}

void MainWindow::StartTemperatureThread()
{
    // Initial temperature update to ensure that it is populated before recordings start.
    m_cameraInterface.m_camera->m_cameraFamily->get()->UpdateCameraTemperature();
    if (m_temperatureThread.joinable())
    {
        StopTemperatureThread();
    }
    m_temperatureThread = boost::thread([&]() {
        ScheduleTemperatureThread();
        m_temperatureIOService.reset();
        m_temperatureIOService.run();
    });
    LOG_XILENS(info) << "Started temperature thread";
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
        LOG_XILENS(info) << "Stopped temperature thread";
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

void MainWindow::HandleExposureValueChanged(int value)
{
    m_cameraInterface.m_camera->SetExposureMs(value);
    UpdateExposure();
}

void MainWindow::HandleViewerImageSliderValueChanged(int value)
{
    {
        boost::lock_guard<boost::mutex> lock(m_mutexImageViewer);
        m_viewerSliderQueue.push(value);
    }
    m_viewerQueueCondition.notify_one();
}

void MainWindow::ProcessViewerImageSliderValueChanged(int value)
{
    std::array<int64_t, B2ND_MAX_DIM> slice_start = {0};
    std::array<int64_t, B2ND_MAX_DIM> slice_stop = {0};
    std::array<int64_t, B2ND_MAX_DIM> slice_shape = {0};
    for (int i = 0; i < this->m_viewerNDArray->ndim; i++)
    {
        slice_start[i] = i == 0 ? value : 0;
        slice_stop[i] = i == 0 ? value + 1 : this->m_viewerNDArray->shape[i];
        slice_shape[i] = slice_stop[i] - slice_start[i];
    }
    auto buffer_size =
        static_cast<int64_t>(this->m_viewerNDArray->shape[1] * this->m_viewerNDArray->shape[2] * sizeof(uint16_t));
    std::vector<uint16_t> buffer(this->m_viewerNDArray->shape[1] * this->m_viewerNDArray->shape[2]);
    b2nd_get_slice_cbuffer(this->m_viewerNDArray, slice_start.data(), slice_stop.data(), buffer.data(),
                           slice_shape.data(), buffer_size);

    // Get image dimensions, dimensions are transposed compared to what OpenCv expects.
    auto width = static_cast<int>(slice_shape[1]);
    auto height = static_cast<int>(slice_shape[2]);
    cv::Mat mat(width, height, CV_16UC1, buffer.data());
    mat /= 4;
    mat.convertTo(mat, CV_8UC1);

    // Indicate that processing is finished.
    emit ViewerImageProcessingComplete(mat);
}

void MainWindow::ViewerWorkerThreadFunc()
{
    // This function is running in a separate thread
    while (true)
    {
        int value = -1; // Default invalid value

        {
            boost::unique_lock<boost::mutex> lock(m_mutexImageViewer);
            m_viewerQueueCondition.wait(lock,
                                        [this]() { return !m_viewerSliderQueue.empty() || !m_viewerThreadRunning; });

            if (!m_viewerThreadRunning && m_viewerSliderQueue.empty())
            {
                break; // Exit condition to shut down the thread
            }

            if (!m_viewerSliderQueue.empty())
            {
                value = m_viewerSliderQueue.front();
                m_viewerSliderQueue.pop();
            }
        }

        if (value != -1)
        {
            ProcessViewerImageSliderValueChanged(value);
        }
    }
}

void MainWindow::UpdateExposure()
{
    // lock ui elements before updating them
    const QSignalBlocker exposureSliderLock(ui->exposureSlider);
    const QSignalBlocker exposureSpinBoxLock(ui->exposureSpinBox);
    int exp_ms = m_cameraInterface.m_camera->GetExposureMs();
    // update the estimated framerate
    int n_skip_frames = ui->skipFramesSpinBox->value();
    ui->hzLabel->setText(QString::number((double)(1000.0 / (exp_ms * (n_skip_frames + 1))), 'g', 2));
    // set exposure values to bot spinbox and slider
    ui->exposureSpinBox->setValue(exp_ms);
    ui->exposureSlider->setValue(exp_ms);
}

void MainWindow::HandleRecordButtonClicked(bool clicked)
{
    static QString original_colour;
    static QString original_button_text;

    if (clicked)
    {
        this->LogMessage(" XILENS RECORDING STARTS", LOG_FILE_NAME, true);
        this->LogMessage(QString(" camera selected: %1 %2")
                             .arg(this->m_cameraInterface.m_cameraIdentifier, this->m_cameraInterface.m_cameraSN),
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
        this->LogMessage(" XILENS RECORDING ENDS", LOG_FILE_NAME, true);
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
        QMetaObject::invokeMethod(ui->fileNameLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->cameraListComboBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->whiteBalanceButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->darkCorrectionButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->reloadCamerasPushButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->baseFolderLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    }
    else
    {
        QMetaObject::invokeMethod(ui->baseFolderButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->fileNameLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->cameraListComboBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->whiteBalanceButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->darkCorrectionButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->reloadCamerasPushButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->baseFolderLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (this->ui->recordButton->isChecked())
    {
        HandleRecordButtonClicked(false);
    }
    this->StopPollingThread();
    QMainWindow::closeEvent(event);
}

void MainWindow::HandleBaseFolderButtonClicked()
{
    bool isValid = false;
    while (!isValid)
    {
        QString baseFolderPath = QFileDialog::getExistingDirectory(
            this, tr("Open Directory"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (QDir(baseFolderPath).exists())
        {
            isValid = true;
            if (!baseFolderPath.isEmpty())
            {
                m_baseFolderPath = baseFolderPath;
                ui->baseFolderLineEdit->clear();
                ui->baseFolderLineEdit->insert(this->GetBaseFolder());
                this->WriteLogHeader();
            }
        }
    }
}

void MainWindow::HandleViewerFileButtonClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("NDArrays (*.b2nd)"));
    if (QFile(filePath).exists())
    {
        if (!filePath.isEmpty())
        {
            m_viewerFilePath = filePath;
            ui->viewerFileLineEdit->clear();
            ui->viewerFileLineEdit->insert(filePath);
            this->HandleViewerFileLineEditReturnPressed();
            OpenFileInViewer(m_viewerFilePath);
        }
    }
}

void MainWindow::OpenFileInViewer(const QString &filePath)
{
    char *path = strdup(filePath.toUtf8().constData());
    b2nd_open(path, &this->m_viewerNDArray);
    this->ui->viewerImageSlider->setEnabled(true);
    auto n_images = static_cast<int>(this->m_viewerNDArray->shape[0] - 1);
    this->ui->viewerImageSlider->setMaximum(n_images);
    this->HandleViewerImageSliderValueChanged(this->ui->viewerImageSlider->value());
}

void MainWindow::WriteLogHeader()
{
    auto version =
        QString(" XILENS Version: %1.%2.%3").arg(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);
    auto hash = " git hash: " + QString(GIT_COMMIT);
    this->LogMessage(hash, LOG_FILE_NAME, true);
    this->LogMessage(version, LOG_FILE_NAME, true);
}

QString MainWindow::GetLogFilePath(const QString &logFile)
{
    return QDir::cleanPath(ui->baseFolderLineEdit->text() + QDir::separator() + logFile);
}

QString MainWindow::LogMessage(const QString &message, const QString &logFile, bool logTime)
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

QString MainWindow::GetBaseFolder() const
{
    return m_baseFolderPath;
}

void MainWindow::ThreadedRecordImage()
{
    this->m_IOService.post([this] { RecordImage(false); });
}

void MainWindow::InitializeImageFileRecorder(std::string subFolder, std::string fileName)
{
    if (fileName.empty())
    {
        fileName = m_fileName.toUtf8().constData();
    }
    QString fullPath = GetFullFilenameStandardFormat(std::move(fileName), ".b2nd", std::move(subFolder));
    this->m_imageContainer.InitializeFile(fullPath.toStdString().c_str());
}

void MainWindow::RecordImage(bool ignoreSkipping)
{
    boost::this_thread::interruption_point();
    XI_IMG image = m_imageContainer.GetCurrentImage();
    boost::lock_guard<boost::mutex> guard(this->m_mutexImageRecording);
    static long lastImageID = image.acq_nframe;
    int nSkipFrames = ui->skipFramesSpinBox->value();
    if (MainWindow::ImageShouldBeRecorded(nSkipFrames, image.acq_nframe) || ignoreSkipping)
    {
        try
        {
            this->m_imageContainer.m_imageFile->WriteImageData(image, GetCameraTemperature());
            m_recordedCount++;
        }
        catch (const std::runtime_error &e)
        {
            LOG_XILENS(error) << "Error while saving image: %s\n" << e.what();
        }
        this->DisplayRecordCount();
        // register image recorded time and emit signal
        RegisterTimeImageRecorded();
    }
    else
    {
        m_skippedCounter++;
    }
    lastImageID = image.acq_nframe;
}

void MainWindow::RegisterTimeImageRecorded()
{
    auto now = std::chrono::steady_clock::now();
    m_recordedTimestamps.push_back(now);
    if (m_recordedTimestamps.size() > MAX_FRAMES_TO_COMPUTE_FPS)
    {
        m_recordedTimestamps.pop_front();
    }
}

bool MainWindow::ImageShouldBeRecorded(int nSkipFrames, long ImageID)
{
    return (nSkipFrames == 0) || (ImageID % nSkipFrames == 0);
}

void MainWindow::DisplayRecordCount()
{
    QMetaObject::invokeMethod(ui->recordedImagesLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(int, static_cast<int>(m_recordedCount.load())));
}

void MainWindow::UpdateTimer()
{
    m_elapsedTime = static_cast<double>(m_elapsedTimer.elapsed()) / 1000.0;
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

void MainWindow::StopTimer()
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
    HANDLE_CONNECTION_RESULT(
        QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::ThreadedRecordImage));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::CountImages));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::UpdateTimer));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(m_updateFPSDisplayTimer, &QTimer::timeout, this, &MainWindow::UpdateFPSLCDDisplay));
    m_updateFPSDisplayTimer->start(UPDATE_RATE_MS_FPS_TIMER);
}

void MainWindow::StopRecording()
{
    HANDLE_CONNECTION_RESULT(QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this,
                                                 &MainWindow::ThreadedRecordImage));
    HANDLE_CONNECTION_RESULT(
        QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::CountImages));
    HANDLE_CONNECTION_RESULT(
        QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::UpdateTimer));
    HANDLE_CONNECTION_RESULT(
        QObject::disconnect(m_updateFPSDisplayTimer, &QTimer::timeout, this, &MainWindow::UpdateFPSLCDDisplay));
    QMetaObject::invokeMethod(this->ui->fpsLCDNumber, "display", Qt::QueuedConnection, Q_ARG(QString, ""));
    this->StopTimer();
    this->m_IOWork.reset();
    this->m_IOWork = nullptr;
    this->m_IOService.stop();
    this->m_threadGroup.interrupt_all();
    this->m_threadGroup.join_all();
    this->m_imageContainer.CloseFile();
    LOG_XILENS(info) << "Total of frames recorded: " << m_recordedCount;
    LOG_XILENS(info) << "Total of frames dropped : " << m_imageCounter - m_recordedCount;
    LOG_XILENS(info) << "Estimate for frames skipped: " << m_skippedCounter;
}

QString MainWindow::GetWritingFolder()
{
    QString writeFolder = GetBaseFolder();
    writeFolder += QDir::separator();
    return QDir::cleanPath(writeFolder);
}

void MainWindow::CreateFolderIfNecessary(const QString &folder)
{
    QDir folderDir(folder);

    if (!folderDir.exists())
    {
        if (folderDir.mkpath(folder))
        {
            LOG_XILENS(info) << "Directory created: " << folder.toStdString();
        }
    }
}

QString MainWindow::GetFullFilenameStandardFormat(std::string &&fileName, const std::string &extension,
                                                  std::string &&subFolder)
{
    QString writingFolder = GetWritingFolder() + QDir::separator() + QString::fromStdString(subFolder);
    if (!writingFolder.endsWith(QDir::separator()))
    {
        writingFolder += QDir::separator();
    }
    MainWindow::CreateFolderIfNecessary(writingFolder);

    QString fullFileName;
    if (!m_testMode)
    {
        fullFileName = QString::fromStdString(fileName);
    }
    else
    {
        fullFileName = QString("test");
    }
    fullFileName += QString::fromStdString(extension);

    return QDir::cleanPath(writingFolder + fullFileName);
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

void MainWindow::HandleAutoexposureCheckboxClicked(bool setAutoexposure)
{
    this->m_cameraInterface.m_camera->AutoExposure(setAutoexposure);
    ui->exposureSlider->setEnabled(!setAutoexposure);
    ui->exposureSpinBox->setEnabled(!setAutoexposure);
    UpdateExposure();
}

void MainWindow::HandleWhiteBalanceButtonClicked()
{
    if (m_referenceRecordingThread.joinable())
    {
        m_referenceRecordingThread.join();
    }
    m_referenceRecordingThread = boost::thread(&MainWindow::RecordReferenceImages, this, "white");
}

void MainWindow::HandleDarkCorrectionButtonClicked()
{
    if (m_referenceRecordingThread.joinable())
    {
        m_referenceRecordingThread.join();
    }
    m_referenceRecordingThread = boost::thread(&MainWindow::RecordReferenceImages, this, "dark");
}

void MainWindow::RecordReferenceImages(const QString &referenceType)
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
        WaitMilliseconds(waitTime);
        this->RecordImage(true);
        int progress = static_cast<int>((static_cast<float>(i + 1) / NR_REFERENCE_IMAGES_TO_RECORD) * 100);
        QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
    }
    this->m_imageContainer.CloseFile();
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

void MainWindow::HandleFileNameLineEditReturnPressed()
{
    m_fileName = ui->fileNameLineEdit->text();
    RestoreLineEditStyle(ui->fileNameLineEdit);
}

void MainWindow::HandleViewerFileLineEditReturnPressed()
{
    auto file = QFile(ui->viewerFileLineEdit->text());
    if (file.exists())
    {
        m_viewerFilePath = ui->viewerFileLineEdit->text();
        OpenFileInViewer(m_viewerFilePath);
        RestoreLineEditStyle(ui->viewerFileLineEdit);
    }
    else
    {
        LOG_XILENS(error) << "Viewer file path does not exist.";
    }
}

void MainWindow::HandleFileNameSnapshotsLineEditReturnPressed()
{
    if (m_fileName == ui->fileNameSnapshotsLineEdit->text())
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle("Error");
        msgBox.setText("<b>Invalid file name.</b>");
        msgBox.setInformativeText("Snapshot file name cannot be the same as video recording file name.");
        msgBox.exec();
        return;
    }
    m_snapshotsFileName = ui->fileNameSnapshotsLineEdit->text();
    RestoreLineEditStyle(ui->fileNameSnapshotsLineEdit);
}

void MainWindow::HandleBaseFolderLineEditReturnPressed()
{
    m_baseFolderPath = ui->baseFolderLineEdit->text();
    RestoreLineEditStyle(ui->baseFolderLineEdit);
}

void MainWindow::HandleLogTextLineEditReturnPressed()
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

void MainWindow::HandleFileNameLineEditTextEdited(const QString &newText)
{
    UpdateComponentEditedStyle(ui->fileNameLineEdit, newText, m_fileName);
}

void MainWindow::HandleFileNameSnapshotsLineEditTextEdited(const QString &newText)
{
    UpdateComponentEditedStyle(ui->fileNameSnapshotsLineEdit, newText, m_snapshotsFileName);
}

void MainWindow::HandleLogTextLineEditTextEdited(const QString &newText)
{
    UpdateComponentEditedStyle(ui->logTextLineEdit, newText, m_triggerText);
}

void MainWindow::HandleBaseFolderLineEditTextEdited(const QString &newText)
{
    UpdateComponentEditedStyle(ui->baseFolderLineEdit, newText, m_baseFolderPath);
}

void MainWindow::HandleViewerFileLineEditTextEdited(const QString &newText)
{
    UpdateComponentEditedStyle(ui->viewerFileLineEdit, newText, m_viewerFilePath);
}

QString MainWindow::FormatTimeStamp(const QString &timestamp)
{
    QDateTime dateTime = QDateTime::fromString(timestamp, "yyyyMMdd_HH-mm-ss-zzz");
    QString formattedDate = dateTime.toString("hh:mm:ss AP");
    return formattedDate;
}

/*
 * updates frames per second label in GUI when the number of skipped frames is
 * modified
 */
void MainWindow::HandleSkipFramesSpinBoxValueChanged()
{
    // spin boxes do not have a returnPressed slot in Qt, which is why the value
    // is always updated upon changes
    int exp_ms = m_cameraInterface.m_camera->GetExposureMs();
    int nSkipFrames = ui->skipFramesSpinBox->value();
    const QSignalBlocker blocker_label(ui->hzLabel);
    ui->hzLabel->setText(QString::number((double)(1000.0 / (exp_ms * (nSkipFrames + 1))), 'g', 2));
}

void MainWindow::HandleCameraListComboBoxCurrentIndexChanged(int index)
{
    boost::lock_guard<boost::mutex> guard(m_mutexImageRecording);
    // image acquisition should be stopped when index 0 (no camera) is selected
    // from the dropdown menu
    try
    {
        this->StopImageAcquisition();
        m_cameraInterface.CloseDevice();
    }
    catch (std::runtime_error &e)
    {
        LOG_XILENS(warning) << "could not stop image acquisition: " << e.what();
    }
    if (index != 0)
    {
        QString cameraIdentifier = ui->cameraListComboBox->currentText();
        QString cameraModel = cameraIdentifier.split("@").at(0);
        m_cameraInterface.m_cameraIdentifier = cameraIdentifier;
        if (getCameraMapper().contains(cameraModel))
        {
            QString cameraType = getCameraMapper().value(cameraModel).cameraType;
            QString originalCameraIdentifier = m_cameraInterface.m_cameraIdentifier;
            try
            {
                // set the camera type needed by the camera interface initialization
                m_display->SetCameraProperties(cameraModel);
                m_cameraInterface.SetCameraProperties(cameraModel);
                this->StartImageAcquisition(cameraIdentifier);
            }
            catch (std::runtime_error &e)
            {
                LOG_XILENS(error) << "could not start image acquisition for camera: " << cameraIdentifier.toStdString();
                // restore camera type and index
                m_display->SetCameraProperties(originalCameraIdentifier);
                m_cameraInterface.SetCameraProperties(originalCameraIdentifier);
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
            LOG_XILENS(error) << "camera model not in CAMERA_MAPPER: " << cameraModel.toStdString();
        }
    }
    else
    {
        const QSignalBlocker blocker_spinbox(ui->cameraListComboBox);
        m_cameraInterface.SetCameraIndex(index);
        this->EnableUi(false);
    }
}

void MainWindow::HandleReloadCamerasPushButtonClicked()
{
    // set button style as pressed down, and process event loop before continuing
    ui->reloadCamerasPushButton->setDown(true);
    QCoreApplication::processEvents();

    QStringList cameraList = m_cameraInterface.GetAvailableCameraIdentifiers();
    // Only add new camera models
    for (const QString &camera : cameraList)
    {
        if (ui->cameraListComboBox->findText(camera) == -1)
        {
            ui->cameraListComboBox->addItem(camera);
        }
    }

    // Remove camera models that are no longer available except for the first placeholder
    int i = 1;
    while (i < ui->cameraListComboBox->count())
    {
        if (cameraList.contains(ui->cameraListComboBox->itemText(i)))
        {
            ++i;
        }
        else
        {
            ui->cameraListComboBox->removeItem(i);
        }
    }

    // restore button style
    ui->reloadCamerasPushButton->setDown(false);
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
    auto totalPixels = static_cast<double>(image.total()); // Total number of pixels in the matrix
    double percentageAboveThreshold = (static_cast<double>(aboveThresholdCount) / totalPixels) * 100.0;
    QString displayValue = QString::number(percentageAboveThreshold, 'f', 1);
    QMetaObject::invokeMethod(ui->overexposurePercentageLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(QString, displayValue));

    int belowThresholdCount = cv::countNonZero(image < UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE);
    double percentageBelowThreshold = (static_cast<double>(belowThresholdCount) / totalPixels) * 100.0;
    displayValue = QString::number(percentageBelowThreshold, 'f', 1);
    QMetaObject::invokeMethod(ui->underexposurePercentageLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(QString, displayValue));
}

void MainWindow::UpdateFPSLCDDisplay()
{
    using namespace std::chrono;
    if (this->m_recordedTimestamps.size() < 2)
    {
        return;
    }
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(this->m_recordedTimestamps.back() -
                                                                          this->m_recordedTimestamps.front())
                        .count();
    double fps = (static_cast<double>(this->m_recordedTimestamps.size()) - 1) * 1000.0 / static_cast<double>(duration);
    QString displayValue = QString::number(fps, 'f', 1);
    QMetaObject::invokeMethod(this->ui->fpsLCDNumber, "display", Qt::QueuedConnection, Q_ARG(QString, displayValue));
}

void MainWindow::UpdateImage(cv::Mat &image, QImage::Format format, QGraphicsView *view,
                             std::unique_ptr<QGraphicsPixmapItem> &pixmapItem, QGraphicsScene *scene)
{
    QImage qtImage((uchar *)image.data, image.cols, image.rows, static_cast<long>(image.step), format);
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
    UpdateImage(image, QImage::Format_RGB888, this->ui->rgbImageGraphicsView, this->m_rgbPixMapItem,
                this->m_rgbScene.get());
}

void MainWindow::UpdateRawImage(cv::Mat &image)
{
    UpdateImage(image, QImage::Format_BGR888, this->ui->rawImageGraphicsView, this->m_rawPixMapItem,
                this->m_rawScene.get());
}

void MainWindow::UpdateRawViewerImage(cv::Mat &image)
{
    UpdateImage(image, QImage::Format_Grayscale8, this->ui->viewerGraphicsView, this->m_rawViewerPixMapItem,
                this->m_rawViewerScene.get());
}

void MainWindow::SetGraphicsViewScene()
{
    this->ui->rgbImageGraphicsView->setScene(this->m_rgbScene.get());
    this->ui->rawImageGraphicsView->setScene(this->m_rawScene.get());
    this->ui->viewerGraphicsView->setScene(this->m_rawViewerScene.get());
}

bool MainWindow::IsSaturationButtonChecked()
{
    return this->ui->saturationToolButton->isChecked();
}

void MainWindow::SetRecordedCount(int count)
{
    m_recordedCount = count;
}
