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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QElapsedTimer>

#include <boost/thread.hpp>

#include "camera_interface.h"
#include "display.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool GetNormalize() const;

    bool GetRGBMatrixTransform() const;

    bool DoParamterScaling() const;

    unsigned GetBand() const;

    unsigned GetRGBNorm() const;

    cv::Range GetUpperLowerBoundsVhb() const;
    cv::Range GetUpperLowerBoundsSao2() const;

protected:
    void closeEvent (QCloseEvent *event);

private slots:

    void on_snapshotButton_clicked();

    void on_exposureSlider_valueChanged(int value);


    void on_recordButton_clicked(bool checked);

    void on_chooseFolder_clicked();
    
    void on_label_exp_editingFinished();

    void on_topFolderName_returnPressed();

    void on_recPrefixlineEdit_returnPressed();

    void on_folderLowExposureImages_returnPressed();

    void on_AutoexposureCheckbox_clicked(bool checked);

    void on_whiteBalanceButton_clicked();

    void on_darkCorrectionButton_clicked();

    void on_min_vhb_line_edit_editingFinished();

    void on_max_vhb_line_edit_editingFinished();

    void on_min_sao2_line_edit_editingFinished();

    void on_max_sao2_line_edit_editingFinished();

    void on_topFolderName_textEdited(const QString &arg1);

    void on_recPrefixlineEdit_textEdited(const QString &arg1);

    void on_folderLowExposureImages_textEdited(const QString &arg1);

    void on_caffeRadioButton_clicked();

    void on_radioButtonRaw_clicked();

    void on_radioButtonSaturation_clicked();

    void on_triggerText_textEdited(const QString &arg1);

    void on_triggerText_returnPressed();

    void on_radioButtonDemo_clicked();

    void on_recLowExposureImages_clicked();

private:
    Ui::MainWindow *ui;

    /**
     * @brief SaveCurrentImage safe the current image to tif
     * @param baseName the base file name to be saved
     * @param specialFolder only neccessary if you want to save the image in a special subfolder as "white" or "dark"
     */
    void SaveCurrentImage(std::string baseName, std::string specialFolder="");

    /**
     * @brief Displays a new image
     */
    void Display();

    void StartRecording();
    void StopRecording();

    void StartPollingThread();
    void StopPollingThread();

    bool SetBaseFolder(QString baseFolderPath);
    void CreateFolderIfNeccessary(QString folder);

    void RecordImage();
    void RecordImage(std::string subFolder);
    void ThreadedRecordImage();
    // counts how many images were recorded
    unsigned long m_recordedCount;
    // counts how many images should have been recorded
    unsigned long m_imageCounter;
    unsigned long m_skippedCounter;
    void CountImages();

    void updateTimer();
    void stopTimer();

    void RunNetwork();
    /**
     * @brief Snapshots helper method to take snapshots, basically just created to be able to
     * thread the snapshot making :-)
     */
    void Snapshots();

    /**
     * @brief lowExposureRecording helper method to record images at different exposure times. Created to thread this recordings.
     */
    void lowExposureRecording();

    /**
     * @brief UpdateMinMaxPixelValues read the min/max values from an roi in the image and display in statusbar
     */
    void UpdateMinMaxPixelValues();

    void UpdateVhbSao2Validators();

    /**
     * @brief UpdateWhiteDarkCorrection Convenience method for white/dark correction setting
     *
     * white dark/image is determined and set in the network.
     * Also, the raw images are stored so they can be recomputed in offline analysis under dark/white
     *
     * @param imagetype what should it be, dark or white correction?
     */
    void UpdateWhiteDarkCorrection(enum Network::InputImage imagetype);

    /**
     * @brief UpdateExposure Synchronizes the sliders and textedits displaying the current exposure setting
     */
    void UpdateExposure();

    /**
     * @brief MainWindow::GetWritingFolder returns the folder there the image files are written to
     * @return
     */
    QString GetWritingFolder();

    /**
     * @brief GetFullFilenameStandardFormat returns the full filename of the current file which shall be written
     *
     * It automatically add the current write path and puts the name in a standard format including timestamp etc.
     *
     * @param fileName the name of the file (snapshot, recording, liver_image, ...)
     * @param frameNumber the acquisition frame number provided by ximea
     * @param extension file extension (.dat or .tif)
     * @param specialFolder sometimes we want to add an additional layer of subfolder, specifically when saving white/dark balance images
     * @return
     */
    QString GetFullFilenameStandardFormat(std::string fileName, long frameNumber, std::string extension, std::string specialFolder="");
    QString GetBaseFolder() const;
    QString m_topFolderName;
    QString m_recPrefixlineEdit;
    QString m_folderLowExposureImages;
    QString m_triggerText;
    QString m_baseFolderLoc;
    std::string m_recBaseName;
    QString m_date;
    QElapsedTimer m_elapsedTimer;
    float m_elapsedTime;

    ImageContainer m_imageContainer;
    CameraInterface m_camInterface;
    Network m_network;
    Displayer* m_display;

    // if testmode is on, recording will always be saved to the same filename. This allows long time testing
    // of camera recording
    bool m_testMode;

    // threads for camera image display and recording
    boost::thread m_image_container_thread;
    boost::asio::io_service m_io_service;
    boost::thread_group m_threadpool;
    boost::asio::io_service::work m_work;
    boost::mutex mtx_;
};

#endif // MAINWINDOW_H
