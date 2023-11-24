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
#include "image_container.h"
#include "displayFunctional.h"
#include "displayRaw.h"
#include "constants.h"


/**
 * @brief XIIMGtoMat helper function which wraps a ximea image in a cv::Mat
 * @param xi_img input ximea image
 * @param mat_img output cv::Mat image
 */
void XIIMGtoMat(XI_IMG &xi_img, cv::Mat &mat_img) {
    mat_img = cv::Mat(xi_img.height, xi_img.width, CV_16UC1, xi_img.bp);
}


/**
* @class MainWindow
* @brief The MainWindow class represents the main window of the application.
*
* This class inherits from QMainWindow and is responsible for initializing the user interface.
* It also manages the IO service, work, and other variables related to image recording and testing.
*/
MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow),
        m_io_service(), m_work(m_io_service),
        m_recordedCount(0),
        m_testMode(g_commandLineArguments.test_mode),
        m_imageCounter(0),
        m_skippedCounter(0),
        m_elapsedTimeTextStream(&m_elapsedTimeText),
        m_elapsedTime(0) {
    ui->setupUi(this);
    // populate available cameras
    QStringList cameraList = m_camInterface.GetAvailableCameraModels();
    ui->cameraListComboBox->addItem("select camera to enable UI...");
    ui->cameraListComboBox->addItems(cameraList);
    ui->cameraListComboBox->setCurrentIndex(0);

    m_display = new DisplayerFunctional(this);

    // hack until we implement proper resource management
    QPixmap pix(":/resources/jet_photo.jpg");
    ui->jet_sao2->setPixmap(pix);
    ui->jet_vhb->setPixmap(pix);

    // set the base folder loc
    m_baseFolderLoc = QDir::cleanPath(
            QDir::currentPath() + QDir::separator() + QString::fromStdString(g_commandLineArguments.output_folder));

    ui->baseFolderLoc->insert(this->GetBaseFolder());

    // synchronize slider and exposure checkbox
    QSlider *slider = ui->exposureSlider;
    QLineEdit *expEdit = ui->label_exp;
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

    BOOST_LOG_TRIVIAL(info) << "test mode (recording everything to same file) is set to: " << m_testMode << "\n";

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
        m_camInterface.StartAcquisition(std::move(camera_identifier));

        this->StartPollingThread();

        /***************************************/
        // setup connections to displaying etc.
        /***************************************/
        // when a new image arrives, display it
        QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display);
        // display min/max values in small center roi of image
        QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this,
                         &MainWindow::UpdateMinMaxPixelValues);
    }
    catch (std::runtime_error &error) {
        BOOST_LOG_TRIVIAL(warning) << "could not start camera, got error " << error.what();
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
    this->StopPollingThread();
    this->StopTemperatureThread();
    m_camInterface.StopAcquisition();
    // disconnect slots for image display
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display);
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this,
                        &MainWindow::UpdateMinMaxPixelValues);
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
void MainWindow::disableWidgetsInLayout(QLayout *layout, bool enable) {
    for (int i = 0; i < layout->count(); ++i) {
        QLayout *subLayout = layout->itemAt(i)->layout();
        QWidget *widget = layout->itemAt(i)->widget();

        if (widget && (widget->objectName() != "recLowExposureImagesButton")) {
            widget->setEnabled(enable);
        }

        if (subLayout) {
            disableWidgetsInLayout(subLayout, enable);
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
    disableWidgetsInLayout(layout, enable);
    ui->exposureSlider->setEnabled(enable);
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
    const unsigned MAX_SAO2 = 100;
    const unsigned MAX_VHB = 30;
    const unsigned MIN_SAO2 = 0;
    const unsigned MIN_VHB = 0;

    ui->min_vhb_line_edit->setValidator(new QIntValidator(MIN_VHB, ui->max_vhb_line_edit->text().toInt(), this));
    ui->max_vhb_line_edit->setValidator(new QIntValidator(ui->min_vhb_line_edit->text().toInt(), MAX_VHB, this));
    ui->min_sao2_line_edit->setValidator(new QIntValidator(MIN_SAO2, ui->max_sao2_line_edit->text().toInt(), this));
    ui->max_sao2_line_edit->setValidator(new QIntValidator(ui->min_sao2_line_edit->text().toInt(), MAX_SAO2, this));
}


/**
 * @brief Updates the minimum and maximum pixel values of the main window.
 *
 * This function calculates the minimum and maximum pixel values from the current image data
 * and updates the corresponding member variables of the main window.
 *
 * @note This function should be called whenever there is a change in the image data that may
 * affect the minimum and maximum pixel values.
 *
 * @todo This method does not seem to be used for anything and could potentially be removed
 */
void MainWindow::UpdateMinMaxPixelValues() {
    const unsigned refresh_rate_ms = 1000; // refresh all 1000ms

    static boost::posix_time::ptime last = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();

    long time_since_last = (now - last).total_milliseconds();

    if (time_since_last > refresh_rate_ms) {
        XI_IMG xi_img = m_imageContainer.GetCurrentImage();
        cv::Mat mat_img;
        XIIMGtoMat(xi_img, mat_img);

        // Select ROI
        int w = 32, h = 32;
        cv::Rect roi = cv::Rect(2048 / 2 - cvRound(w / 2), 1024 / 2 - cvRound(h / 2), w, h);
        cv::Mat subImg = mat_img(roi);
        double min, max;

        cv::minMaxLoc(subImg, &min, &max);
        std::stringstream message;
        message << "min: " << (unsigned) min << " -- max: " << (unsigned) max;
        last = now;
    }
}


/**
 * @brief Destructor for MainWindow class.
 *
 * Cleans up resources used by the MainWindow object.
 * This destructor is automatically called when the object is destroyed.
 */
MainWindow::~MainWindow() {
    m_io_service.stop();
    m_threadpool.join_all();

    this->StopTemperatureThread();
    this->StopSnapshotsThread();

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
void MainWindow::Snapshots() {
    static QString original_colour = ui->recordButton->styleSheet();
    // create invocation to method to trigger changes in UI from the main thread
    QMetaObject::invokeMethod(ui->snapshotButton, "setStyleSheet", Qt::QueuedConnection,
                              Q_ARG(QString, BUTTON_PRESSED_STYLE));
    std::string name = ui->snapshotPrefixlineEdit->text().toUtf8().constData();
    int nr_images = ui->nrSnapshotsspinBox->value();

    for (int i = 0; i < nr_images; i++) {
        int exp_time = m_camInterface.GetExposureMs();
        std::stringstream name_i;
        name_i << name << "_" << i;
        this->SaveCurrentImage(name_i.str());
        // wait 2x exposure time to be sure to save new image
        int waitTime = 2 * exp_time;
        wait(waitTime);
        int progress = static_cast<int>((static_cast<float>(i + 1) / nr_images) * 100);
        QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
    }
    QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
    QMetaObject::invokeMethod(ui->snapshotButton, "setStyleSheet", Qt::QueuedConnection,
                              Q_ARG(QString, original_colour));
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
    m_snapshotsThread = boost::thread(&MainWindow::Snapshots, this);
}


/**
 * @brief Slot function called when the text in the snapshotPrefixlineEdit is edited.
 *
 * @param arg1 The new text entered in the snapshotPrefixlineEdit.
 */
void MainWindow::on_snapshotPrefixlineEdit_textEdited(const QString &arg1){
    updateLineEditStyle(ui->snapshotPrefixlineEdit, arg1, m_snapshotPrefix);
}


/**
 * @brief Slot function triggered when the return key is pressed in the snapshotPrefixlineEdit QLineEdit widget.
 * This method re-styles the appearance of the lineEdit object.
 *
 * @details This function is a slot that is connected to the returnPressed() signal of the snapshotPrefixlineEdit widget
 * in the MainWindow class. When the user presses the return key in the snapshotPrefixlineEdit widget,
 * this function is called.
 *
 * @note This function does not return any value.
 *
 * @sa MainWindow
 */
void MainWindow::on_snapshotPrefixlineEdit_returnPressed(){
    m_snapshotPrefix = ui->snapshotPrefixlineEdit->text();
    restoreLineEditStyle(ui->snapshotPrefixlineEdit);

}


/**
 * @brief Records the temperature of the camera.
 *
 * This function records the temperature of the camera and logs it to a file in the base folder specified in the GUI.
 * It uses the camera API to get the temperature value and prints it to the console output.
 * This function should be called periodically to monitor the camera's temperature.
 */
void MainWindow::RecordCameraTemperature() {
    QString message;
    m_camInterface.UpdateRecordedCameraTemperature();
    for (const QString &key: m_camInterface.m_cameraTemperature.keys()) {
        float temp = m_camInterface.m_cameraTemperature.value(key);
        message = QString("\t%1\t%2").arg(key).arg(temp);
        this->LogMessage(message, TEMP_LOG_FILE_NAME, true);
    }
    this->LogMessage(message, TEMP_LOG_FILE_NAME, true);
}


/**
 * @brief Starts a thread to schedule temperature.
 *
 * This function starts a thread which executes the temperature scheduling logic.
 * The temperature scheduling logic periodically checks the current temperature. The temperature is only logged
 * periodically according to the TEMP_LOG_INTERVAL variable
 */
void MainWindow::ScheduleTemperatureThread() {
    m_temperatureThreadTimer = new boost::asio::steady_timer(m_io_service);
    m_temperatureThreadTimer->expires_after(std::chrono::seconds(TEMP_LOG_INTERVAL));
    m_temperatureThreadTimer->async_wait(
            boost::bind(&MainWindow::HandleTimer, this, m_temperatureThreadTimer, boost::asio::placeholders::error));
}


/**
 * \brief Handle the timer expiration event
 *
 * This function is called when the timer expires. It is responsible for handling the timer expiration event.
 *
 * \param timer Pointer to the boost::asio::steady_timer object representing the timer.
 * \param error The error code associated with the timer expiration event, if any.
 */
void MainWindow::HandleTimer(boost::asio::steady_timer *timer, const boost::system::error_code &error) {
    if (error) {
        BOOST_LOG_TRIVIAL(error) << "Timer cancelled. Error: " << error;
        delete timer;
        return;
    }

    this->RecordCameraTemperature();

    // Reset timer
    timer->expires_after(std::chrono::seconds(TEMP_LOG_INTERVAL));
    timer->async_wait(boost::bind(&MainWindow::HandleTimer, this, timer, boost::asio::placeholders::error));
}


/**
 * @brief this method starts a new thread for temperature monitoring.
 *
 * This method creates a new thread to monitor the temperature by periodically calling the
 * `ReadTemperature()` method. The temperature is read and stored in a member variable for
 * further processing.
 * \see StopTemperatureThread()
 */
void MainWindow::StartTemperatureThread() {
    QString log_filename = QDir::cleanPath(ui->baseFolderLoc->text() + QDir::separator() + TEMP_LOG_FILE_NAME);
    QFile file(log_filename);
    QFileInfo fileInfo(file);
    if (fileInfo.size() == 0) {
        this->LogMessage("time\tsensor_location\ttemperature", TEMP_LOG_FILE_NAME, false);
    }
    file.close();
    m_temperatureThread = boost::thread([&]() {
        ScheduleTemperatureThread();
        m_io_service.run();
    });
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
        m_temperatureThreadTimer->cancel();
        m_temperatureThread.interrupt();
        m_temperatureThread.join();
        delete m_temperatureThreadTimer;
        m_temperatureThreadTimer = nullptr;
    }
}


void MainWindow::StopSnapshotsThread(){
    if (m_snapshotsThread.joinable()){
        m_snapshotsThread.join();
    }
}

void MainWindow::on_exposureSlider_valueChanged(int value) {
    m_camInterface.SetExposureMs(value);
    UpdateExposure();
}


void MainWindow::UpdateExposure() {
    int exp_ms = m_camInterface.GetExposureMs();
    int n_skip_frames = ui->skipFramesSpinBox->value();
    ui->label_exp->setText(QString::number((int) exp_ms));
    ui->label_hz->setText(QString::number((double) (1000.0 / (exp_ms * (n_skip_frames + 1))), 'g', 2));

    // need to block the signals to make sure the event is not immediately
    // thrown back to label_exp.
    // could be done with a QSignalBlocker from Qt5.3 on for exception safe treatment.
    // see: http://doc.qt.io/qt-5/qsignalblocker.html
    const QSignalBlocker blocker_slider(ui->exposureSlider);
    ui->exposureSlider->setValue(exp_ms);
}


void MainWindow::on_label_exp_returnPressed() {
    m_label_exp = ui->label_exp->text();
    m_camInterface.SetExposureMs(m_label_exp.toInt());
    UpdateExposure();
    restoreLineEditStyle(ui->label_exp);
}


void MainWindow::on_label_exp_textEdited(const QString &arg1) {
    updateLineEditStyle(ui->label_exp, arg1, m_label_exp);
}


void MainWindow::on_recordButton_clicked(bool checked) {
    static QString original_colour;
    static QString original_button_text;

    if (checked) {
        QString cameraModel = ui->cameraListComboBox->currentText();
        this->LogMessage("SUSICAM RECORDING STARTS", LOG_FILE_NAME, true);
        this->LogMessage(QString("camera selected: %1").arg(cameraModel), LOG_FILE_NAME, true);

        this->m_elapsedTimer.start();
        this->StartRecording();
        original_colour = ui->recordButton->styleSheet();
        original_button_text = ui->recordButton->text();
        // create invocation to method to trigger changes in UI from the main thread
        QMetaObject::invokeMethod(ui->recordButton, "setStyleSheet", Qt::QueuedConnection,
                                  Q_ARG(QString, BUTTON_PRESSED_STYLE));
        QMetaObject::invokeMethod(ui->recLowExposureImagesButton, "setEnabled", Qt::QueuedConnection,
                                  Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->cameraListComboBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
        ui->recordButton->setText("Stop recording");
    } else {
        this->LogMessage("SUSICAM RECORDING ENDS", LOG_FILE_NAME, true);
        this->StopRecording();
        QMetaObject::invokeMethod(ui->recordButton, "setStyleSheet", Qt::QueuedConnection,
                                  Q_ARG(QString, original_colour));
        QMetaObject::invokeMethod(ui->recLowExposureImagesButton, "setEnabled", Qt::QueuedConnection,
                                  Q_ARG(bool, false));
        QMetaObject::invokeMethod(ui->cameraListComboBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(ui->recordButton, "setText", Qt::QueuedConnection,
                                  Q_ARG(QString, original_button_text));
        ui->recordButton->setText(original_button_text);
    }
}


void MainWindow::closeEvent(QCloseEvent *event) {
    this->StopPollingThread();
    QMainWindow::closeEvent(event);
}


void MainWindow::on_chooseFolder_clicked() {
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
                ui->baseFolderLoc->clear();
                ui->baseFolderLoc->insert(this->GetBaseFolder());
                this->WriteLogHeader();
                this->StartTemperatureThread();
            }
        }
    }
}


void MainWindow::WriteLogHeader() {
    this->LogMessage(" git hash: " + QString::fromLatin1(libfive_git_revision()), LOG_FILE_NAME, true);
    this->LogMessage(" git branch: " + QString::fromLatin1(libfive_git_branch()), LOG_FILE_NAME, true);
    this->LogMessage(" git tags matching hash: " + QString::fromLatin1(libfive_git_version()), LOG_FILE_NAME, true);
}


QString MainWindow::LogMessage(QString message, QString log_file, bool log_time) {
    QString timestamp;
    QString curr_time = (QTime::currentTime()).toString("hh-mm-ss-zzz");
    QString date = (QDate::currentDate()).toString("yyyyMMdd_");
    timestamp = date + curr_time;
    QString log_filename = QDir::cleanPath(ui->baseFolderLoc->text() + QDir::separator() + log_file);
    QFile file(log_filename);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    if (log_time) {
        stream << timestamp;
    }
    stream << message << "\n";
    file.close();
    return timestamp;
}


void MainWindow::on_topFolderName_returnPressed() {
    m_topFolderName = ui->topFolderName->text();
    restoreLineEditStyle(ui->topFolderName);
}


void MainWindow::on_recPrefixlineEdit_returnPressed() {
    m_recPrefixlineEdit = ui->recPrefixlineEdit->text();
    restoreLineEditStyle(ui->recPrefixlineEdit);
}


bool MainWindow::GetNormalize() const {
    return this->ui->normalizeCheckbox->isChecked();
}


bool MainWindow::DoParamterScaling() const {
    return this->ui->ScaleParamtersCheckBox->isChecked();
}


cv::Range MainWindow::GetUpperLowerBoundsVhb() const {
    uchar lower, upper;
    lower = this->ui->min_vhb_line_edit->text().toInt();
    upper = this->ui->max_vhb_line_edit->text().toInt();
    return {lower, upper};
}

cv::Range MainWindow::GetUpperLowerBoundsSao2() const {
    uchar lower, upper;
    lower = this->ui->min_sao2_line_edit->text().toInt();
    upper = this->ui->max_sao2_line_edit->text().toInt();
    return cv::Range(lower, upper);
}


unsigned MainWindow::GetBand() const {
    return this->ui->bandSlider->value();
}


unsigned MainWindow::GetBGRNorm() const {
    return this->ui->rgbNormSlider->value();
}


bool MainWindow::SetBaseFolder(QString baseFolderPath) {
    if (QDir(baseFolderPath).exists()) {
        m_baseFolderLoc = baseFolderPath;
        return true;
    } else {
        return false;
    }
}


QString MainWindow::GetBaseFolder() const {
    return m_baseFolderLoc;
}


void MainWindow::ThreadedRecordImage() {
    m_io_service.post([this] { RecordImage(); });
}


void MainWindow::RecordImage() {
    XI_IMG image = m_imageContainer.GetCurrentImage();
    static long last_id = image.acq_nframe;
    int n_skip_frames = ui->skipFramesSpinBox->value();

    if ((image.acq_nframe == last_id) || (image.acq_nframe > last_id +
                                                             1)) { // attention, this is not thread save and might give false result when ThreadedRecordImage is used
        m_skippedCounter++;
    }
    last_id = image.acq_nframe;
    if ((n_skip_frames == 0) || (image.acq_nframe % n_skip_frames == 0)) {
        ++m_recordedCount;

        std::string currentFileName = m_recPrefixlineEdit.toUtf8().constData();
        QString fullPath = GetFullFilenameStandardFormat(currentFileName, image.acq_nframe, ".dat");

        try {
            FileImage f(fullPath.toStdString().c_str(), "wb");
            f.write(image);
        } catch (const std::runtime_error &e) {
            BOOST_LOG_TRIVIAL(error) << "Error: %s\n" << e.what();
        }

    }
    QMetaObject::invokeMethod(ui->recordedImagesLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(int, m_recordedCount));
}


void MainWindow::RecordImage(std::string subFolder) {
    XI_IMG image = m_imageContainer.GetCurrentImage();
    static long last_id = image.acq_nframe;
    int n_skip_frames = ui->skipFramesSpinBox->value();

    if ((image.acq_nframe == last_id) || (image.acq_nframe > last_id +
                                                             1)) { // attention, this is not thread save and might give false result when ThreadedRecordImage is used
        m_skippedCounter++;
    }
    last_id = image.acq_nframe;

    if ((n_skip_frames == 0) || (image.acq_nframe % n_skip_frames == 0)) {
        ++m_recordedCount;

        std::string currentFileName = m_recPrefixlineEdit.toUtf8().constData();
        QString fullPath = GetFullFilenameStandardFormat(currentFileName, image.acq_nframe, ".dat", subFolder);

        try {
            FileImage f(fullPath.toStdString().c_str(), "wb");
            f.write(image);
        } catch (const std::runtime_error &e) {
            BOOST_LOG_TRIVIAL(error) << "Error: %s\n" << e.what();
        }
    }
    QMetaObject::invokeMethod(ui->recordedImagesLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(int, m_recordedCount));
}


/**
 * @brief Updates the timer in the main window.
 *
 * This function is responsible for updating the timer displayed in the main window.
 * It should be called periodically to ensure the timer is always up-to-date.
 *
 * @note This function does not return any value.
 *
 * @par Usage
 *
 * @code{.cpp}
 * updateTimer();
 * @endcode
 *
 * @par Thread Safety
 *
 * This function is not thread-safe. It is expected to be called from the main thread.
 *
 * @see MainWindow
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


void MainWindow::stopTimer() {
    ui->timerLCDNumber->display(0);
}


void MainWindow::CountImages() {
    m_imageCounter++;
}


void MainWindow::StartRecording() {
    QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::ThreadedRecordImage);
    QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::CountImages);
    QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::updateTimer);
}


void MainWindow::StopRecording() {
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::ThreadedRecordImage);
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::CountImages);
    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::updateTimer);
    this->stopTimer();
    BOOST_LOG_TRIVIAL(info) << "Total of frames recorded: " << m_recordedCount;
    BOOST_LOG_TRIVIAL(info) << "Total of frames dropped : " << m_imageCounter - m_recordedCount;
    BOOST_LOG_TRIVIAL(info) << "Estimate for frames skipped: " << m_skippedCounter;
}


QString MainWindow::GetWritingFolder() {
    QString writeFolder = GetBaseFolder();

    writeFolder += QDir::separator() + m_topFolderName + QDir::separator();

    return QDir::cleanPath(writeFolder);
}


void MainWindow::CreateFolderIfNecessary(QString folder) {
    // Check if the directory exists, If not create it.
    // @TODO: Check for valid name.

    QDir folderDir(folder);

    if (!folderDir.exists()) {
        if (folderDir.mkpath(folder)) {
            BOOST_LOG_TRIVIAL(info) << "Directory created: " << folder.toStdString();
        }
    }
}


QString MainWindow::GetFullFilenameStandardFormat(std::string filename, long frameNumber, std::string extension,
                                                  std::string specialFolder) {

    QString writingFolder =
            GetWritingFolder() + QDir::separator() + QString::fromStdString(specialFolder) + QDir::separator();
    this->CreateFolderIfNecessary(writingFolder);

    QString fileName;
    if (!m_testMode) {
        int exp_time = m_camInterface.GetExposureMs();
        QString exp_time_str("exp" + QString::number(exp_time) + "ms");
        QString curr_time = (QTime::currentTime()).toString("hh-mm-ss-zzz");
        QString date = (QDate::currentDate()).toString("yyyyMMdd_");
        fileName = QString::fromStdString(filename) + "_" + date + curr_time + "_" + exp_time_str + "_" +
                   QString::number(frameNumber);
    } else {
        fileName = QString("test");
    }
    fileName += QString::fromStdString(extension);

    return QDir::cleanPath(writingFolder + fileName);
}


void MainWindow::SaveCurrentImage(std::string baseName, std::string specialFolder) {
    XI_IMG image = m_imageContainer.GetCurrentImage();
    QString fullPath = GetFullFilenameStandardFormat(baseName, image.acq_nframe, ".dat", specialFolder);
    try {
        FileImage f(fullPath.toStdString().c_str(), "wb");
        f.write(image);
    } catch (const std::runtime_error &e) {
        BOOST_LOG_TRIVIAL(error) << "Error: %s\n" << e.what();
    }
    BOOST_LOG_TRIVIAL(info) << "image " << fullPath.toStdString();
    m_recordedCount++;
    QMetaObject::invokeMethod(ui->recordedImagesLCDNumber, "display", Qt::QueuedConnection,
                              Q_ARG(int, m_recordedCount));
}


void MainWindow::StartPollingThread() {
    m_imageContainer.StartPolling();
    m_imageContainerThread = boost::thread(&ImageContainer::PollImage, &m_imageContainer, m_camInterface.GetHandle(),
                                           5);
}


void MainWindow::StopPollingThread() {
    m_imageContainerThread.interrupt();
    m_imageContainerThread.join();
    m_imageContainer.StopPolling();
}


void MainWindow::on_AutoexposureCheckbox_clicked(bool checked) {
    this->m_camInterface.AutoExposure(checked);
    ui->exposureSlider->setEnabled(!checked);
    ui->label_exp->setEnabled(!checked);
    UpdateExposure();
}


void MainWindow::on_whiteBalanceButton_clicked() {

}


void MainWindow::on_darkCorrectionButton_clicked() {

}


void MainWindow::on_min_vhb_line_edit_editingFinished() {
    this->UpdateVhbSao2Validators();
}


void MainWindow::on_max_vhb_line_edit_editingFinished() {
    this->UpdateVhbSao2Validators();
}


void MainWindow::on_min_sao2_line_edit_editingFinished() {
    this->UpdateVhbSao2Validators();
}


void MainWindow::on_max_sao2_line_edit_editingFinished() {
    this->UpdateVhbSao2Validators();
}


void MainWindow::updateLineEditStyle(QLineEdit* lineEdit, const QString& newString, const QString& originalString)
{
    if (QString::compare(newString, originalString, Qt::CaseSensitive)) {
        lineEdit->setStyleSheet(FIELD_EDITED_STYLE);
    } else {
        lineEdit->setStyleSheet(FIELD_ORIGINAL_STYLE);
    }
}


void MainWindow::restoreLineEditStyle(QLineEdit* lineEdit) {
    lineEdit->setStyleSheet(FIELD_ORIGINAL_STYLE);
}


void MainWindow::on_topFolderName_textEdited(const QString &arg1) {
    updateLineEditStyle(ui->topFolderName, arg1, m_topFolderName);
}


void MainWindow::on_recPrefixlineEdit_textEdited(const QString &arg1) {
    updateLineEditStyle(ui->recPrefixlineEdit, arg1, m_recPrefixlineEdit);
}


void MainWindow::on_functionalRadioButton_clicked() {
    delete m_display;
    m_display = new DisplayerFunctional(this);
}


void MainWindow::on_radioButtonRaw_clicked() {
    delete m_display;
    m_display = new DisplayerRaw(this);
}


/**
 * @brief MainWindow::lowExposureRecording
 *
 * We definde a static QString for the original color, a string sub_folder_name, an int original_exposure, a string prefix,
 * a vector exp_time and an int waitTime and set it zero.
 * We set the background color of the button recLowExposureImages to red and set exposureSlider and label_exp to false.
 * We call the methof on_recordButton_clocked and set it to false.
 * We define a Qstring tmp_topFolderName, wait 2* original_exposure and set m_topFolderName as nullptr.
 * We iterate through exp_time in a for-loop. Within this loop we iterate through the lowExposureImages in a second loop
 * and call the method RecordImage(subfolder) to save the images.
 * Then we set the color of the button recLowExposureImages to original color, m_topFolderName back to QString and
 * method on_recordButtol_clicked, exposureSlider and lab_exp back to true.
 * Then we synchronize the sliders and textedits dis´playing the current exposure setting.
 */
void MainWindow::lowExposureRecording() {
    int waitTime;
    static QString original_colour = ui->recLowExposureImagesButton->styleSheet();
    // store original skip frame value and disable spin box
    int n_skip_frames = ui->skipFramesSpinBox->value();
    QMetaObject::invokeMethod(ui->skipFramesSpinBox, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
    QMetaObject::invokeMethod(ui->skipFramesSpinBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, false));
    std::string sub_folder_name = ui->folderLowExposureImages->text().toUtf8().constData();
    int original_exposure = m_camInterface.GetExposureMs();
    std::vector<int> exp_time = {5, 10, 20, 40, 60, 80, 100, 150};
    // change style of record low exposure images button and disable exposure components
    QMetaObject::invokeMethod(ui->recLowExposureImagesButton, "setStyleSheet", Q_ARG(QString, BUTTON_PRESSED_STYLE));
    QMetaObject::invokeMethod(ui->exposureSlider, "setEnabled", Q_ARG(bool, false));
    QMetaObject::invokeMethod(ui->label_exp, "setEnabled", Q_ARG(bool, false));

    this->on_recordButton_clicked(false);
    QString tmp_topFolderName = m_topFolderName;
    wait(2 * original_exposure);
    m_topFolderName = nullptr;
    int nr_images = ui->nLowExposureImages->value() * exp_time.size();
    for (int i: exp_time) {
        m_camInterface.SetExposureMs(i);
        waitTime = 2 * i;
        wait(waitTime);
        for (int j = 0; j < ui->nLowExposureImages->value(); j++) {
            RecordImage(sub_folder_name);
            m_recordedCount++;
            int progress = static_cast<int>((static_cast<float>(i + 1) / nr_images) * 100);
            QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
        }
    }
    QMetaObject::invokeMethod(ui->progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, 0));
    m_camInterface.SetExposureMs(original_exposure);
    wait(2 * original_exposure);
    QMetaObject::invokeMethod(ui->recLowExposureImagesButton, "setStyleSheet", Q_ARG(QString, original_colour));
    QMetaObject::invokeMethod(ui->exposureSlider, "setEnabled", Q_ARG(bool, true));
    QMetaObject::invokeMethod(ui->label_exp, "setEnabled", Q_ARG(bool, true));
    // restore record button state
    m_topFolderName = tmp_topFolderName;
    this->on_recordButton_clicked(true);
    // re-enable skip frames spin box
    QMetaObject::invokeMethod(ui->skipFramesSpinBox, "setValue", Qt::QueuedConnection, Q_ARG(int, n_skip_frames));
    QMetaObject::invokeMethod(ui->skipFramesSpinBox, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    UpdateExposure();
}


void MainWindow::on_recLowExposureImagesButton_clicked() {
    boost::thread(&MainWindow::lowExposureRecording, this);
}


void MainWindow::on_folderLowExposureImages_textEdited(const QString &arg1) {
    updateLineEditStyle(ui->folderLowExposureImages, arg1, m_folderLowExposureImages);
}


void MainWindow::on_folderLowExposureImages_returnPressed() {
    m_folderLowExposureImages = ui->folderLowExposureImages->text();
    restoreLineEditStyle(ui->folderLowExposureImages);
}


void MainWindow::on_triggerText_textEdited(const QString &arg1) {
    updateLineEditStyle(ui->triggerText, arg1, m_triggerText);
}


void MainWindow::on_triggerText_returnPressed() {
    QString timestamp;
    QString trigger_message = ui->triggerText->text();
    // block signals until method ends
    const QSignalBlocker triggerTextBlocker(ui->triggerText);
    const QSignalBlocker triggersTextEdit(ui->triggersTextEdit);
    // log message and update member variable for trigger text
    trigger_message.prepend(" ");
    timestamp = this->LogMessage(trigger_message, LOG_FILE_NAME, true);
    m_triggerText = timestamp + trigger_message + "\n";

    // handle UI calls
    restoreLineEditStyle(ui->triggerText);
    ui->triggersTextEdit->append(m_triggerText);
    ui->triggersTextEdit->show();
    ui->triggerText->clear();
}


/*
 * updates frames per second label in GUI when number of skipped frames is modified
 */
void MainWindow::on_skipFramesSpinBox_valueChanged() {
    int exp_ms = m_camInterface.GetExposureMs();
    int n_skip_frames = ui->skipFramesSpinBox->value();
    const QSignalBlocker blocker_label(ui->label_hz);
    ui->label_hz->setText(QString::number((double) (1000.0 / (exp_ms * (n_skip_frames + 1))), 'g', 2));
}


void MainWindow::on_cameraListComboBox_currentIndexChanged(int index) {
    boost::lock_guard<boost::mutex> guard(mtx_);
    try {
        this->StopImageAcquisition();
        m_camInterface.CloseDevice();
    } catch (std::runtime_error &e) {
        BOOST_LOG_TRIVIAL(error) << "could not stop image acquisition: " << e.what();
    }
    if (index != 0) {
        QString cameraModel = ui->cameraListComboBox->currentText();
        m_camInterface.m_cameraModel = cameraModel;
        if (CAMERA_TYPE_MAPPER.contains(cameraModel)) {
            QString cameraType = CAMERA_TYPE_MAPPER.value(cameraModel);
            try {
                this->StartImageAcquisition(ui->cameraListComboBox->currentText());
            } catch (std::runtime_error &e) {
                BOOST_LOG_TRIVIAL(error) << "could not start image acquisition for camera: " << cameraModel.toStdString();
                const QSignalBlocker blocker_spinbox(ui->cameraListComboBox);
                ui->cameraListComboBox->setCurrentIndex(m_camInterface.m_cameraIndex);
                return;
            }
            m_display->SetCameraType(cameraType);
            m_camInterface.SetCameraType(cameraType);
            m_camInterface.SetCameraIndex(index);
            this->EnableUi(true);
            if (cameraType == SPECTRAL_CAMERA) {
                QMetaObject::invokeMethod(ui->bandSlider, "setEnabled", Q_ARG(bool, true));
                QMetaObject::invokeMethod(ui->rgbNormSlider, "setEnabled", Q_ARG(bool, true));
            } else {
                QMetaObject::invokeMethod(ui->bandSlider, "setEnabled", Q_ARG(bool, false));
                QMetaObject::invokeMethod(ui->rgbNormSlider, "setEnabled", Q_ARG(bool, false));
            }
        } else {
            BOOST_LOG_TRIVIAL(error) << "camera model not in CAMERA_TYPE_MAPPER: " << cameraModel.toStdString();
        }
    } else {
        const QSignalBlocker blocker_spinbox(ui->cameraListComboBox);
        m_camInterface.SetCameraIndex(index);
        this->EnableUi(false);
    }
}
