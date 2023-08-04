/*
 * ===================================================================
 * Surgical Spectral Imaging Library (SuSI)
 *
 * Copyright (c) German Cancer Research Center,
 * Division of Medical and Biological Informatics.
 * All rights reserved.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 *
 * See LICENSE.txt for details.
 * ===================================================================
 */


#include <iostream>
#include <string>
#include <stdlib.h>     //for using the function sleep

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#if CV_VERSION_MAJOR==3
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/core/types_c.h>
#endif

#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/bind/bind.hpp>

#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QDateTime>
#include <QTextStream>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "util.h"
#include "camera_interface.h"
#include "image_container.h"
#include "default_defines.h"
#include "displayFunctional.h"
#include "displayRaw.h"


/**
 * @brief XIIMGtoMat helper function which wraps a ximea image in a cv::Mat
 * @param xi_img input ximea image
 * @param mat_img output cv::Mat image
 */
void XIIMGtoMat(XI_IMG& xi_img, cv::Mat& mat_img)
{
    mat_img = cv::Mat(xi_img.height, xi_img.width, CV_16UC1, xi_img.bp);
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_io_service(), m_work(m_io_service),
    m_recordedCount(0),
    m_testMode(g_commandLineArguments.test_mode),
    m_imageCounter(0),
    m_skippedCounter(0)
{
    ui->setupUi(this);

    m_display = new DisplayerFunctional(this);

    // hack until we implement proper resource management
    QPixmap pix(":/resources/jet_photo.jpg");
    ui->jet_sao2->setPixmap(pix);
    ui->jet_vhb->setPixmap(pix);

    // set the basefolder loc
    m_baseFolderLoc =  QDir::cleanPath(QDir::currentPath() + QDir::separator() + QString::fromStdString(g_commandLineArguments.output_folder));

    ui->baseFolderLoc->insert(this->GetBaseFolder());

    // synchronize slider and exposure checkbox
    QSlider* slider = ui->exposureSlider;
    QLineEdit* expEdit = ui->label_exp;
    // set default values and ranges
    int slider_min = slider->minimum();
    int slider_max = slider->maximum();
    expEdit->setValidator(new QIntValidator(slider_min, slider_max, this));
    QString initialExpString = QString::number(slider->value());
    expEdit->setText(initialExpString);
    UpdateVhbSao2Validators();

    // create threadpool
    for (int i = 0; i < 2; i++) // put 2 threads in threadpool
    {
        m_threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &m_io_service));
    }

    BOOST_LOG_TRIVIAL(info) << "test mode (recording everything to same file) is set to: " << m_testMode << "\n";

    try {
        m_camInterface.StartAcquisition();

        this->StartPollingThread();

        /***************************************/
        // setup connections to displaying etc.
        /***************************************/
        // when a new image arrives, display it
        QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display);
        // display min/max values in small center roi of image
        QObject::connect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::UpdateMinMaxPixelValues);
    }
    catch (std::runtime_error& error)
    {
        BOOST_LOG_TRIVIAL(warning) << "could not start camera, got error " << error.what();
    }
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


void MainWindow::UpdateVhbSao2Validators()
{
    const unsigned MAX_SAO2 = 100;
    const unsigned MAX_VHB = 30;
    const unsigned MIN_SAO2 = 0;
    const unsigned MIN_VHB = 0;

    ui->min_vhb_line_edit->setValidator(new QIntValidator(MIN_VHB, ui->max_vhb_line_edit->text().toInt(), this));
    ui->max_vhb_line_edit->setValidator(new QIntValidator(ui->min_vhb_line_edit->text().toInt(), MAX_VHB, this));
    ui->min_sao2_line_edit->setValidator(new QIntValidator(MIN_SAO2, ui->max_sao2_line_edit->text().toInt(), this));
    ui->max_sao2_line_edit->setValidator(new QIntValidator(ui->min_sao2_line_edit->text().toInt(), MAX_SAO2, this));
}

void MainWindow::UpdateMinMaxPixelValues()
{
    const unsigned refresh_rate_ms = 1000; // refresh all 1000ms

    static boost::posix_time::ptime last = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();

    long time_since_last = (now - last).total_milliseconds();

    if (time_since_last > refresh_rate_ms)
    {
        XI_IMG xi_img = m_imageContainer.GetCurrentImage();
        cv::Mat mat_img;
        XIIMGtoMat(xi_img, mat_img);

        // Select ROI
        int w=32, h=32;
        cv::Rect roi = cv::Rect(2048/2-cvRound(w/2), 1024/2-cvRound(h/2), w, h);
        cv::Mat subImg = mat_img(roi);
        double min, max;

        cv::minMaxLoc(subImg, &min, &max);
        std::stringstream message;
        message << "min: " << (unsigned) min << " -- max: " << (unsigned) max;
        last=now;
    }
}


MainWindow::~MainWindow()
{
    m_io_service.stop();
    m_threadpool.join_all();

    QObject::disconnect(&(this->m_imageContainer), &ImageContainer::NewImage, this, &MainWindow::Display);

    delete ui;
}


void MainWindow::Snapshots()
{
    std::string name = ui->snapshotPrefixlineEdit->text().toUtf8().constData();

    for (int i=0; i < ui->nrSnapshotsspinBox->value(); i++)
    {
        int exp_time = m_camInterface.GetExposureMs();
        std::stringstream name_i;
        name_i << name << "_" << i;
        this->SaveCurrentImage(name_i.str());
        // wait 2x exposure time to be sure to save new image
        int waitTime = 2 * exp_time;
        wait(waitTime);
    }
}


void MainWindow::on_snapshotButton_clicked()
{
    boost::thread(&MainWindow::Snapshots, this);
}


void MainWindow::on_exposureSlider_valueChanged(int value)
{
    m_camInterface.SetExposureMs(value);
    UpdateExposure();
}


void MainWindow::UpdateExposure()
{
    int exp_ms = m_camInterface.GetExposureMs();
    int n_skip_frames = ui->skipFramesSpinBox->value();
//    const QSignalBlocker blocker_label(ui->label_exp);
    ui->label_exp->setText(QString::number((int) exp_ms));
    ui->label_hz->setText(QString::number((double) (1000.0 / (exp_ms * (n_skip_frames + 1))), 'g', 2));

    // need to block the signals to make sure the event is not immediately
    // thrown back to label_exp.
    // could be done with a QSignalBlocker from Qt5.3 on for exception safe treatment.
    // see: http://doc.qt.io/qt-5/qsignalblocker.html
    const QSignalBlocker blocker_slider(ui->exposureSlider);
    ui->exposureSlider->setValue(exp_ms);
}


void MainWindow::on_label_exp_editingFinished()
{
    m_camInterface.SetExposureMs(ui->label_exp->text().toInt());
    UpdateExposure();
}


void MainWindow::on_recordButton_clicked(bool checked)
{
    static QString original_colour;

    if (checked)
    {
        this->m_elapsedTimer.start();
        this->StartRecording();
        original_colour = ui->recordButton->styleSheet();
        ui->recordButton->setStyleSheet("background-color: rgb(255, 0, 0)");
        ui->recLowExposureImagesButton->setEnabled(true);
    }
    else
    {
        this->StopRecording();
        ui->recordButton->setStyleSheet(original_colour);
        ui->recLowExposureImagesButton->setEnabled(false);
    }
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    bool m_running;
    this->StopPollingThread();
    QMainWindow::closeEvent(event);
}

void MainWindow::on_chooseFolder_clicked()
{
    bool isValid = false;
    while (!isValid)
    {
        QString baseFolderPath = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                          "", QFileDialog::ShowDirsOnly
                                                          | QFileDialog::DontResolveSymlinks);

        if (QDir(baseFolderPath).exists())
        {
            isValid = true;
            if (!baseFolderPath.isEmpty())
            {
                this->SetBaseFolder(baseFolderPath);
                ui->baseFolderLoc->clear();
                ui->baseFolderLoc->insert(this->GetBaseFolder());
            }
        }
    }
}



void MainWindow::on_topFolderName_returnPressed()
{
    m_topFolderName = ui->topFolderName->text();
    ui->topFolderName->setStyleSheet("background-color: rgb(255, 255, 255)");
}


void MainWindow::on_recPrefixlineEdit_returnPressed()
{
    m_recPrefixlineEdit = ui->recPrefixlineEdit->text();
    ui->recPrefixlineEdit->setStyleSheet("background-color: rgb(255, 255, 255)");
}


bool MainWindow::GetNormalize() const
{
    return this->ui->normalizeCheckbox->isChecked();
}


bool MainWindow::GetRGBMatrixTransform() const
{
    return this->ui->rgbMatrixTransformCheckBox->isChecked();
}


bool MainWindow::DoParamterScaling() const
{
    return this->ui->ScaleParamtersCheckBox->isChecked();
}


cv::Range MainWindow::GetUpperLowerBoundsVhb() const
{
    uchar lower, upper;
    lower = this->ui->min_vhb_line_edit->text().toInt();
    upper = this->ui->max_vhb_line_edit->text().toInt();
    return cv::Range(lower, upper);
}

cv::Range MainWindow::GetUpperLowerBoundsSao2() const
{
    uchar lower, upper;
    lower = this->ui->min_sao2_line_edit->text().toInt();
    upper = this->ui->max_sao2_line_edit->text().toInt();
    return cv::Range(lower, upper);
}

unsigned MainWindow::GetBand() const
{
    return this->ui->bandSlider->value();
}

unsigned MainWindow::GetRGBNorm() const {
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
        return false;
}

QString MainWindow::GetBaseFolder() const
{
    return m_baseFolderLoc;
}


void MainWindow::ThreadedRecordImage()
{
    m_io_service.post(boost::bind(&MainWindow::RecordImage, this));
}


void MainWindow::RecordImage()
{
    XI_IMG image = m_imageContainer.GetCurrentImage();
    static long last_id = image.acq_nframe;
    int n_skip_frames = ui->skipFramesSpinBox->value();

    if ((image.acq_nframe==last_id) || (image.acq_nframe > last_id+1))
    { // attention, this is not thread save and might give false result when ThreadedRecordImage is used
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
        } catch (const std::runtime_error&e) {
            BOOST_LOG_TRIVIAL(error) << "Error: %s\n" <<  e.what();
        }

    }
}

void MainWindow::RecordImage(std::string subFolder)
{
    XI_IMG image = m_imageContainer.GetCurrentImage();
    static long last_id = image.acq_nframe;
    int n_skip_frames = ui->skipFramesSpinBox->value();

    if ((image.acq_nframe==last_id) || (image.acq_nframe > last_id+1))
    { // attention, this is not thread save and might give false result when ThreadedRecordImage is used
        m_skippedCounter++;
    }
    last_id = image.acq_nframe;

    if ((n_skip_frames == 0) || (image.acq_nframe % n_skip_frames == 0)) {
        ++m_recordedCount;

        std::string currentFileName = m_recPrefixlineEdit.toUtf8().constData();
        QString fullPath = GetFullFilenameStandardFormat(currentFileName, image.acq_nframe, ".dat", subFolder);

        // TODO SW: this is not nice code so far, nicen it up with RAII
        FILE *imageFile = fopen(fullPath.toStdString().c_str(), "wb");
        fwrite(image.bp, image.width * image.height, sizeof(UINT16), imageFile);
        fclose(imageFile);
    }
}

void MainWindow::updateTimer(){
    m_elapsedTime = float(m_elapsedTimer.elapsed()) / 1000.0;
    ui->lcdNumberTimer->display(QString::number(m_elapsedTime, 'f', 3).rightJustified(7, '0'));
}

void MainWindow::stopTimer(){
    ui->lcdNumberTimer->display(0);
}

void MainWindow::CountImages()
{
    m_imageCounter++;
}


void MainWindow::StartRecording()
{
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
    std::cout << "Total of frames recorded: " << m_recordedCount << std::endl;
    std::cout << "Total of frames dropped : " << m_imageCounter - m_recordedCount << std::endl;
    std::cout << "Estimate for frames skipped: " << m_skippedCounter << std::endl;
}


QString MainWindow::GetWritingFolder()
{
    QString writeFolder = GetBaseFolder();

    writeFolder += QDir::separator() + m_topFolderName + QDir::separator();

    return QDir::cleanPath(writeFolder);
}


void MainWindow::CreateFolderIfNeccessary(QString folder)
{
    // Check if the directory exists, If not create it.
    // @TODO: Check for valid name.

    QDir folderDir(folder);

    if (!folderDir.exists())
    {
        if (folderDir.mkpath(folder))
        {
            std::cout << "Directory created: " << folder.toStdString() << "\n" << std::flush;
        }
    }
}


QString MainWindow::GetFullFilenameStandardFormat(std::string filename, long frameNumber, std::string extension, std::string specialFolder)
{

    QString writingFolder = GetWritingFolder() + QDir::separator() + QString::fromStdString(specialFolder) + QDir::separator();
    this->CreateFolderIfNeccessary(writingFolder);

    QString fileName;
    if (!m_testMode)
    {
        int exp_time = m_camInterface.GetExposureMs();
        QString exp_time_str("exp" + QString::number(exp_time) + "ms");
        QString curr_time = (QTime::currentTime()).toString("hh-mm-ss-zzz");
        QString date = (QDate::currentDate()).toString("yyyyMMdd_");
        fileName = QString::fromStdString(filename) + "_" + date + curr_time + "_" +  exp_time_str + "_" + QString::number(frameNumber);
    }
    else
    {
        fileName = QString("test");
    }
    fileName += QString::fromStdString(extension);

    return QDir::cleanPath(writingFolder + fileName);
}



void MainWindow::SaveCurrentImage(std::string baseName, std::string specialFolder)
{
    XI_IMG image = m_imageContainer.GetCurrentImage();
    QString fullPath = GetFullFilenameStandardFormat(baseName, image.acq_nframe, ".dat", specialFolder);
    try {
        FileImage f(fullPath.toStdString().c_str(), "wb");
        f.write(image);
    } catch (const std::runtime_error&e) {
        BOOST_LOG_TRIVIAL(error) << "Error: %s\n" <<  e.what();
    }
    std::cout << "image " << fullPath.toStdString() << " saved\n" << std::flush;
}

void MainWindow::StartPollingThread()
{
    m_image_container_thread = boost::thread(&ImageContainer::PollImage, &m_imageContainer, m_camInterface.GetHandle(), 5);
}

void MainWindow::StopPollingThread()
{
    m_image_container_thread.interrupt();
    m_imageContainer.StopPolling();
    m_image_container_thread.join();
}


void MainWindow::on_AutoexposureCheckbox_clicked(bool checked)
{
    this->m_camInterface.AutoExposure(checked);
    ui->exposureSlider->setEnabled(!checked);
    ui->label_exp->setEnabled(!checked);
    UpdateExposure();
}


void MainWindow::on_whiteBalanceButton_clicked()
{

}

void MainWindow::on_darkCorrectionButton_clicked()
{

}

void MainWindow::on_min_vhb_line_edit_editingFinished()
{
    this->UpdateVhbSao2Validators();
}

void MainWindow::on_max_vhb_line_edit_editingFinished()
{
    this->UpdateVhbSao2Validators();
}

void MainWindow::on_min_sao2_line_edit_editingFinished()
{
    this->UpdateVhbSao2Validators();
}

void MainWindow::on_max_sao2_line_edit_editingFinished()
{
    this->UpdateVhbSao2Validators();
}


void MainWindow::on_topFolderName_textEdited(const QString &arg1)
{
    if (QString::compare(arg1, m_topFolderName, Qt::CaseSensitive))
    {
        ui->topFolderName->setStyleSheet("background-color: rgb(255, 105, 180)");
    }
    else
    {
        ui->topFolderName->setStyleSheet("background-color: rgb(255, 255, 255)");
    }


}


void MainWindow::on_recPrefixlineEdit_textEdited(const QString &arg1)
{
    if (QString::compare(arg1, m_recPrefixlineEdit, Qt::CaseSensitive))
    {
        ui->recPrefixlineEdit->setStyleSheet("background-color: rgb(255, 105, 180)");
    }
    else
    {
        ui->recPrefixlineEdit->setStyleSheet("background-color: rgb(255, 255, 255)");
    }
}

void MainWindow::on_functionalRadioButton_clicked()
{
    delete m_display;
    m_display = new DisplayerFunctional(this);
}

void MainWindow::on_radioButtonRaw_clicked()
{
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
void MainWindow::lowExposureRecording()
{
    static QString original_colour = ui->recLowExposureImagesButton->styleSheet();
    std::string sub_folder_name = ui->folderLowExposureImages->text().toUtf8().constData();
    int original_exposure = m_camInterface.GetExposureMs();
    std::string prefix = "low_exposure";
    std::vector<int> exp_time = {5, 10, 20, 40, 60, 80, 100, 150};

    int waitTime = 0;
    ui->recLowExposureImagesButton->setStyleSheet("background-color: rgb(255, 0, 0)");
    ui->exposureSlider->setEnabled(false);
    ui->label_exp->setEnabled(false);

    this->on_recordButton_clicked(false);
    QString tmp_topFolderName = m_topFolderName;
    wait(2 * original_exposure);
    m_topFolderName = nullptr;


    for (int i=0; i < exp_time.size(); i++)
    {
        m_camInterface.SetExposureMs(exp_time[i]);
        waitTime = 2 * exp_time[i];
        wait(waitTime);
        for (int j=0; j < ui->nLowExposureImages->value(); j++)
        {
            RecordImage(sub_folder_name);
        }
    }
    m_camInterface.SetExposureMs(original_exposure);
    wait(2 * original_exposure);
    ui->recLowExposureImagesButton->setStyleSheet(original_colour);

    m_topFolderName = tmp_topFolderName;
    this->on_recordButton_clicked(true);

    ui->exposureSlider->setEnabled(true);
    ui->label_exp->setEnabled(true);
    UpdateExposure();
}

void MainWindow::on_recLowExposureImagesButton_clicked()
{
    boost::thread(&MainWindow::lowExposureRecording, this);
}

void MainWindow::on_folderLowExposureImages_textEdited(const QString &arg1)
{
    if (QString::compare(arg1, m_folderLowExposureImages, Qt::CaseSensitive))
    {
        ui->folderLowExposureImages->setStyleSheet("background-color: rgb(255, 105, 180)");
    }
    else
    {
        ui->folderLowExposureImages->setStyleSheet("background-color: rgb(255, 255, 255)");
    }


}

void MainWindow::on_folderLowExposureImages_returnPressed()
{
    m_folderLowExposureImages = ui->folderLowExposureImages->text();
    ui->folderLowExposureImages->setStyleSheet("background-color: rgb(255, 255, 255)");
}

void MainWindow::on_triggerText_textEdited(const QString &arg1)
{
    if (QString::compare(arg1, m_triggerText, Qt::CaseSensitive))
    {
        ui->triggerText->setStyleSheet("background-color: rgb(255, 105, 180)");
    }
    else
    {
        ui->triggerText->setStyleSheet("background-color: rgb(255, 255, 255)");
    }
}

void MainWindow::on_triggerText_returnPressed()
{
    QString trigger_message = ui->triggerText->text();
    QString curr_time = (QTime::currentTime()).toString("hh-mm-ss-zzz");
    QString log_filename = QDir::cleanPath(ui->baseFolderLoc->text() + QDir::separator() + "logFile.txt");
    QString file_content = "";
    QFile file(log_filename);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    if (file.size() == 0){
        stream << curr_time + " " + "git hash: " << libfive_git_revision() << "\n";
        stream << curr_time + " " + "git branch: " << libfive_git_branch() << "\n";
        stream << curr_time + " " + "git tags matching hash: " << libfive_git_version() << "\n";
    }
    m_triggerText = curr_time + " " + trigger_message + "\n";
    stream << m_triggerText;
    file.close();
    m_triggerText = m_triggerText;
    ui->triggerText->setStyleSheet("background-color: rgb(255, 255, 255)");
    ui->triggersTextEdit->append(m_triggerText);
    ui->triggerText->clear();
}

/*
 * updates frames per second label in GUI when number of skipped frames is modified
 */
void MainWindow::on_skipFramesSpinBox_valueChanged()
{
    int exp_ms = m_camInterface.GetExposureMs();
    int n_skip_frames = ui->skipFramesSpinBox->value();
    const QSignalBlocker blocker_label(ui->label_hz);
    ui->label_hz->setText(QString::number((double) (1000.0 / (exp_ms * (n_skip_frames + 1))), 'g', 2));
}
