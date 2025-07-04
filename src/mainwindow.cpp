/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QMessageBox>
#include <QTextStream>
#include <QUrl>
#include <b2nd.h>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <opencv2/core/types_c.h>
#include <string>
#include <utility>

#include "constants.h"
#include "displayFunctional.h"
#include "errors.h"
#include "imageContainer.h"
#include "logger.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include "xiAPIWrapper.h"

MainWindow::MainWindow(QWidget *parent, const std::shared_ptr<XiAPIWrapper> &xiAPIWrapper)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_elapsedTime(0), m_elapsedTimeTextStream(&m_elapsedTimeText),
      m_testMode(g_commandLineArguments.test_mode), m_viewerThread(&MainWindow::ViewerWorkerThreadFunc, this),
      m_viewerThreadRunning(true), m_temperatureIOWork(new boost::asio::io_service::work(m_temperatureIOService)),
      m_recordedCount(0), m_imageCounter(0), m_skippedCounter(0),
      m_bandSelectorSliderPopup(new QSliderPopup(1, 16, 10, Qt::Orientation::Horizontal)),
      m_rgbNormSliderPopup(new QSliderPopup(1, 30, 5, Qt::Orientation::Horizontal)),
      m_snapshotPopup(new QLineSpinPopup(this)),
      m_saturationSpinBoxesPopup(new QDoubleSpinBoxesWithColorPickersPopup(this)),
      m_rgbChannelSpinBoxesPopup(new QRgbChannelSpinBoxesPopup(this))
{
    this->m_xiAPIWrapper = xiAPIWrapper == nullptr ? this->m_xiAPIWrapper : xiAPIWrapper;
    m_cameraInterface.Initialize(this->m_xiAPIWrapper);
    m_imageContainer.Initialize(this->m_xiAPIWrapper);
    m_updateFPSDisplayTimer = new QTimer(this);
    ui->setupUi(this);
    this->SetUpCustomUiComponents();

    // Initialize BLOSC2
    blosc2_init();

    // Display needs to be instantiated before changing the camera list because
    // calling setCurrentIndex on the list.
    m_display = new DisplayerFunctional(this);

    // populate available cameras
    ui->cameraListComboBox->addItem("select camera to enable UI...");
    this->HandleReloadCamerasToolButtonClicked();
    ui->cameraListComboBox->setCurrentIndex(0);

    // set the base folder path
    m_baseFolderPath = QDir::cleanPath(QDir::homePath());
    ui->baseFolderLineEdit->insert(this->GetBaseFolder());

    LOG_XILENS(info) << "test mode (recording everything to same file) is set to: " << m_testMode << "\n";
    this->SetUpConnections();
    EnableUi(false);
}

void MainWindow::SetUpConnections()
{
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->recordSnapshotToolButton, &QToolButton::clicked, this,
                                              &MainWindow::HandleSnapshotToolButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->recordSnapshotToolButton, &QArrowToolButton::ArrowClicked, this,
                                              &MainWindow::HandleSnapshotToolButtonArrowClicked));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(ui->exposureSpinBox, &QSpinBox::valueChanged, this, &MainWindow::HandleExposureValueChanged));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->viewerImageSlider, &QSlider::valueChanged, this,
                                              &MainWindow::HandleViewerImageSliderValueChanged));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(ui->recordToolButton, &QPushButton::clicked, this, &MainWindow::HandleRecordButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->baseFolderButton, &QPushButton::clicked, this,
                                              &MainWindow::HandleBaseFolderButtonClicked));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(this, &MainWindow::ViewerImageProcessingComplete, this, &MainWindow::UpdateRawViewerImage));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->viewerFileButton, &QPushButton::clicked, this,
                                              &MainWindow::HandleViewerFileButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->fileNameLineEdit, &QLineEdit::textEdited, this,
                                              &MainWindow::HandleFileNameLineEditTextEdited));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->autoExposureToolButton, &QToolButton::clicked, this,
                                              &MainWindow::HandleAutoexposureToolButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->bandSelectorToolButton, &QToolButton::clicked, this,
                                              &MainWindow::HandleBandSelectorToolButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->rgbNormToolButton, &QToolButton::clicked, this,
                                              &MainWindow::HandleRGBNormToolButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->recordWhiteToolButton, &QPushButton::clicked, this,
                                              &MainWindow::HandleWhiteBalanceButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->recordDarkToolButton, &QPushButton::clicked, this,
                                              &MainWindow::HandleDarkCorrectionButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->logTextLineEdit, &QLineEdit::textEdited, this,
                                              &MainWindow::HandleLogTextLineEditTextEdited));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->logTextLineEdit, &QLineEdit::returnPressed, this,
                                              &MainWindow::HandleLogTextLineEditReturnPressed));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->skipFramesSpinBox, &QSpinBox::valueChanged, this,
                                              &MainWindow::HandleSkipFramesSpinBoxValueChanged));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->cameraListComboBox, &QComboBox::currentIndexChanged, this,
                                              &MainWindow::HandleCameraListComboBoxCurrentIndexChanged));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->reloadCamerasToolButton, &QPushButton::clicked, this,
                                              &MainWindow::HandleReloadCamerasToolButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(m_snapshotPopup->m_lineEdit, &QLineEdit::textEdited, this,
                                              &MainWindow::HandleFileNameSnapshotsLineEditTextEdited));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->baseFolderLineEdit, &QLineEdit::textEdited, this,
                                              &MainWindow::HandleBaseFolderLineEditTextEdited));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->viewerFileLineEdit, &QLineEdit::textEdited, this,
                                              &MainWindow::HandleViewerFileLineEditTextEdited));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->viewerFileLineEdit, &QLineEdit::returnPressed, this,
                                              &MainWindow::HandleViewerFileLineEditReturnPressed));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->reloadViewerFileToolButton, &QToolButton::clicked, this,
                                              &MainWindow::HandleReloadViewerFileToolButtonClicked));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(m_display, &Displayer::ImageReadyToUpdateRGB, this, &MainWindow::UpdateRGBImage));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(m_display, &Displayer::ImageReadyToUpdateRaw, this, &MainWindow::UpdateRawImage));
    HANDLE_CONNECTION_RESULT(QObject::connect(m_display, &Displayer::SaturationPercentageReady, this,
                                              &MainWindow::UpdateSaturationPercentageLCDDisplays));
    HANDLE_CONNECTION_RESULT(QObject::connect(m_saturationSpinBoxesPopup, &QDoubleSpinBoxesPopup::minValueChanged, this,
                                              &MainWindow::HandleSaturationMinValueChanged));
    HANDLE_CONNECTION_RESULT(QObject::connect(m_saturationSpinBoxesPopup, &QDoubleSpinBoxesPopup::maxValueChanged, this,
                                              &MainWindow::HandleSaturationMaxValueChanged));
    HANDLE_CONNECTION_RESULT(QObject::connect(m_saturationSpinBoxesPopup,
                                              &QDoubleSpinBoxesWithColorPickersPopup::leftColorChanged, this,
                                              &MainWindow::HandleSaturationDarkColorChanged));
    HANDLE_CONNECTION_RESULT(QObject::connect(m_saturationSpinBoxesPopup,
                                              &QDoubleSpinBoxesWithColorPickersPopup::rightColorChanged, this,
                                              &MainWindow::HandleSaturationSaturatedColorChanged));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->saturationToolButton, &QArrowToolButton::ArrowClicked, this,
                                              &MainWindow::HandleSaturationToolButtonArrowClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->rgbChannelToolButton, &QToolButton::clicked, this,
                                              &MainWindow::HandleRgbChannelToolButtonClicked));
    HANDLE_CONNECTION_RESULT(QObject::connect(m_rgbChannelSpinBoxesPopup, &QRgbChannelSpinBoxesPopup::ValueChanged,
                                              m_display, &Displayer::UpdateBGRChannels));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::HandleAboutActionTriggered));
    HANDLE_CONNECTION_RESULT(QObject::connect(ui->actionDocumentation, &QAction::triggered, this,
                                              &MainWindow::HandleDocumentationActionTriggered));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(ui->actionHowToCite, &QAction::triggered, this, &MainWindow::HandleHowToCiteActionTriggered));
}

void MainWindow::HandleConnectionResult(const bool status, const char *file, const int line, const char *func)
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
    (void)m_cameraInterface.StopAcquisition();
    // disconnect slots for image display
    HANDLE_CONNECTION_RESULT(
        QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display));
    LOG_XILENS(info) << "Stopped Image Acquisition";
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
void MainWindow::EnableWidgetsInLayout(const QLayout *layout, const bool enable)
{
    for (int i = 0; i < layout->count(); ++i)
    {
        const QLayout *subLayout = layout->itemAt(i)->layout();
        if (const auto widget = layout->itemAt(i)->widget())
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

void MainWindow::EnableUi(const bool enable) const
{
    EnableWidgetsInLayout(ui->mainUiVerticalLayout->layout(), enable);
    SetGraphicsViewScene();
    EnableWidgetsInLayout(ui->recordingControlsHorizontalLayout->layout(), enable);
    ui->logTextLineEdit->setEnabled(enable);
    this->m_bandSelectorSliderPopup->setEnabled(enable);
    ui->autoExposureToolButton->setEnabled(enable);
}

void MainWindow::SetUpCustomUiComponents() const
{
    // set tool tips
    m_bandSelectorSliderPopup->setToolTip("Image band to display");
    m_rgbNormSliderPopup->setToolTip("RGB image intensity level");
    m_snapshotPopup->m_lineEdit->setToolTip("File name");
    m_snapshotPopup->m_lineEdit->setPlaceholderText("File name ...");
    m_snapshotPopup->m_spinBox->setToolTip("Number of images to record");
    m_saturationSpinBoxesPopup->setToolTips("Under-exposure color", "Over-exposure color", "Minimum value",
                                            "Maximum value");
    // reload camera list button
    QIcon reloadButtonIcon;
    reloadButtonIcon.addFile(":/icon/theme/primary/reload.svg", QSize(), QIcon::Normal);
    reloadButtonIcon.addFile(":/icon/theme/disabled/reload.svg", QSize(), QIcon::Disabled);
    reloadButtonIcon.addFile(":/icon/theme/active/reload.svg", QSize(), QIcon::Active);
    ui->reloadCamerasToolButton->setIcon(reloadButtonIcon);
    // contrast tool button
    QIcon saturationButtonIcon;
    saturationButtonIcon.addFile(":/icon/theme/primary/saturation.svg", QSize(), QIcon::Normal);
    saturationButtonIcon.addFile(":/icon/theme/disabled/saturation.svg", QSize(), QIcon::Disabled);
    saturationButtonIcon.addFile(":/icon/theme/active/saturation.svg", QSize(), QIcon::Active);
    ui->saturationToolButton->setIcon(saturationButtonIcon);
    // image normalization
    QIcon normalizationButtonIcon;
    normalizationButtonIcon.addFile(":/icon/theme/primary/normalization.svg", QSize(), QIcon::Normal);
    normalizationButtonIcon.addFile(":/icon/theme/disabled/normalization.svg", QSize(), QIcon::Disabled);
    normalizationButtonIcon.addFile(":/icon/theme/active/normalization.svg", QSize(), QIcon::Active);
    ui->normalizeImageToolButton->setIcon(normalizationButtonIcon);
    // auto exposure
    QIcon autoExposureButtonIcon;
    autoExposureButtonIcon.addFile(":/icon/theme/primary/auto_exposure.svg", QSize(), QIcon::Normal);
    autoExposureButtonIcon.addFile(":/icon/theme/disabled/auto_exposure.svg", QSize(), QIcon::Disabled);
    autoExposureButtonIcon.addFile(":/icon/theme/active/auto_exposure.svg", QSize(), QIcon::Active);
    ui->autoExposureToolButton->setIcon(autoExposureButtonIcon);
    // band selector
    QIcon bandSelectorButtonIcon;
    bandSelectorButtonIcon.addFile(":/icon/theme/primary/band_selector.svg", QSize(), QIcon::Normal);
    bandSelectorButtonIcon.addFile(":/icon/theme/disabled/band_selector.svg", QSize(), QIcon::Disabled);
    bandSelectorButtonIcon.addFile(":/icon/theme/active/band_selector.svg", QSize(), QIcon::Active);
    ui->bandSelectorToolButton->setIcon(bandSelectorButtonIcon);
    // image intensity
    QIcon imageIntensityButtonIcon;
    imageIntensityButtonIcon.addFile(":/icon/theme/primary/rgb_norm.svg", QSize(), QIcon::Normal);
    imageIntensityButtonIcon.addFile(":/icon/theme/disabled/rgb_norm.svg", QSize(), QIcon::Disabled);
    imageIntensityButtonIcon.addFile(":/icon/theme/active/rgb_norm.svg", QSize(), QIcon::Active);
    ui->rgbNormToolButton->setIcon(imageIntensityButtonIcon);
    // record
    this->SetRecordButtonIcons(false);
    // record white
    QIcon recordWhiteButtonIcon;
    recordWhiteButtonIcon.addFile(":/icon/theme/primary/record_white.svg", QSize(), QIcon::Normal);
    recordWhiteButtonIcon.addFile(":/icon/theme/disabled/record_white.svg", QSize(), QIcon::Disabled);
    recordWhiteButtonIcon.addFile(":/icon/theme/active/record_white.svg", QSize(), QIcon::Active);
    ui->recordWhiteToolButton->setIcon(recordWhiteButtonIcon);
    // record dark
    QIcon recordDarkButtonIcon;
    recordDarkButtonIcon.addFile(":/icon/theme/primary/record_dark.svg", QSize(), QIcon::Normal);
    recordDarkButtonIcon.addFile(":/icon/theme/disabled/record_dark.svg", QSize(), QIcon::Disabled);
    recordDarkButtonIcon.addFile(":/icon/theme/active/record_dark.svg", QSize(), QIcon::Active);
    ui->recordDarkToolButton->setIcon(recordDarkButtonIcon);
    // record snapshots
    QIcon recordSnapshotsButtonIcon;
    recordSnapshotsButtonIcon.addFile(":/icon/theme/primary/snapshot.svg", QSize(), QIcon::Normal);
    recordSnapshotsButtonIcon.addFile(":/icon/theme/disabled/snapshot.svg", QSize(), QIcon::Disabled);
    recordSnapshotsButtonIcon.addFile(":/icon/theme/active/snapshot.svg", QSize(), QIcon::Active);
    ui->recordSnapshotToolButton->setIcon(recordSnapshotsButtonIcon);
    // RGB channel selector
    QIcon rgbChannelButtonIcon;
    rgbChannelButtonIcon.addFile(":/icon/theme/primary/rgb_channel.svg", QSize(), QIcon::Normal);
    rgbChannelButtonIcon.addFile(":/icon/theme/disabled/rgb_channel.svg", QSize(), QIcon::Disabled);
    rgbChannelButtonIcon.addFile(":/icon/theme/active/rgb_channel.svg", QSize(), QIcon::Active);
    ui->rgbChannelToolButton->setIcon(rgbChannelButtonIcon);
    // reload viewer file button
    QIcon reloadViewerFileButtonIcon;
    reloadViewerFileButtonIcon.addFile(":/icon/theme/primary/reload.svg", QSize(), QIcon::Normal);
    reloadViewerFileButtonIcon.addFile(":/icon/theme/disabled/reload.svg", QSize(), QIcon::Disabled);
    reloadViewerFileButtonIcon.addFile(":/icon/theme/active/reload.svg", QSize(), QIcon::Active);
    ui->reloadViewerFileToolButton->setIcon(reloadViewerFileButtonIcon);
}

void MainWindow::Display()
{
    static boost::posix_time::ptime last = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();

    // display new images with at most every 35ms
    if ((now - last).total_milliseconds() > 35)
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
    const int nrImages = m_snapshotPopup->value();
    ToggleSnapshotUI(false);

    std::string fileName = m_snapshotPopup->text().toUtf8().constData();
    const QString filePath = GetFullFilenameStandardFormat(std::move(fileName), ".b2nd", "");
    const auto snapshotsFile = OpenFileForSnapshots(filePath);
    if (!snapshotsFile)
    {
        // If the file couldn't be opened, restore UI and exit
        ResetSnapshotUI();
        return;
    }

    for (int i = 0; i < nrImages; i++)
    {
        CaptureAndStoreSnapshotImage(*snapshotsFile, i, nrImages);
    }
    snapshotsFile->AppendMetadata();
    LOG_XILENS(info) << "Closed snapshot recording file";
    ResetSnapshotUI();
}

std::unique_ptr<FileImage> MainWindow::OpenFileForSnapshots(const QString &filePath)
{
    auto image = m_imageContainer.GetCurrentImage();
    try
    {
        return std::make_unique<FileImage>(filePath.toStdString().c_str(), image.height, image.width);
    }
    catch (const XiLensError &error)
    {
        LOG_XILENS(error) << "Could not open snapshot recording file: " << filePath.toStdString();

        // Use a helper for showing error dialogs
        ShowErrorDialog("Invalid file name.", error.toString().data());
        return nullptr; // Return nullptr to indicate failure
    }
}

void MainWindow::CaptureAndStoreSnapshotImage(FileImage &snapshotsFile, const int currentIndex, const int totalImages)
{
    const int exposure = m_cameraInterface.m_camera->GetExposureMs();
    const int waitTime = 2 * exposure;
    WaitMilliseconds(waitTime);

    // Capture the current image
    const auto image = m_imageContainer.GetCurrentImage();
    snapshotsFile.WriteImageData(image, GetCameraTemperature());

    // Update progress bar
    const int progress = static_cast<int>((currentIndex + 1) * 100.0 / totalImages);
    QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
}

void MainWindow::ToggleSnapshotUI(const bool enabled) const
{
    QMetaObject::invokeMethod(m_snapshotPopup->m_spinBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, enabled));
    QMetaObject::invokeMethod(m_snapshotPopup->m_lineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, enabled));
}

void MainWindow::ResetSnapshotUI() const
{
    QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
    ToggleSnapshotUI(true);
}

void MainWindow::ShowErrorDialog(const QString &text, const QString &informativeText)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText("<b>" + text + "</b>");
    msgBox.setInformativeText(informativeText);
    msgBox.exec();
}

void MainWindow::HandleSnapshotToolButtonClicked()
{
    if (HandleFileNameSnapshotsLineEditTextEdited(m_snapshotPopup->text()))
    {
        return;
    }
    m_snapshotsThread = boost::thread(&MainWindow::RecordSnapshots, this);
}

QMap<QString, float> MainWindow::GetCameraTemperature() const
{
    m_cameraInterface.m_camera->m_cameraFamily->get()->UpdateCameraTemperature();
    auto cameraTemperature = m_cameraInterface.m_camera->m_cameraFamily->get()->m_cameraTemperature;
    return cameraTemperature;
}

void MainWindow::DisplayCameraTemperature() const
{
    const double temp = m_cameraInterface.m_camera->m_cameraFamily->get()->m_cameraTemperature.value(SENSOR_BOARD_TEMP);
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
    m_temperatureThreadTimer->async_wait([this](const boost::system::error_code &errorTemperatureTimer) {
        this->HandleTemperatureTimer(errorTemperatureTimer);
    });
}

void MainWindow::StartTemperatureThread()
{
    // Initial temperature update to ensure that it is populated before recordings start.
    m_cameraInterface.m_camera->m_cameraFamily->get()->UpdateCameraTemperature();
    if (m_temperatureThread.joinable())
    {
        StopTemperatureThread();
    }
    m_temperatureThread = boost::thread([&] {
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
        ui->temperatureLCDNumber->display(0);
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

void MainWindow::HandleExposureValueChanged(const int value) const
{
    m_cameraInterface.m_camera->SetExposureMs(value);
    UpdateExposure();
}

void MainWindow::HandleBandSelectorToolButtonClicked() const
{
    constexpr int popupWidth = 450;
    constexpr int popupHeight = 50;
    ShowPopupOnToolButtonInteraction(ui->bandSelectorToolButton, m_bandSelectorSliderPopup, popupWidth, popupHeight,
                                     true);
}

void MainWindow::HandleRGBNormToolButtonClicked() const
{
    constexpr int popupWidth = 400;
    constexpr int popupHeight = 50;
    ShowPopupOnToolButtonInteraction(ui->rgbNormToolButton, m_rgbNormSliderPopup, popupWidth, popupHeight, true);
}

void MainWindow::HandleSnapshotToolButtonArrowClicked() const
{
    constexpr int popupWidth = 400;
    constexpr int popupHeight = 50;
    ShowPopupOnToolButtonInteraction(ui->recordSnapshotToolButton, m_snapshotPopup, popupWidth, popupHeight, false);
}

void MainWindow::ShowPopupOnToolButtonInteraction(const QWidget *button, QWidget *popup, const int popupWidth,
                                                  const int popupHeight, const bool centerAlign)
{
    const QPoint buttonTopLeft = button->mapToGlobal(QPoint(0, 0));
    const int buttonWidth = button->width();

    const QPoint globalPos(centerAlign ? buttonTopLeft.x() + (buttonWidth - popupWidth) / 2 : buttonTopLeft.x(),
                           buttonTopLeft.y() + button->height());

    popup->resize(popupWidth, popupHeight);
    popup->move(globalPos);
    popup->show();
}

void MainWindow::HandleViewerImageSliderValueChanged(const int value)
{
    {
        boost::lock_guard lock(m_mutexImageViewer);
        m_viewerSliderQueue.push(value);
    }
    m_viewerQueueCondition.notify_one();
}

void MainWindow::ProcessViewerImageSliderValueChanged(const int value)
{
    std::array<int64_t, B2ND_MAX_DIM> slice_start = {};
    std::array<int64_t, B2ND_MAX_DIM> slice_stop = {};
    std::array<int64_t, B2ND_MAX_DIM> slice_shape = {};
    for (int i = 0; i < this->m_viewerNDArray->ndim; i++)
    {
        slice_start[i] = i == 0 ? value : 0;
        slice_stop[i] = i == 0 ? value + 1 : this->m_viewerNDArray->shape[i];
        slice_shape[i] = slice_stop[i] - slice_start[i];
    }
    const auto buffer_size =
        static_cast<int64_t>(this->m_viewerNDArray->shape[1] * this->m_viewerNDArray->shape[2] * sizeof(uint16_t));
    std::vector<uint16_t> buffer(this->m_viewerNDArray->shape[1] * this->m_viewerNDArray->shape[2]);
    b2nd_get_slice_cbuffer(this->m_viewerNDArray, slice_start.data(), slice_stop.data(), buffer.data(),
                           slice_shape.data(), buffer_size);

    // Get image dimensions, dimensions are transposed compared to what OpenCv expects.
    const auto width = static_cast<int>(slice_shape[1]);
    const auto height = static_cast<int>(slice_shape[2]);
    cv::Mat mat(width, height, CV_16UC1, buffer.data());
    mat /= 4;
    mat.convertTo(mat, CV_8UC1);

    // Indicate that processing is finished.
    const auto viewerQImage = GetQImageFromMatrix(mat, QImage::Format_Grayscale8);
    emit ViewerImageProcessingComplete(viewerQImage);
}

void MainWindow::ViewerWorkerThreadFunc()
{
    // This function is running in a separate thread
    while (true)
    {
        int value = -1; // Default invalid value

        {
            boost::unique_lock lock(m_mutexImageViewer);
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

void MainWindow::UpdateExposure() const
{
    // lock ui elements before updating them
    const QSignalBlocker exposureSpinBoxLock(ui->exposureSpinBox);
    const int exposureMilliseconds = m_cameraInterface.m_camera->GetExposureMs();
    // update the estimated framerate
    const int nrSkipFrames = ui->skipFramesSpinBox->value();
    ui->hzLabel->setText(QString::number(1000.0 / (exposureMilliseconds * (nrSkipFrames + 1)), 'g', 2));
    // set exposure values to bot spinbox and slider
    ui->exposureSpinBox->setValue(exposureMilliseconds);
}

void MainWindow::HandleRecordButtonClicked(const bool clicked)
{
    if (clicked)
    {
        (void)this->LogMessage(" XILENS RECORDING STARTS", LOG_FILE_NAME, true);
        this->LogMessage(QString(" camera selected: %1 %2")
                             .arg(this->m_cameraInterface.m_cameraIdentifier, this->m_cameraInterface.m_cameraSN),
                         LOG_FILE_NAME, true);
        this->m_elapsedTimer.start();
        try
        {
            this->StartRecording();
        }
        catch (const XiLensError &error)
        {
            (void)this->LogMessage(" ERROR WHILE STARTING RECORDING", LOG_FILE_NAME, true);
            (void)this->LogMessage(error.what(), LOG_FILE_NAME, true);
            QMetaObject::invokeMethod(this, "SetRecordButtonIcons", Qt::QueuedConnection, Q_ARG(bool, false));
            return;
        }
        this->HandleElementsWhileRecording(clicked);
    }
    else
    {
        (void)this->LogMessage(" XILENS RECORDING ENDS", LOG_FILE_NAME, true);
        this->StopRecording();
        this->HandleElementsWhileRecording(clicked);
    }
}

void MainWindow::SetRecordButtonIcons(const bool isRecording) const
{
    QIcon recordButtonIcon;
    if (isRecording)
    {
        recordButtonIcon.addFile(":/icon/theme/primary/record_stop.svg", QSize(), QIcon::Normal);
        recordButtonIcon.addFile(":/icon/theme/disabled/record_stop.svg", QSize(), QIcon::Disabled);
        recordButtonIcon.addFile(":/icon/theme/active/record_stop.svg", QSize(), QIcon::Active);
    }
    else
    {
        recordButtonIcon.addFile(":/icon/theme/primary/record.svg", QSize(), QIcon::Normal);
        recordButtonIcon.addFile(":/icon/theme/disabled/record.svg", QSize(), QIcon::Disabled);
        recordButtonIcon.addFile(":/icon/theme/active/record.svg", QSize(), QIcon::Active);
    }
    ui->recordToolButton->setIcon(recordButtonIcon);
}

void MainWindow::HandleElementsWhileRecording(const bool recordingInProgress) const
{
    QMetaObject::invokeMethod(ui->baseFolderButton, "setEnabled", Qt::QueuedConnection,
                              Q_ARG(bool, !recordingInProgress));
    QMetaObject::invokeMethod(ui->fileNameLineEdit, "setEnabled", Qt::QueuedConnection,
                              Q_ARG(bool, !recordingInProgress));
    QMetaObject::invokeMethod(ui->cameraListComboBox, "setEnabled", Qt::QueuedConnection,
                              Q_ARG(bool, !recordingInProgress));
    QMetaObject::invokeMethod(ui->recordWhiteToolButton, "setEnabled", Qt::QueuedConnection,
                              Q_ARG(bool, !recordingInProgress));
    QMetaObject::invokeMethod(ui->recordDarkToolButton, "setEnabled", Qt::QueuedConnection,
                              Q_ARG(bool, !recordingInProgress));
    QMetaObject::invokeMethod(ui->reloadCamerasToolButton, "setEnabled", Qt::QueuedConnection,
                              Q_ARG(bool, !recordingInProgress));
    QMetaObject::invokeMethod(ui->baseFolderLineEdit, "setEnabled", Qt::QueuedConnection,
                              Q_ARG(bool, !recordingInProgress));
    this->SetRecordButtonIcons(recordingInProgress);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (ui->recordToolButton->isChecked())
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
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("NDArrays (*.b2nd)"));
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
    const char *path = strdup(filePath.toUtf8().constData());
    b2nd_open(path, &this->m_viewerNDArray);
    const auto nrImages = static_cast<int>(this->m_viewerNDArray->shape[0] - 1);
    constexpr int defaultIndex = 0;
    // only enable slider when more than one image is in the file
    if (nrImages != 0)
    {
        ui->viewerImageSlider->setEnabled(true);
        ui->viewerImageSlider->setMaximum(nrImages);
    }
    else
    {
        ui->viewerImageSlider->setEnabled(false);
    }
    ui->viewerImageSlider->setValue(defaultIndex);
    this->HandleViewerImageSliderValueChanged(defaultIndex);
}

void MainWindow::WriteLogHeader() const
{
    const auto version =
        QString(" XILENS Version: %1.%2.%3").arg(PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH);
    const auto hash = " git hash: " + QString(GIT_COMMIT);
    (void)this->LogMessage(hash, LOG_FILE_NAME, true);
    (void)this->LogMessage(version, LOG_FILE_NAME, true);
}

QString MainWindow::GetLogFilePath(const QString &logFile) const
{
    return QDir::cleanPath(ui->baseFolderLineEdit->text() + QDir::separator() + logFile);
}

QString MainWindow::LogMessage(const QString &message, const QString &logFile, const bool logTime) const
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
    return ui->normalizeImageToolButton->isChecked();
}

unsigned MainWindow::GetBand() const
{
    return this->m_bandSelectorSliderPopup->value();
}

int MainWindow::GetSaturationMinValue() const
{
    return this->m_saturationSpinBoxesPopup->minValue();
}

int MainWindow::GetSaturationMaxValue() const
{
    return this->m_saturationSpinBoxesPopup->maxValue();
}

unsigned MainWindow::GetBGRNorm() const
{
    return this->m_rgbNormSliderPopup->value();
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
    const QString fullPath = GetFullFilenameStandardFormat(std::move(fileName), ".b2nd", std::move(subFolder));
    try
    {
        this->m_imageContainer.InitializeFile(fullPath.toStdString().c_str());
    }
    catch (const XiLensError &error)
    {
        LOG_XILENS(error) << "Error while initializing image file: " << fullPath.toStdString() << " "
                          << error.toString();
        ShowErrorDialog("Invalid file name.", error.toString().data());
        throw;
    }
}

void MainWindow::RecordImage(const bool ignoreSkipping)
{
    boost::this_thread::interruption_point();
    const XI_IMG image = m_imageContainer.GetCurrentImage();
    boost::lock_guard guard(this->m_mutexImageRecording);
    const int nSkipFrames = ui->skipFramesSpinBox->value();
    if (ImageShouldBeRecorded(nSkipFrames, image.acq_nframe) || ignoreSkipping)
    {
        try
        {
            this->m_imageContainer.m_imageFile->WriteImageData(image, GetCameraTemperature());
            ++m_recordedCount;
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
        ++m_skippedCounter;
    }
}

void MainWindow::RegisterTimeImageRecorded()
{
    const auto now = std::chrono::steady_clock::now();
    m_recordedTimestamps.push_back(now);
    if (m_recordedTimestamps.size() > MAX_FRAMES_TO_COMPUTE_FPS)
    {
        m_recordedTimestamps.pop_front();
    }
}

bool MainWindow::ImageShouldBeRecorded(const int nSkipFrames, const long ImageID)
{
    return nSkipFrames == 0 || ImageID % nSkipFrames == 0;
}

void MainWindow::DisplayRecordCount() const
{
    QMetaObject::invokeMethod(ui->recordedImagesLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(int, static_cast<int>(m_recordedCount.load())));
}

void MainWindow::UpdateTimer()
{
    m_elapsedTime = static_cast<double>(m_elapsedTimer.elapsed()) / 1000.0;
    const int totalSeconds = static_cast<int>(m_elapsedTime);
    const int hours = totalSeconds / 3600;
    const int minutes = (totalSeconds % 3600) / 60;
    const int seconds = totalSeconds % 60;
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
    m_elapsedTimeTextStream << seconds;
    ui->timerLCDNumber->display(m_elapsedTimeText);
}

void MainWindow::StopTimer() const
{
    ui->timerLCDNumber->display(0);
}

void MainWindow::CountImages()
{
    ++m_imageCounter;
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
        QObject::connect(&this->m_imageContainer, &ImageContainer::NewImage, this, &MainWindow::ThreadedRecordImage));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(&this->m_imageContainer, &ImageContainer::NewImage, this, &MainWindow::CountImages));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(&this->m_imageContainer, &ImageContainer::NewImage, this, &MainWindow::UpdateTimer));
    HANDLE_CONNECTION_RESULT(
        QObject::connect(m_updateFPSDisplayTimer, &QTimer::timeout, this, &MainWindow::UpdateFPSLCDDisplay));
    m_updateFPSDisplayTimer->start(UPDATE_RATE_MS_FPS_TIMER);
}

void MainWindow::StopRecording()
{
    HANDLE_CONNECTION_RESULT(QObject::disconnect(&this->m_imageContainer, &ImageContainer::NewImage, this,
                                                 &MainWindow::ThreadedRecordImage));
    HANDLE_CONNECTION_RESULT(
        QObject::disconnect(&this->m_imageContainer, &ImageContainer::NewImage, this, &MainWindow::CountImages));
    HANDLE_CONNECTION_RESULT(
        QObject::disconnect(&this->m_imageContainer, &ImageContainer::NewImage, this, &MainWindow::UpdateTimer));
    HANDLE_CONNECTION_RESULT(
        QObject::disconnect(m_updateFPSDisplayTimer, &QTimer::timeout, this, &MainWindow::UpdateFPSLCDDisplay));
    QMetaObject::invokeMethod(ui->fpsLCDNumber, "display", Qt::QueuedConnection, Q_ARG(QString, ""));
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

QString MainWindow::GetWritingFolder() const
{
    QString writeFolder = GetBaseFolder();
    writeFolder += QDir::separator();
    return QDir::cleanPath(writeFolder);
}

void MainWindow::CreateFolderIfNecessary(const QString &folder)
{
    if (const QDir folderDir(folder); !folderDir.exists())
    {
        if (folderDir.mkpath(folder))
        {
            LOG_XILENS(info) << "Directory created: " << folder.toStdString();
        }
    }
}

QString MainWindow::GetFullFilenameStandardFormat(std::string &&fileName, const std::string &extension,
                                                  std::string &&subFolder) const
{
    QString writingFolder = GetWritingFolder() + QDir::separator() + QString::fromStdString(subFolder);
    if (!writingFolder.endsWith(QDir::separator()))
    {
        writingFolder += QDir::separator();
    }
    CreateFolderIfNecessary(writingFolder);

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

void MainWindow::HandleAutoexposureToolButtonClicked(const bool setAutoexposure) const
{
    this->m_cameraInterface.m_camera->AutoExposure(setAutoexposure);
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
    QMetaObject::invokeMethod(ui->recordToolButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    if (referenceType == "white")
    {
        QMetaObject::invokeMethod(ui->recordDarkToolButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    }
    else if (referenceType == "dark")
    {
        QMetaObject::invokeMethod(ui->recordWhiteToolButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    }

    const QString baseFolder = ui->baseFolderLineEdit->text();
    const QDir dir(baseFolder);
    QStringList nameFilters;
    nameFilters << referenceType + "*";
    QStringList fileNameList = dir.entryList(nameFilters, QDir::Files | QDir::NoDotAndDotDot);
    const QRegularExpression re("^" + referenceType + "(\\d*)\\.[a-zA-Z0-9]+");

    int fileNum = 0;
    for (const QString &fileName : fileNameList)
    {
        if (QRegularExpressionMatch match = re.match(fileName); match.hasMatch())
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
        const int exp_time = m_cameraInterface.m_camera->GetExposureMs();
        const int waitTime = 2 * exp_time;
        WaitMilliseconds(waitTime);
        this->RecordImage(true);
        int progress = static_cast<int>((static_cast<float>(i + 1) / NR_REFERENCE_IMAGES_TO_RECORD) * 100);
        QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
    }
    this->m_imageContainer.CloseFile();
    QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
    QMetaObject::invokeMethod(ui->recordToolButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    if (referenceType == "white")
    {
        QMetaObject::invokeMethod(ui->recordDarkToolButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    }
    else if (referenceType == "dark")
    {
        QMetaObject::invokeMethod(ui->recordWhiteToolButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
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

void MainWindow::HandleViewerFileLineEditReturnPressed()
{
    if (const auto file = QFile(ui->viewerFileLineEdit->text()); file.exists())
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

void MainWindow::HandleReloadViewerFileToolButtonClicked()
{
    ui->reloadViewerFileToolButton->setDown(true);
    QCoreApplication::processEvents();

    HandleViewerFileLineEditReturnPressed();
    ui->reloadViewerFileToolButton->setDown(false);
}

void MainWindow::HandleLogTextLineEditReturnPressed()
{
    QString trigger_message = ui->logTextLineEdit->text();
    // block signals until method ends
    const QSignalBlocker triggerTextBlocker(ui->logTextLineEdit);
    const QSignalBlocker triggersTextEdit(ui->logTextEdit);
    // log message and update member variable for trigger text
    trigger_message.prepend(" ");
    QString timestamp = this->LogMessage(trigger_message, LOG_FILE_NAME, true);
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
    m_fileName = newText;
}

int MainWindow::HandleFileNameSnapshotsLineEditTextEdited(const QString &newText)
{
    if (m_fileName == newText)
    {
        ShowErrorDialog("Invalid file name.", "Snapshot file name cannot be the same as video recording file name.");
        return 1;
    }
    m_snapshotsFileName = newText;
    return 0;
}

void MainWindow::HandleLogTextLineEditTextEdited(const QString &newText) const
{
    UpdateComponentEditedStyle(ui->logTextLineEdit, newText, m_triggerText);
}

void MainWindow::HandleBaseFolderLineEditTextEdited(const QString &newText)
{
    m_baseFolderPath = newText;
}

void MainWindow::HandleViewerFileLineEditTextEdited(const QString &newText) const
{
    UpdateComponentEditedStyle(ui->viewerFileLineEdit, newText, m_viewerFilePath);
}

QString MainWindow::FormatTimeStamp(const QString &timestamp)
{
    const QDateTime dateTime = QDateTime::fromString(timestamp, "yyyyMMdd_HH-mm-ss-zzz");
    QString formattedDate = dateTime.toString("hh:mm:ss AP");
    return formattedDate;
}

/*
 * updates frames per second label in GUI when the number of skipped frames is
 * modified
 */
void MainWindow::HandleSkipFramesSpinBoxValueChanged() const
{
    // spin boxes do not have a returnPressed slot in Qt, which is why the value
    // is always updated upon changes
    const int exposureMilliseconds = m_cameraInterface.m_camera->GetExposureMs();
    const int nSkipFrames = ui->skipFramesSpinBox->value();
    const QSignalBlocker blocker_label(ui->hzLabel);
    ui->hzLabel->setText(QString::number(1000.0 / (exposureMilliseconds * (nSkipFrames + 1)), 'g', 2));
}

void MainWindow::HandleCameraListComboBoxCurrentIndexChanged(const int index)
{
    boost::lock_guard guard(m_mutexImageRecording);
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
        const QString cameraIdentifier = ui->cameraListComboBox->currentText();
        const QString cameraModel = cameraIdentifier.split("@").at(0);
        m_cameraInterface.m_cameraIdentifier = cameraIdentifier;
        if (getCameraMapper().contains(cameraModel))
        {
            const QString cameraType = getCameraMapper().value(cameraModel).cameraType;
            const QString originalCameraIdentifier = m_cameraInterface.m_cameraIdentifier;
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
                LOG_XILENS(error) << "error: " << e.what();
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
            HandleCameraSpecificUiComponents(cameraType, cameraModel);
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

void MainWindow::HandleReloadCamerasToolButtonClicked()
{
    // set button style as pressed down, and process event loop before continuing
    ui->reloadCamerasToolButton->setDown(true);
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
    ui->reloadCamerasToolButton->setDown(false);
}

void MainWindow::HandleSaturationToolButtonArrowClicked() const
{
    constexpr int popupWidth = 100;
    constexpr int popupHeight = 50;
    ShowPopupOnToolButtonInteraction(ui->saturationToolButton, m_saturationSpinBoxesPopup, popupWidth, popupHeight,
                                     true);
}

void MainWindow::HandleSaturationMinValueChanged(const int value) const
{
    m_display->UpdateLut(value, m_saturationSpinBoxesPopup->maxValue(), m_saturationSpinBoxesPopup->getLeftColor(),
                         m_saturationSpinBoxesPopup->getRightColor());
}

void MainWindow::HandleSaturationMaxValueChanged(const int value) const
{
    m_display->UpdateLut(m_saturationSpinBoxesPopup->minValue(), value, m_saturationSpinBoxesPopup->getLeftColor(),
                         m_saturationSpinBoxesPopup->getRightColor());
}

void MainWindow::HandleSaturationDarkColorChanged(const QColor &color) const
{
    m_display->UpdateLut(m_saturationSpinBoxesPopup->minValue(), m_saturationSpinBoxesPopup->maxValue(), color,
                         m_saturationSpinBoxesPopup->getRightColor());
}

void MainWindow::HandleSaturationSaturatedColorChanged(const QColor &color) const
{
    m_display->UpdateLut(m_saturationSpinBoxesPopup->minValue(), m_saturationSpinBoxesPopup->maxValue(),
                         m_saturationSpinBoxesPopup->getLeftColor(), color);
}

void MainWindow::HandleRgbChannelToolButtonClicked() const
{
    constexpr int popupWidth = 200;
    constexpr int popupHeight = 50;
    ShowPopupOnToolButtonInteraction(ui->rgbChannelToolButton, m_rgbChannelSpinBoxesPopup, popupWidth, popupHeight,
                                     true);
}

void MainWindow::HandleAboutActionTriggered()
{
    static QString aboutText =
        "XiLens is an application for camera control and image recording.\n\n"
        "Version: " PROJECT_VERSION_MAJOR "." PROJECT_VERSION_MINOR "." PROJECT_VERSION_PATCH "\n"
        "Build details:\n"
        "\tCommit SHA: " GIT_COMMIT "\n"
        "\tSystem: " CMAKE_SYSTEM "\n"
        "\tProcessor: " CMAKE_SYSTEM_PROCESSOR "\n"
        "\tCompiler: " CMAKE_CXX_COMPILER "\n"
        "\tDate: " BUILD_TIMESTAMP "\n\n"
        "© 2025 Intelligent Medical Systems. All rights reserved.\n\n"
        "Source code can be found in:\nhttps://github.com/IMSY-DKFZ/xilens";
    QMessageBox msgBox(this);
    msgBox.setText(aboutText);
    msgBox.setWindowTitle("About XiLens");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

void MainWindow::HandleDocumentationActionTriggered()
{
    QDesktopServices::openUrl(QUrl("https://xilens.readthedocs.io"));
}

void MainWindow::HandleHowToCiteActionTriggered()
{
    static QString citeText =
        "To learn how to cite this software please check the \"Cite this repository\" option in:\n\n"
        "https://github.com/IMSY-DKFZ/xilens";
    QMessageBox msgBox(this);
    msgBox.setText(citeText);
    msgBox.setWindowTitle("How to cite XiLens");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

void MainWindow::UpdateSaturationPercentageLCDDisplays(const double percentageBelowThreshold,
                                                       const double percentageAboveThreshold) const
{
    QString displayValue = QString::number(percentageAboveThreshold, 'f', 1);
    QMetaObject::invokeMethod(ui->overexposurePercentageLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(QString, displayValue));
    displayValue = QString::number(percentageBelowThreshold, 'f', 1);
    QMetaObject::invokeMethod(ui->underexposurePercentageLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(QString, displayValue));
}

void MainWindow::UpdateFPSLCDDisplay() const
{
    using namespace std::chrono;
    if (this->m_recordedTimestamps.size() < 2)
    {
        return;
    }
    const auto duration =
        std::chrono::duration_cast<milliseconds>(this->m_recordedTimestamps.back() - this->m_recordedTimestamps.front())
            .count();
    const double fps =
        (static_cast<double>(this->m_recordedTimestamps.size()) - 1) * 1000.0 / static_cast<double>(duration);
    const QString displayValue = QString::number(fps, 'f', 1);
    QMetaObject::invokeMethod(ui->fpsLCDNumber, "display", Qt::QueuedConnection, Q_ARG(QString, displayValue));
}

void MainWindow::UpdateImage(QImage image, const QGraphicsView *view, std::unique_ptr<QGraphicsPixmapItem> &pixmapItem,
                             QGraphicsScene *scene)
{
    image = image.scaled(view->width(), view->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (pixmapItem == nullptr)
    {
        pixmapItem.reset(scene->addPixmap(QPixmap::fromImage(image)));
        pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, false);
        pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    }
    else
    {
        pixmapItem->setPixmap(QPixmap::fromImage(image));
    }
}

void MainWindow::UpdateRGBImage(const QImage &image)
{
    UpdateImage(image, ui->rgbImageGraphicsView, this->m_rgbPixMapItem, this->m_rgbScene.get());
}

void MainWindow::UpdateRawImage(const QImage &image)
{
    UpdateImage(image, ui->rawImageGraphicsView, this->m_rawPixMapItem, this->m_rawScene.get());
}

void MainWindow::UpdateRawViewerImage(const QImage &image)
{
    UpdateImage(image, ui->viewerGraphicsView, this->m_rawViewerPixMapItem, this->m_rawViewerScene.get());
}

void MainWindow::SetGraphicsViewScene() const
{
    ui->rgbImageGraphicsView->setScene(this->m_rgbScene.get());
    ui->rawImageGraphicsView->setScene(this->m_rawScene.get());
    ui->viewerGraphicsView->setScene(this->m_rawViewerScene.get());
}

bool MainWindow::IsSaturationButtonChecked() const
{
    return ui->saturationToolButton->isChecked();
}

void MainWindow::SetRecordedCount(const int count)
{
    m_recordedCount = count;
}

void MainWindow::HandleCameraSpecificUiComponents(const QString &cameraType, const QString &cameraModel) const
{
    const bool enableSpectralComponents = cameraType == CAMERA_TYPE_SPECTRAL;
    QMetaObject::invokeMethod(ui->bandSelectorToolButton, "setEnabled", Qt::QueuedConnection,
                              Q_ARG(bool, enableSpectralComponents));
    QMetaObject::invokeMethod(ui->rgbChannelToolButton, "setEnabled", Qt::QueuedConnection,
                              Q_ARG(bool, enableSpectralComponents));
    QMetaObject::invokeMethod(this->m_bandSelectorSliderPopup, "setEnabled", Q_ARG(bool, enableSpectralComponents));

    // get default rgb camera channels
    if (enableSpectralComponents)
    {
        if (!getCameraMapper().contains(cameraModel))
        {
            LOG_XILENS(error) << "Could not find camera model in Mapper: " << cameraModel.toStdString();
            throw std::runtime_error("Could not find camera in Mapper");
        }

        // Update RGB channels
        const auto bgrChannels = getCameraMapper().value(cameraModel).bgrChannels;
        if (bgrChannels.empty())
        {
            LOG_XILENS(error) << "Empty BGR channel indices";
            throw std::runtime_error("Empty RGB channel indices");
        }
        this->m_rgbChannelSpinBoxesPopup->UpdateRgb(bgrChannels.at(2), bgrChannels.at(1), bgrChannels.at(0));
        QMetaObject::invokeMethod(this->m_rgbChannelSpinBoxesPopup, "UpdateRgb", Qt::QueuedConnection,
                                  Q_ARG(const int, bgrChannels.at(2)), Q_ARG(const int, bgrChannels.at(1)),
                                  Q_ARG(const int, bgrChannels.at(0)));
        this->m_display->UpdateBGRChannels(bgrChannels);

        // Update maximum channels in all components
        auto mosaicShape = getCameraMapper().value(cameraModel).mosaicShape;
        const int nChannels = std::accumulate(mosaicShape.begin(), mosaicShape.end(), 1, std::multiplies<>());
        this->m_bandSelectorSliderPopup->SetMaximum(nChannels);
        this->m_rgbChannelSpinBoxesPopup->m_spinBox1->setMaximum(nChannels);
        this->m_rgbChannelSpinBoxesPopup->m_spinBox2->setMaximum(nChannels);
        this->m_rgbChannelSpinBoxesPopup->m_spinBox3->setMaximum(nChannels);
    }
}
