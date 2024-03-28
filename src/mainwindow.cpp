/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
#include <iostream>
#include <string>
#include <utility>

#include <opencv2/core/core.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QDateTime>
#include <QTextStream>

#if CV_VERSION_MAJOR == 3
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/core/types_c.h>
#endif

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include "imageContainer.h"
#include "displayFunctional.h"
#include "displayRaw.h"
#include "constants.h"
#include "logger.h"
#include "xiAPIWrapper.h"


/**
* @class MainWindow
* @brief The MainWindow class represents the main window of the application.
*
* This class inherits from QMainWindow and is responsible for initializing the user interface.
* It also manages the IO service, work, and other variables related to image recording and testing.
*/
MainWindow::MainWindow(QWidget *parent, std::shared_ptr<XiAPIWrapper> xiAPIWrapper) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        m_io_service(), m_work(m_io_service),
        m_temperature_io_service(), m_temperature_work(new boost::asio::io_service::work(m_temperature_io_service)),
        m_cameraInterface(),
        m_recordedCount(0),
        m_testMode(g_commandLineArguments.test_mode),
        m_imageCounter(0),
        m_skippedCounter(0),
        m_elapsedTimeTextStream(&m_elapsedTimeText),
        m_elapsedTime(0) {
    this->m_xiAPIWrapper = xiAPIWrapper == nullptr ? this->m_xiAPIWrapper : xiAPIWrapper;
    m_cameraInterface.Initialize(this->m_xiAPIWrapper);
    m_imageContainer.Initialize(this->m_xiAPIWrapper);
    ui->setupUi(this);

    // Display needs to be instantiated before changing camera list because calling setCurrentIndex on the list.
    m_display = new DisplayerFunctional(this);

    // populate available cameras
    QStringList cameraList = m_cameraInterface.GetAvailableCameraModels();
    ui->cameraListComboBox->addItem("select camera to enable UI...");
    ui->cameraListComboBox->addItems(cameraList);
    ui->cameraListComboBox->setCurrentIndex(0);

    // hack until we implement proper resource management
    QPixmap pix(":/resources/jet_photo.jpg");
    ui->jet_sao2->setPixmap(pix);
    ui->jet_vhb->setPixmap(pix);

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
    UpdateVhbSao2Validators();

    // create thread pool
    for (int i = 0; i < 2; i++) // put 2 threads in thread pool
    {
        m_threadpool.create_thread([&] { return m_io_service.run(); });
    }

    LOG_SUSICAM(info) << "test mode (recording everything to same file) is set to: " << m_testMode << "\n";

    EnableUi(false);
}


/**
 * @brief Starts image acquisition for a specific camera.
 *
 * This function initiates the image acquisition process for a specified camera. this is done on a pool of threads.
 * The pool of threads need to be joined when not needed in order to avoid dangling threads.
 * This method also connects the incoming images from the camera to the Displayer that handles the displaying process
 * on screen. Another connection is established between incoming images and UpdateMinMaxPixelValues.
 *
 * @param camera_identifier The identifier of the camera to start the image acquisition for.
 */
void MainWindow::StartImageAcquisition(QString camera_identifier) {
    try {
        this->m_display->StartDisplayer();
        m_cameraInterface.StartAcquisition(std::move(camera_identifier));
        this->StartPollingThread();
        this->StartTemperatureThread();

        /***************************************/
        // setup connections to displaying etc.
        /***************************************/
        // when a new image arrives, display it
        QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display);
    }
    catch (std::runtime_error &error) {
        LOG_SUSICAM(warning) << "could not start camera, got error " << error.what();
        throw std::runtime_error(error.what());
    }
}


/**
 * @brief Stops the image acquisition.
 *
 * This function is used to stop the acquisition of images in the MainWindow class.
 * It stops any ongoing image acquisition and performs the necessary clean-up actions.
 * After calling this function, the image acquisition process will be completely stopped
 * and no further images will be acquired.
 * The pool of threads in charge of collecting the images from the camera is stopped as well as the thread in charge of
 * recording the camera temperature.
 * The corresponding signals and slots are also disconnected from NewImage
 */
void MainWindow::StopImageAcquisition() {
    this->m_display->StopDisplayer();
    this->StopPollingThread();
    this->StopTemperatureThread();
    m_cameraInterface.StopAcquisition();
    // disconnect slots for image display
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display);
    LOG_SUSICAM(info) << "Stopped Image Acquisition";
}


/**
 * @brief Disable or enable widgets in a given layout.
 *
 * This function can be used to disable or enable all widgets within a layout
 * by setting their disabled state. This is done iteratively, so that widgets in nested layouts are also disabled or
 * re-enabled
 *
 * @param layout The layout object containing the widgets.
 *
 * @param enable A boolean value indicating whether to enable or disable the widgets.
 *               true to enable, false to disable.
 */
void MainWindow::EnableWidgetsInLayout(QLayout *layout, bool enable) {
    for (int i = 0; i < layout->count(); ++i) {
        QLayout *subLayout = layout->itemAt(i)->layout();
        QWidget *widget = layout->itemAt(i)->widget();
        if (widget) {
            widget->setEnabled(enable);
        }
        if (subLayout) {
            EnableWidgetsInLayout(subLayout, enable);
        }
    }
}


/**
 * @brief Enables or disables the user interface of the main window.
 *
 * This function sets the enabled state of all the user interface components
 * in the main window, such as buttons, menus, and text fields. When called with
 * the parameter 'enable' set to true, all the components will be enabled, and
 * when called with 'enable' set to false, all the components will be disabled.
 *
 * @param enable True to enable the user interface components, false to disable.
 */
void MainWindow::EnableUi(bool enable) {
    QLayout *layout = ui->mainUiVerticalLayout->layout();
    EnableWidgetsInLayout(layout, enable);
    ui->exposureSlider->setEnabled(enable);
    ui->logTextLineEdit->setEnabled(enable);
    QLayout *layoutExtras = ui->extrasVerticalLayout->layout();
    EnableWidgetsInLayout(layoutExtras, enable);
    QLayout *functionalLayout = ui->functionalParametersColoringVerticalLayout->layout();
    if(m_cameraInterface.m_cameraType != CAMERA_TYPE_SPECTRAL){
        EnableWidgetsInLayout(functionalLayout, false);
    } else{
        EnableWidgetsInLayout(functionalLayout, enable);
    }
}


/**
 * @class MainWindow
 *
 * @brief Virtual method in charge of displaying the images.
 *
 * Displays an image at most every 35 milliseconds. The images are queried from the container.
 */
void MainWindow::Display() {
    static boost::posix_time::ptime last = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();

    // display new images with at most every 35ms
    bool display_new = (now - last).total_milliseconds() > 35;

    if (display_new) {
        // first get the pointer to the image to display
        XI_IMG image = m_imageContainer.GetCurrentImage();
        this->m_display->Display(image);
        last = now;
    }

}


/**
 * \brief Update the validators for Vhb and Sao2 fields in the MainWindow class.
 *
 * This function updates the validators for the Vhb and Sao2 fields in the MainWindow class.
 * The validators ensure that the user input for these fields is in a valid format.
 */
void MainWindow::UpdateVhbSao2Validators() {
    ui->minVhbLineEdit->setValidator(new QIntValidator(MIN_VHB, ui->maxVhbLineEdit->text().toInt(), this));
    ui->maxVhbLineEdit->setValidator(new QIntValidator(ui->minVhbLineEdit->text().toInt(), MAX_VHB, this));
    ui->minSao2LineEdit->setValidator(new QIntValidator(MIN_SAO2, ui->maxSao2LineEdit->text().toInt(), this));
    ui->maxSao2LineEdit->setValidator(new QIntValidator(ui->minSao2LineEdit->text().toInt(), MAX_SAO2, this));
}


/**
 * @brief Destructor for MainWindow class.
 *
 * Cleans up resources used by the MainWindow object.
 * This destructor is automatically called when the object is destroyed.
 */
MainWindow::~MainWindow() {
    m_io_service.stop();
    m_temperature_io_service.stop();
    m_threadpool.join_all();

    this->StopTemperatureThread();
    this->StopSnapshotsThread();
    this->StopReferenceRecordingThread();

    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display);

    delete ui;
}


/**
 * @class MainWindow
 * @brief Collects and stores the specified number of images in the base folder specified through the GUI
 *
 * The snapshot button is re-styled when clicked, and the progress bar is updated in the GUI during the process of recording the
 * images. The button style is restored after the data recording has completed.
 */
void MainWindow::RecordSnapshots() {
    static QString recordButtonOriginalColour = ui->recordButton->styleSheet();
    int nr_images = ui->nSnapshotsSpinBox->value();
    QMetaObject::invokeMethod(ui->nSnapshotsSpinBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    QMetaObject::invokeMethod(ui->filePrefixExtrasLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    QMetaObject::invokeMethod(ui->subFolderExtrasLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));

    std::string name = ui->filePrefixExtrasLineEdit->text().toUtf8().constData();
    std::string snapshotSubFolderName = ui->subFolderExtrasLineEdit->text().toUtf8().constData();

    for (int i = 0; i < nr_images; i++) {
        int exp_time = m_cameraInterface.m_camera->GetExposureMs();
        int waitTime = 2 * exp_time;
        wait(waitTime);
        std::stringstream name_i;
        name_i << name << "_" << i;
        this->RecordImage(snapshotSubFolderName, name_i.str(), true);
        int progress = static_cast<int>((static_cast<float>(i + 1) / nr_images) * 100);
        QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
    }
    QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
    QMetaObject::invokeMethod(ui->nSnapshotsSpinBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    QMetaObject::invokeMethod(ui->filePrefixExtrasLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    QMetaObject::invokeMethod(ui->subFolderExtrasLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
}


/**
 * @brief Slot for the snapshotButton clicked signal.
 *
 * This function is called when the snapshotButton is clicked by the user.
 * It handles the logic for capturing a snapshot of the current state of the window. It launches a thread that takes
 * care of the data recording.
 *
 * @return void
 */
void MainWindow::on_snapshotButton_clicked() {
    m_snapshotsThread = boost::thread(&MainWindow::RecordSnapshots, this);
}


/**
 * @brief Records the temperature of the camera.
 *
 * This function records the temperature of the camera and logs it to a file in the base folder specified in the GUI.
 * It uses the camera API to get the temperature value and prints it to the console output.
 * This function should be called periodically to monitor the camera's temperature.
 */
void MainWindow::LogCameraTemperature() {
    QString message;
    m_cameraInterface.m_camera->family->get()->UpdateCameraTemperature();
    auto cameraTemperature = m_cameraInterface.m_camera->family->get()->m_cameraTemperature;
    for (const QString &key: cameraTemperature.keys()) {
        float temp = m_cameraInterface.m_cameraTemperature.value(key);
        message = QString("\t%1\t%2\t%3\t%4").arg(key).arg(temp).arg(this->m_cameraInterface.m_cameraModel).arg(this->m_cameraInterface.m_cameraSN);
        this->LogMessage(message, TEMP_LOG_FILE_NAME, true);
    }
}


/**
 * Displays the camera temperature in an LCD display on the GUI
 */
void MainWindow::DisplayCameraTemperature() {
    double temp = m_cameraInterface.m_camera->family->get()->m_cameraTemperature.value(SENSOR_BOARD_TEMP);
    QMetaObject::invokeMethod(ui->temperatureLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(double, temp));
}


/**
 * @brief Starts a thread to schedule temperature.
 *
 * This function starts a thread which executes the temperature scheduling logic.
 * The temperature scheduling logic periodically checks the current temperature. The temperature is only logged
 * periodically according to the TEMP_LOG_INTERVAL variable
 */
void MainWindow::ScheduleTemperatureThread() {
    m_temperature_work = std::make_unique<boost::asio::io_service::work>(m_temperature_io_service);
    m_temperatureThreadTimer = std::make_shared<boost::asio::steady_timer>(m_temperature_io_service);
    m_temperatureThreadTimer->expires_after(std::chrono::seconds(TEMP_LOG_INTERVAL));
    m_temperatureThreadTimer->async_wait(
            boost::bind(&MainWindow::HandleTemperatureTimer, this, boost::asio::placeholders::error));
}


/**
 * \brief Handle the timer expiration event
 *
 * This function is called when the timer expires. It is responsible for handling the timer expiration event.
 *
 * \param timer Pointer to the boost::asio::steady_timer object representing the timer.
 * \param error The error code associated with the timer expiration event, if any.
 */
void MainWindow::HandleTemperatureTimer(const boost::system::error_code &error) {
    if (error == boost::asio::error::operation_aborted) {
        LOG_SUSICAM(warning) << "Timer cancelled. Error: " << error;
        return;
    }

    this->LogCameraTemperature();
    this->DisplayCameraTemperature();

    // Reset timer
    m_temperatureThreadTimer->expires_after(std::chrono::seconds(TEMP_LOG_INTERVAL));
    m_temperatureThreadTimer->async_wait(boost::bind(&MainWindow::HandleTemperatureTimer, this, boost::asio::placeholders::error));
}


/**
 * @brief this method starts a new thread for temperature monitoring.
 *
 * This method creates a new thread to monitor the temperature by periodically calling the
 * `ReadTemperature()` method. The temperature is read and stored in a member variable for
 * further processing.
 *
 * \see StopTemperatureThread()
 */
void MainWindow::StartTemperatureThread() {
    if (m_temperatureThread.joinable()) {
        StopTemperatureThread();
    }
    QString log_filename = QDir::cleanPath(ui->baseFolderLineEdit->text() + QDir::separator() + TEMP_LOG_FILE_NAME);
    QFile file(log_filename);
    QFileInfo fileInfo(file);
    if (fileInfo.size() == 0) {
        this->LogMessage("time\tsensor_location\ttemperature\tcamera_model\tcamera_sn", TEMP_LOG_FILE_NAME, false);
    }
    file.close();
    m_temperatureThread = boost::thread([&]() {
        ScheduleTemperatureThread();
        m_temperature_io_service.reset();
        m_temperature_io_service.run();
    });
    LOG_SUSICAM(info) << "Started temperature thread";
}


/**
 * \brief Stops the temperature thread.
 *
 * This function stops the thread responsible for temperature readings. It
 * sends a stop signal to the thread and waits for it to finish before returning.
 *
 * \see StartTemperatureThread()
 */
void MainWindow::StopTemperatureThread() {
    if (m_temperatureThread.joinable()) {
        if (m_temperatureThreadTimer) {
            m_temperatureThreadTimer->cancel();
            m_temperatureThreadTimer = nullptr;
        }
        m_temperature_work.reset();
        m_temperatureThread.join();
        this->ui->temperatureLCDNumber->display(0);
        LOG_SUSICAM(info) << "Stopped temperature thread";
    }
}


/**
 * @brief Stop the snapshots thread.
 *
 * This function is used to stop the thread responsible for capturing snapshots.
 * It sends a signal to the thread to terminate its execution and waits for it to finish.
 *
 * @note This function should be called when the application is being closed or when the snapshots thread is no longer needed.
 */
void MainWindow::StopSnapshotsThread(){
    if (m_snapshotsThread.joinable()){
        m_snapshotsThread.join();
    }
}

/**
 * This function joins the thread responsible for recording the white and dark reference images
 */
void MainWindow::StopReferenceRecordingThread(){
    if (m_referenceRecordingThread.joinable()){
        m_referenceRecordingThread.join();
    }
}

/**
 * @brief Handles the value changed event of the exposure slider.
 *
 * This function is called when the value of the exposure slider is changed.
 * It updates the exposure value the camera being used.
 *
 * @param value The new value of the exposure slider.
 */
void MainWindow::on_exposureSlider_valueChanged(int value) {
    m_cameraInterface.m_camera->SetExposureMs(value);
    UpdateExposure();
}


/**
 * @brief Update the exposure value of the camera.
 *
 * This function updates the exposure value of the camera, and updates the labels in the GUI as well as the estimated
 * framerate.
 *
 */
void MainWindow::UpdateExposure() {
    int exp_ms = m_cameraInterface.m_camera->GetExposureMs();
    int n_skip_frames = ui->skipFramesSpinBox->value();
    ui->exposureLineEdit->setText(QString::number((int) exp_ms));
    ui->hzLabel->setText(QString::number((double) (1000.0 / (exp_ms * (n_skip_frames + 1))), 'g', 2));

    // need to block the signals to make sure the event is not immediately
    // thrown back to label_exp.
    // could be done with a QSignalBlocker from Qt5.3 on for exception safe treatment.
    // see: http://doc.qt.io/qt-5/qsignalblocker.html
    const QSignalBlocker blocker_slider(ui->exposureSlider);
    ui->exposureSlider->setValue(exp_ms);
}


/**
 * @brief slot triggered when the return key is pressed in the "label_exp" QLabel.
 * This method sets the camera exposure time and restores the GUI element style to its original style.
 */
void MainWindow::on_exposureLineEdit_returnPressed() {
    m_label_exp = ui->exposureLineEdit->text();
    m_cameraInterface.m_camera->SetExposureMs(m_label_exp.toInt());
    UpdateExposure();
    RestoreLineEditStyle(ui->exposureLineEdit);
}


/**
 * @brief Slot triggered when the text in the label_exp QLineEdit is edited.
 *
 * This method updates the style of the GUI component to indicate that the value has changed
 *
 * @param arg1 The new text entered in the label_exp QLineEdit.
 */
void MainWindow::on_exposureLineEdit_textEdited(const QString &arg1) {
    updateComponentEditedStyle(ui->exposureLineEdit, arg1, m_label_exp);
}


/**
 * @brief Handles the "recordButton" clicked event.
 *
 * This method calls StartRecording() to initiate the recording of the images.
 * This slot function sets the style of the record button to indicate if the recordings are running or if they are not.
 * A message is also logged into the log file located at the base folder to indicate that recordings started or ended.
 *
 * @param clicked The clicked state of the "recordButton".
 * \see StartRecording()
 */
void MainWindow::on_recordButton_clicked(bool clicked) {
    static QString original_colour;
    static QString original_button_text;

    if (clicked) {
        this->LogMessage(" SUSICAM RECORDING STARTS", LOG_FILE_NAME, true);
        this->LogMessage(QString(" camera selected: %1 %2").arg(this->m_cameraInterface.m_cameraModel, this->m_cameraInterface.m_cameraSN), LOG_FILE_NAME, true);
        this->m_elapsedTimer.start();
        this->StartRecording();
        this->HandleElementsWhileRecording(clicked);
        original_colour = ui->recordButton->styleSheet();
        original_button_text = ui->recordButton->text();
        // button text seems to be an object property and cannot be changed by using QMetaObject::invokeMethod
        ui->recordButton->setText(" Stop recording");
    } else {
        this->LogMessage(" SUSICAM RECORDING ENDS", LOG_FILE_NAME, true);
        this->StopRecording();
        this->HandleElementsWhileRecording(clicked);
        ui->recordButton->setText(original_button_text);
    }
}


/**
 * Enables and disables elements of the GUI that should not me modified while recordings are in progress
 *
 * @param recordingInProgress
 */
void MainWindow::HandleElementsWhileRecording(bool recordingInProgress){
    if (recordingInProgress) {
        QMetaObject::invokeMethod(ui->baseFolderButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->subFolderLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->filePrefixLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->cameraListComboBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->whiteBalanceButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->darkCorrectionButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    } else {
        QMetaObject::invokeMethod(ui->baseFolderButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->subFolderLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->filePrefixLineEdit, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->cameraListComboBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->whiteBalanceButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->darkCorrectionButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    }
}


/**
 * @brief Event handler for the close event of the main window.
 *
 * This method is called when the user attempts to close the main window either by clicking the close button
 * or using the system shortcut. It is responsible for handling any necessary cleanup or actions before
 * the application closes.
 *
 * @param event A pointer to the event object representing the close event.
 */
void MainWindow::closeEvent(QCloseEvent *event) {
    this->StopPollingThread();
    QMainWindow::closeEvent(event);
}


/**
 * @brief Slot function triggered when the "Choose Folder" button is clicked in the MainWindow.
 *
 * This function is responsible for handling the event when the user clicks the "Choose Folder" button
 * in the MainWindow. It opens a file dialog to allow the user to select a folder and performs necessary
 * operations based on the selected folder.
 */
void MainWindow::on_baseFolderButton_clicked() {
    bool isValid = false;
    this->StopTemperatureThread();
    while (!isValid) {
        QString baseFolderPath = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                                   "", QFileDialog::ShowDirsOnly
                                                                       | QFileDialog::DontResolveSymlinks);

        if (QDir(baseFolderPath).exists()) {
            isValid = true;
            if (!baseFolderPath.isEmpty()) {
                this->SetBaseFolder(baseFolderPath);
                ui->baseFolderLineEdit->clear();
                ui->baseFolderLineEdit->insert(this->GetBaseFolder());
                this->WriteLogHeader();
            }
        }
    }
    this->StartTemperatureThread();
}


/**
 * @brief Writes the log header to the log file.
 *
 * This function writes the header in the log file, which includes information such as the date and time when
 * the log was created, and any other relevant information to describe the log file format.
 */
void MainWindow::WriteLogHeader() {
    this->LogMessage(" git hash: " + QString::fromLatin1(libfiveGitRevision()), LOG_FILE_NAME, true);
    this->LogMessage(" git branch: " + QString::fromLatin1(libfiveGitBranch()), LOG_FILE_NAME, true);
    this->LogMessage(" git tags matching hash: " + QString::fromLatin1(libfiveGitVersion()), LOG_FILE_NAME, true);
}


/**
 * @brief Logs a message to a file with optional timestamp.
 *
 * This function appends the given message to a log file. The logFile parameter specifies the path of the log file.
 * If logTime is set to true, the current timestamp will be added to the log entry. Otherwise, only the message will be logged.
 *
 * @param message The message to be logged.
 * @param logFile The path of the log file.
 * @param logTime Specifies whether to log the timestamp along with the message.
 *
 * @note If the log file does not exist, it will be created. If it already exists, the message will be appended to it.
 * @note The function does not handle exceptions or errors when writing to the log file. It assumes the file can be written to successfully.
 */
QString MainWindow::LogMessage(QString message, QString logFile, bool logTime) {
    QString timestamp;
    QString curr_time = (QTime::currentTime()).toString("hh-mm-ss-zzz");
    QString date = (QDate::currentDate()).toString("yyyyMMdd_");
    timestamp = date + curr_time;
    QString log_filename = QDir::cleanPath(ui->baseFolderLineEdit->text() + QDir::separator() + logFile);
    QFile file(log_filename);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    if (logTime) {
        stream << timestamp;
    }
    stream << message << "\n";
    file.close();
    return timestamp;
}


/**
 * @brief Slot function called when the return key is pressed in the topFolderName QLineEdit.
 * It handles the logic when the user inputs a new folder name and presses the return key. Member variable is updated
 * and the original style of the GUI element restored.
 */
void MainWindow::on_subFolderLineEdit_returnPressed() {
    m_subFolder = ui->subFolderLineEdit->text();
    RestoreLineEditStyle(ui->subFolderLineEdit);
}


/**
 * \brief Slot function called when the user presses enter in the recPrefixlineEdit field.
 *
 * This function is a slot connected to the returnPressed() signal of the recPrefixlineEdit widget
 * in the MainWindow class. It is called when the user presses the enter key while the recPrefixlineEdit
 * field has focus.
 */
void MainWindow::on_filePrefixLineEdit_returnPressed() {
    m_recPrefixlineEdit = ui->filePrefixLineEdit->text();
    RestoreLineEditStyle(ui->filePrefixLineEdit);
}


/**
 * @brief Retrieves the normalization checkbox state from the GUI.
 */
bool MainWindow::GetNormalize() const {
    return this->ui->normalizeCheckbox->isChecked();
}


/**
 * @brief retrieves the state of the Scale Parameters checkbox from the GUI
 */
bool MainWindow::DoParamterScaling() const {
    return this->ui->scaleParamtersCheckBox->isChecked();
}


/**
 * \brief returns the upper and lower bounds of VHB values.
 *
 */
cv::Range MainWindow::GetUpperLowerBoundsVhb() const {
    uchar lower, upper;
    lower = this->ui->minVhbLineEdit->text().toInt();
    upper = this->ui->maxVhbLineEdit->text().toInt();
    return {lower, upper};
}


/**
 * \brief returns the upper and lower bounds of SaO2 values.
 *
 */
cv::Range MainWindow::GetUpperLowerBoundsSao2() const {
    uchar lower, upper;
    lower = this->ui->minSao2LineEdit->text().toInt();
    upper = this->ui->maxSao2LineEdit->text().toInt();
    return cv::Range(lower, upper);
}


/**
 * \brief returns the index of the band specified by the slider in the GUI
 *
 */
unsigned MainWindow::GetBand() const {
    return this->ui->bandSlider->value();
}


/**
 * \brief retrieves the state of the RGB normalization checkbox in the GUI
 *
 */
unsigned MainWindow::GetBGRNorm() const {
    return this->ui->rgbNormSlider->value();
}


/**
 * \brief Sets the value of the member variable storing the value of the base folder specified in the GUI
 *
 */
bool MainWindow::SetBaseFolder(QString baseFolderPath) {
    if (QDir(baseFolderPath).exists()) {
        m_baseFolderLoc = baseFolderPath;
        return true;
    } else {
        return false;
    }
}


/**
 * \brief returns the value of the member variable that stores the path to the base folder where images are recorded
 *
 */
QString MainWindow::GetBaseFolder() const {
    return m_baseFolderLoc;
}


/**
 * \brief Executes image recording in a separate thread.
 */
void MainWindow::ThreadedRecordImage() {
    m_io_service.post([this] { RecordImage(); });
}


/**
 * @brief Records an image
 *
 * This function is responsible for recording an image in the MainWindow class.
 * It performs the necessary operations to save the current image displayed in the
 * application to a file or any other desired location.
 * The LCD displaying the number of recorded images is also updated.
 *
 * @param subFolder folder inside base folder where image will be recorded
 * @param filePrefix file prefix used to name file. If not provided, the prefix specified in the GUI is used
 * @param ignoreSkipping ignores the number of frames to skip and stores the image anyways
 */
void MainWindow::RecordImage(std::string subFolder, std::string filePrefix, bool ignoreSkipping) {
    XI_IMG image = m_imageContainer.GetCurrentImage();
    static long lastImageID = image.acq_nframe;
    int nSkipFrames = ui->skipFramesSpinBox->value();
    if (this->ImageShouldBeRecorded(nSkipFrames, image.acq_nframe) || ignoreSkipping) {
        if (filePrefix.empty()){
            filePrefix = m_recPrefixlineEdit.toUtf8().constData();
        }
        if (subFolder.empty()){
            subFolder = m_subFolder.toStdString();
        }
        QString fullPath = GetFullFilenameStandardFormat(std::move(filePrefix), image.acq_nframe, ".dat", std::move(subFolder));
        try {
            FileImage f(fullPath.toStdString().c_str(), "wb");
            f.write(image);
            m_recordedCount++;
        } catch (const std::runtime_error &e) {
            LOG_SUSICAM(error) << "Error: %s\n" << e.what();
        }
        this->DisplayRecordCount();
    } else {
        m_skippedCounter++;
    }
    lastImageID = image.acq_nframe;
}


/**
 * Indicates if an image should be recorded to file or not
 *
 * @param nSkipFrames number of frames to skip
 * @param ImageID unique image ID
 * @return
 */
bool MainWindow::ImageShouldBeRecorded(int nSkipFrames, long ImageID) {
    return (nSkipFrames == 0) || (ImageID % nSkipFrames == 0);
}

/**
 * Displays in the GUI the number of recorded images
 */
void MainWindow::DisplayRecordCount() {
    QMetaObject::invokeMethod(ui->recordedImagesLCDNumber, "display", Qt::QueuedConnection, Q_ARG(int, m_recordedCount));
}


/**
 * @brief Updates the timer in the main window.
 *
 * This function is responsible for updating the timer displayed in the main window.
 * It should be called periodically to ensure the timer is always up-to-date.
 */
void MainWindow::updateTimer() {
    m_elapsedTime = static_cast<float>(m_elapsedTimer.elapsed()) / 1000.0;
    int totalSeconds = static_cast<int>(m_elapsedTime);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    m_elapsedTimeText.clear();
    m_elapsedTimeTextStream.seek(0);
    m_elapsedTimeTextStream.setFieldWidth(2);  // Set field width to 2 or numbers and 1 for separators
    m_elapsedTimeTextStream.setPadChar('0');   // Zero-fill numbers
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


/**
 * \brief Stops the timer in the MainWindow class.
 */
void MainWindow::stopTimer() {
    ui->timerLCDNumber->display(0);
}


/**
 * @brief Adds 1 to the member variable that stores the number of images counter.
 */
void MainWindow::CountImages() {
    m_imageCounter++;
}


/**
 * @brief Starts the recording process.
 *
 * This function is responsible for starting the recording process in the MainWindow class.
 * It makes the appropriate connections between slots: ThreadedRecordImage, CountImages & updateTimer with the NewImage
 * signal.
 *
 * @note Make sure to call StopRecording() to stop the recording process.
 */
void MainWindow::StartRecording() {
    QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::ThreadedRecordImage);
    QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::CountImages);
    QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::updateTimer);
}


/**
 * @brief Stops the current recording process.
 *
 * This method disconnects the connections made by StartRecording and stops the timer as well as logs to console the
 * estimated number of recorded images.
 */

void MainWindow::StopRecording() {
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::ThreadedRecordImage);
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::CountImages);
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::updateTimer);
    this->stopTimer();
    LOG_SUSICAM(info) << "Total of frames recorded: " << m_recordedCount;
    LOG_SUSICAM(info) << "Total of frames dropped : " << m_imageCounter - m_recordedCount;
    LOG_SUSICAM(info) << "Estimate for frames skipped: " << m_skippedCounter;
}


/**
 * Generates the base folder where all recordings are stored. The base folder is defined in the GUI and stored in a
 * member variable
 *
 * @return base folder path
 */
QString MainWindow::GetWritingFolder() {
    QString writeFolder = GetBaseFolder();
    writeFolder += QDir::separator();
    return QDir::cleanPath(writeFolder);
}


/**
 * Checks if folder path exists, if not then it is created
 *
 * @param folder: path to folder to be created
 */
void MainWindow::CreateFolderIfNecessary(QString folder) {
    QDir folderDir(folder);

    if (!folderDir.exists()) {
        if (folderDir.mkpath(folder)) {
            LOG_SUSICAM(info) << "Directory created: " << folder.toStdString();
        }
    }
}


/**
 * Generates a filePrefix where an image will be stored. The filePrefix is composed of prefix + date + time + image number +
 * extension. The date and time are queried each time this function is called.
 *
 * @param filePrefix the prefix used for the filePrefix, date, time and extension are added to this
 * @param frameNumber number of the image that is going to be stored on this file. Used as an identified
 * @param extension file extension to use for the filePrefix
 * @param subFolder sub folder inside the base folder where the file should reside
 * @return full file path, includes the base folder where the file should reside
 */
QString MainWindow::GetFullFilenameStandardFormat(std::string&& filePrefix, long frameNumber, std::string extension,
                                                  std::string&& subFolder) {

    QString writingFolder = GetWritingFolder() + QDir::separator() + QString::fromStdString(subFolder);
    if (!writingFolder.endsWith(QDir::separator())){
        writingFolder += QDir::separator();
    }
    this->CreateFolderIfNecessary(writingFolder);

    QString fileName;
    if (!m_testMode) {
        int exp_time = m_cameraInterface.m_camera->GetExposureMs();
        QString exp_time_str("exp" + QString::number(exp_time) + "ms");
        QString curr_time = (QTime::currentTime()).toString("hh-mm-ss-zzz");
        QString date = (QDate::currentDate()).toString("yyyyMMdd_");
        fileName = QString::fromStdString(filePrefix) + "_" + date + curr_time + "_" + exp_time_str + "_" +
                   QString::number(frameNumber);
    } else {
        fileName = QString("test");
    }
    fileName += QString::fromStdString(extension);

    return QDir::cleanPath(writingFolder + fileName);
}


/**
 * Starts the thread of the image container and starts polling the images on the image container
 */
void MainWindow::StartPollingThread() {
    m_imageContainer.StartPolling();
    m_imageContainerThread = boost::thread(&ImageContainer::PollImage, &m_imageContainer, &m_cameraInterface.m_cameraHandle,5);
}


/**
 * Stops the thread in charge of the image container and stops polling the images on the image container
 */
void MainWindow::StopPollingThread() {
    m_imageContainer.StopPolling();
    m_imageContainerThread.interrupt();
    m_imageContainerThread.join();
}


/**
 * Sets the camera exposure time to automatic. The process of setting the exposure time is managed internally by the
 * camera.
 *
 * @param setAutoexposure whether the exposure should be set to automatic management by the camera
 */
void MainWindow::on_autoexposureCheckbox_clicked(bool setAutoexposure) {
    this->m_cameraInterface.m_camera->AutoExposure(setAutoexposure);
    ui->exposureSlider->setEnabled(!setAutoexposure);
    ui->exposureLineEdit->setEnabled(!setAutoexposure);
    UpdateExposure();
}


/**
 * Launches thread to record white reference images.
 *
 * @see MainWindow::RecordReferenceImages
 */
void MainWindow::on_whiteBalanceButton_clicked() {
    if (m_referenceRecordingThread.joinable()) {
        m_referenceRecordingThread.join();
    }
    m_referenceRecordingThread = boost::thread(&MainWindow::RecordReferenceImages, this, "white");
}


/**
 * Launches thread to record dark reference images.
 *
 * @see MainWindow::RecordReferenceImages
 */
void MainWindow::on_darkCorrectionButton_clicked() {
    if (m_referenceRecordingThread.joinable()) {
        m_referenceRecordingThread.join();
    }
    m_referenceRecordingThread = boost::thread(&MainWindow::RecordReferenceImages, this, "dark");
}


/**
 * Record reference images (white or dark). The number of images to be recorded is defined by
 * constants:NR_REFERENCE_IMAGES_TO_RECORD. After recording each image an amount of time equal to twice the integration
 * time is waited before recording the next image.
 *
 * @param referenceType
 */
void MainWindow::RecordReferenceImages(QString referenceType) {
    QMetaObject::invokeMethod(ui->recordButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    if (referenceType == "white"){
        QMetaObject::invokeMethod(ui->darkCorrectionButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    } else if (referenceType == "dark"){
        QMetaObject::invokeMethod(ui->whiteBalanceButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    }

    QString baseFolder = ui->baseFolderLineEdit->text();
    QDir dir(baseFolder);
    QStringList nameFilters;
    nameFilters << referenceType + "*";
    QStringList folderNameList = dir.entryList(nameFilters, QDir::Dirs | QDir::NoDotAndDotDot);
    QRegularExpression re("^" + referenceType + "(\\d*)$");

    QString referenceFolderName = referenceType;
    for(const QString& folderName : folderNameList){
        QRegularExpressionMatch match = re.match(folderName);
        if(match.hasMatch()) {
            int folderNum = match.captured(1).toInt();
            ++folderNum;
            referenceFolderName = referenceType + QString::number(folderNum);
        }
    }

    for (int i = 0; i < NR_REFERENCE_IMAGES_TO_RECORD; i++) {
        int exp_time = m_cameraInterface.m_camera->GetExposureMs();
        int waitTime = 2 * exp_time;
        wait(waitTime);
        std::stringstream name_i;
        name_i << referenceType.toStdString();
        this->RecordImage(referenceFolderName.toStdString(), name_i.str(), true);
        int progress = static_cast<int>((static_cast<float>(i + 1) / NR_REFERENCE_IMAGES_TO_RECORD) * 100);
        QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
    }
    QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
    QMetaObject::invokeMethod(ui->recordButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    if (referenceType == "white"){
        QMetaObject::invokeMethod(ui->darkCorrectionButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    } else if (referenceType == "dark"){
        QMetaObject::invokeMethod(ui->whiteBalanceButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    }
}


/**
 * Updates the style of the line edit element when edited.
 */
void MainWindow::on_minVhbLineEdit_textEdited(const QString &newText) {
    updateComponentEditedStyle(ui->minVhbLineEdit, newText, m_minVhb);
}


/*
 * Triggers the update of the blood volume fraction and oxygenation validators when minimum range of vhb is modified.
 */
void MainWindow::on_minVhbLineEdit_returnPressed() {
    this->UpdateVhbSao2Validators();
    m_minVhb = ui->minVhbLineEdit->text();
    RestoreLineEditStyle(ui->minVhbLineEdit);
}


/*
 * Updates the style of the line edit element when edited.
 */
void MainWindow::on_maxVhbLineEdit_textEdited(const QString &newText){
    updateComponentEditedStyle(ui->maxVhbLineEdit, newText, m_maxVhb);
}


/**
 * triggers the update of the blood volume fraction and oxygenation validators when maximum range of vhb is modified.
 */
void MainWindow::on_maxVhbLineEdit_returnPressed() {
    this->UpdateVhbSao2Validators();
    m_maxVhb = ui->maxVhbLineEdit->text();
    RestoreLineEditStyle(ui->maxVhbLineEdit);
}

/**
 * Updates the style of the line edit element when edited.
 */
void MainWindow::on_minSao2LineEdit_textEdited(const QString &newText) {
    updateComponentEditedStyle(ui->minSao2LineEdit, newText, m_minSao2);
}


/**
 * triggers the update of the blood volume fraction and oxygenation validators when minimum range of oxygen is modified
 */
void MainWindow::on_minSao2LineEdit_returnPressed() {
    this->UpdateVhbSao2Validators();
    m_minSao2 = ui->minSao2LineEdit->text();
    RestoreLineEditStyle(ui->minSao2LineEdit);
}


/*
 * Updates the style of the line edit element when edited.
 */
void MainWindow::on_maxSao2LineEdit_textEdited(const QString &newText){
    updateComponentEditedStyle(ui->maxSao2LineEdit, newText, m_maxSao2);
}


/**
 * triggers the update of the blood volume fraction and oxygenation validators when maximum range of oxygen is modified.
 */
void MainWindow::on_maxSao2LineEdit_returnPressed() {
    this->UpdateVhbSao2Validators();
    m_maxSao2 = ui->maxSao2LineEdit->text();
    RestoreLineEditStyle(ui->maxSao2LineEdit);
}


/**
 * Updates the look of a QLineEdit object based on if its content has changed. If the content has changed, the
 * appearance is changed to reflect a change. Once return key is pressed, the appearance is reseted.
 *
 * @param lineEdit object for which the look should be changed
 * @param newString new text of the QLineEdit object
 * @param originalString initial text of the QLineEdit object
 */
void MainWindow::updateComponentEditedStyle(QLineEdit* lineEdit, const QString& newString, const QString& originalString){
    if (QString::compare(newString, originalString, Qt::CaseSensitive)) {
        lineEdit->setStyleSheet(FIELD_EDITED_STYLE);
    } else {
        lineEdit->setStyleSheet(FIELD_ORIGINAL_STYLE);
    }
}


/**
 * restores the original appearance of a QLineEdit object
 *
 * @param lineEdit element to change style
 */
void MainWindow::RestoreLineEditStyle(QLineEdit* lineEdit) {
    lineEdit->setStyleSheet(FIELD_ORIGINAL_STYLE);
}


/**
 * Updates the appearance of the top folder name field in the GUI when edited
 *
 * @param newText new text of the QLineEdit object
 */
void MainWindow::on_subFolderLineEdit_textEdited(const QString &newText) {
    updateComponentEditedStyle(ui->subFolderLineEdit, newText, m_subFolder);
}


/**
 * Updates the appearance of the file prefix field in the GUI when edited
 *
 * @param newText new text of the QLineEdit object
 */
void MainWindow::on_filePrefixLineEdit_textEdited(const QString &newText) {
    updateComponentEditedStyle(ui->filePrefixLineEdit, newText, m_recPrefixlineEdit);
}


/**
 * Activates the display related to functional properties. When active, the raw image, RGB estimate and the functional
 * parameter estimations are displayed.
 */
void MainWindow::on_functionalRadioButton_clicked() {
    delete m_display;
    m_display = new DisplayerFunctional(this);
    m_display->StartDisplayer();
    QString cameraModel = ui->cameraListComboBox->currentText();
    m_display->SetCameraProperties(cameraModel);
}


/**
 * Activates the display related to functional properties. When active, the raw image is displayed on higher resolution
 * compared to the functional displayed
 *
 * @see MainWindow::on_functionalRadioButton_clicked
 */
void MainWindow::on_rawRadioButton_clicked() {
    delete m_display;
    m_display = new DisplayerRaw(this);
    m_display->StartDisplayer();
    QString cameraModel = ui->cameraListComboBox->currentText();
    m_display->SetCameraProperties(cameraModel);
}


/**
 * Updates the appearance of the "extras" folder name field in the GUI when edited
 *
 * @param newText new text of the QLineEdit object
 */
void MainWindow::on_subFolderExtrasLineEdit_textEdited(const QString &newText) {
    updateComponentEditedStyle(ui->subFolderExtrasLineEdit, newText, m_extrasSubFolder);
}


/**
 * stores name of folder where "extras" recordings should be stored in a member variable and restores QLineEdit
 * appearance.
 */
void MainWindow::on_subFolderExtrasLineEdit_returnPressed() {
    m_extrasSubFolder = ui->subFolderExtrasLineEdit->text();
    RestoreLineEditStyle(ui->subFolderExtrasLineEdit);
}


/**
 * @brief Slot function called when the text in the snapshotPrefixlineEdit is edited.
 *
 * @param newText The new text entered in the snapshotPrefixlineEdit.
 */
void MainWindow::on_filePrefixExtrasLineEdit_textEdited(const QString &newText){
    updateComponentEditedStyle(ui->filePrefixExtrasLineEdit, newText, m_extrasFilePrefix);
}


/**
 * @brief Slot function triggered when the return key is pressed in the snapshotPrefixlineEdit QLineEdit widget.
 * This method re-styles the appearance of the lineEdit object.
 *
 * This function is a slot that is connected to the returnPressed() signal of the snapshotPrefixlineEdit widget
 * in the MainWindow class. When the user presses the return key in the snapshotPrefixlineEdit widget,
 * this function is called.
 */
void MainWindow::on_filePrefixExtrasLineEdit_returnPressed(){
    m_extrasFilePrefix = ui->filePrefixExtrasLineEdit->text();
    RestoreLineEditStyle(ui->filePrefixExtrasLineEdit);
}


/**
 * Updates the appearance of the trigger text field in the GUI when edited
 *
 * @param newText new text of the QLineEdit object
 */
void MainWindow::on_logTextLineEdit_textEdited(const QString &newText) {
    updateComponentEditedStyle(ui->logTextLineEdit, newText, m_triggerText);
}


/**
 * Formats timestamp from  yyyyMMdd_HH-mm-ss-zzz to a human readable format
 *
 * @param timestamp timestamp to re-format
 * @return reformatted timestamp
 */
QString MainWindow::FormatTimeStamp(QString timestamp){
    QDateTime dateTime = QDateTime::fromString(timestamp, "yyyyMMdd_HH-mm-ss-zzz");
    QString formattedDate = dateTime.toString("hh:mm:ss AP");
    return formattedDate;
}


/**
 * Logs the written message from the GUI to the file named in constants.h: LOG_FILE_NAME. It also resets the content of
 * of the trigger text line edit and displays all messages on GUI
 */
void MainWindow::on_logTextLineEdit_returnPressed() {
    QString timestamp;
    QString trigger_message = ui->logTextLineEdit->text();
    // block signals until method ends
    const QSignalBlocker triggerTextBlocker(ui->logTextLineEdit);
    const QSignalBlocker triggersTextEdit(ui->logTextEdit);
    // log message and update member variable for trigger text
    trigger_message.prepend(" ");
    timestamp = this->LogMessage(trigger_message, LOG_FILE_NAME, true);
    timestamp = FormatTimeStamp(timestamp);
    m_triggerText = QString("<span style=\"color:gray;\">%1</span>").arg(timestamp) + QString("<b>%1</b>").arg(trigger_message) + "\n";

    // handle UI calls
    RestoreLineEditStyle(ui->logTextLineEdit);
    ui->logTextEdit->append(m_triggerText);
    ui->logTextEdit->show();
    ui->logTextLineEdit->clear();
}


/*
 * updates frames per second label in GUI when number of skipped frames is modified
 */
void MainWindow::on_skipFramesSpinBox_valueChanged() {
    // spin boxes do not have a returnPressed slot in Qt, which is why the value is always updated upon changes
    int exp_ms = m_cameraInterface.m_camera->GetExposureMs();
    int nSkipFrames = ui->skipFramesSpinBox->value();
    const QSignalBlocker blocker_label(ui->hzLabel);
    ui->hzLabel->setText(QString::number((double) (1000.0 / (exp_ms * (nSkipFrames + 1))), 'g', 2));
}


/**
 * Initializes the camera selected from a dropdown menu and stores its camera type as well as the camera model.
 * If initialization succeeds, the GUI components are activated and can from there on be used.
 *
 * @param index index from a dropdown menu of the camera to be initialized
 */
void MainWindow::on_cameraListComboBox_currentIndexChanged(int index) {
    boost::lock_guard<boost::mutex> guard(mtx_);
    // image acquisition should be stopped when index 0 (no camera) is selected from the dropdown menu
    try {
        this->StopImageAcquisition();
        m_cameraInterface.CloseDevice();
    } catch (std::runtime_error &e) {
        LOG_SUSICAM(warning) << "could not stop image acquisition: " << e.what();
    }
    if (index != 0) {
        QString cameraModel = ui->cameraListComboBox->currentText();
        m_cameraInterface.m_cameraModel = cameraModel;
        if (CAMERA_MAPPER.contains(cameraModel)) {
            QString cameraType = CAMERA_MAPPER.value(cameraModel).cameraType;
            QString originalCameraModel = m_cameraInterface.m_cameraModel;
            try {
                // set camera type needed by the camera interface initialization
                m_display->SetCameraProperties(cameraModel);
                m_cameraInterface.SetCameraProperties(cameraModel);
                this->StartImageAcquisition(ui->cameraListComboBox->currentText());
            } catch (std::runtime_error &e) {
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
            if (cameraType == CAMERA_TYPE_SPECTRAL) {
                QMetaObject::invokeMethod(ui->bandSlider, "setEnabled", Q_ARG(bool, true));
            } else {
                QMetaObject::invokeMethod(ui->bandSlider, "setEnabled", Q_ARG(bool, false));
            }
        } else {
            LOG_SUSICAM(error) << "camera model not in CAMERA_MAPPER: " << cameraModel.toStdString();
        }
    } else {
        const QSignalBlocker blocker_spinbox(ui->cameraListComboBox);
        m_cameraInterface.SetCameraIndex(index);
        this->EnableUi(false);
    }
}


/**
 * Updates the saturation percentage LCD displays with the given image.
 *
 * @param image The input image to compute saturation percentages.
 * @throw std::invalid_argument If the input matrix is empty or not of type CV_8UC1.
 */
void MainWindow::UpdateSaturationPercentageLCDDisplays(cv::Mat &image) const{
    if (image.empty() || image.type() != CV_8UC1) {
        throw std::invalid_argument("Invalid input matrix. It must be non-empty and of type CV_8UC1, got: " + cv::typeToString(image.type()));
    }

    int aboveThresholdCount = cv::countNonZero(image > OVEREXPOSURE_PIXEL_BOUNDARY_VALUE);
    double totalPixels = image.total();  // Total number of pixels in the matrix
    double percentageAboveThreshold = (static_cast<double>(aboveThresholdCount) / totalPixels) * 100.0;
    QMetaObject::invokeMethod(ui->overexposurePercentageLCDNumber, "display", Q_ARG(double, percentageAboveThreshold));

    int belowThresholdCount = cv::countNonZero(image < UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE);
    double percentageBelowThreshold = (static_cast<double>(belowThresholdCount) / totalPixels) * 100.0;
    QMetaObject::invokeMethod(ui->underexposurePercentageLCDNumber, "display", Q_ARG(double, percentageBelowThreshold));
}