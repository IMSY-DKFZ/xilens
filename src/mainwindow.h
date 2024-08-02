/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QCloseEvent>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMainWindow>
#include <QScreen>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "cameraInterface.h"
#include "display.h"
#include "xiAPIWrapper.h"

/**
 * Main window namespace
 */
namespace Ui
{
class MainWindow;
}

/**
 * Main window class declaration.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = 0, std::shared_ptr<XiAPIWrapper> xiAPIWrapper = nullptr);

    ~MainWindow();

    /**
     * Queries if normalization should be applied to the displayed images
     */
    bool GetNormalize() const;

    /**
     * Queries the band number to be displayed
     */
    virtual unsigned GetBand() const;

    /**
     * Queries the normalization factor to be used
     */
    unsigned GetBGRNorm() const;

    /**
     * Enables the UI elements
     */
    void EnableUi(bool enable);

    /**
     * Disables the UI elements
     */
    void EnableWidgetsInLayout(QLayout *layout, bool enable);

    /**
     * Writes general information as header of the log file
     */
    void WriteLogHeader();

    /**
     * Logs message to log file and returns the timestamp used during logging
     */
    QString LogMessage(QString message, QString logFile, bool logTime);

    /**
     * Queries the path where the logfile is stored
     *
     * @return
     */
    QString GetLogFilePath(QString logFile);

    /**
     * Logs camera temperature to log file
     */
    void LogCameraTemperature();

    /**
     * Displays camera temperature on an LCD display
     */
    void DisplayCameraTemperature();

    /**
     * Creates schedule for the thread in charge of logging temperature of the
     * camera
     */
    void ScheduleTemperatureThread();

    /**
     * Starts the thread in charge of logging camera temperature
     */
    void StartTemperatureThread();

    /**
     * Stops thread in charge of logging camera temperature
     */
    void StopTemperatureThread();

    /**
     * Handle for timer used to schedule camera temperature logging
     */
    void HandleTemperatureTimer(const boost::system::error_code &error);

    /**
     * Stops thread in charge of recording snapshot images
     */
    void StopSnapshotsThread();

    /**
     * @brief Updates the saturation percentage on the LCD displays.
     *
     * @param image The input image of type CV_8UC1. It must be non-empty.
     * @throws std::invalid_argument if the input matrix is empty or of the wrong
     * type.
     */
    void UpdateSaturationPercentageLCDDisplays(cv::Mat &image) const;

  protected:
    /**
     * @brief Event handler for the close event of the main window.
     *
     * This method is called when the user attempts to close the main window
     * either by clicking the close button or using the system shortcut. It is
     * responsible for handling any necessary cleanup or actions before the
     * application closes. If a recording is running when the close event is
     * triggered, the recordings are first stopped to ensure no loss of data
     * happens.
     *
     * @param event A pointer to the event object representing the close event.
     */
    void closeEvent(QCloseEvent *event);

  private slots:

    /**
     * Qt slot triggered when the snapshot button is pressed. Triggers the
     * recording of snapshot images or stops it when pressed a second time.
     */
    void on_snapshotButton_clicked();

    /**
     * Qt slot triggered when the camera exposure slider is modified.
     */
    void on_exposureSlider_valueChanged(int value);

    /**
     * Qt slot triggered when the record button is pressed. Stars the continuous
     * recording of images to files and stops it when pressed a second time. This
     * is synchronized with the exposure time label.
     */
    void on_recordButton_clicked(bool clicked);

    /**
     * Qt slot triggered when the button to choose a base folder is clicked. Opens
     * a dialog where a folder can be selected.
     */
    void on_baseFolderButton_clicked();

    /**
     * Qt slot triggered when the exposure time labels is modified manually. This
     * changes the appearance of the field but does not trigger the change in the
     * camera. Return key needs to be pressed for the change to be applied.
     */
    void on_exposureLineEdit_textEdited(const QString &arg1);

    /**
     * Qt slot triggered when return key is pressed after modifying the exposure
     * time. This is synchronized with the exposure time slider.
     */
    void on_exposureLineEdit_returnPressed();

    /**
     * Qt slot triggered when return key is pressed on the field where the top
     * folder is defined in the UI. It updates the member variable that stores the
     * value.
     */
    void on_subFolderLineEdit_returnPressed();

    /**
     * Qt slot triggered when the return key is pressed on the field tha tdefines
     * the file prefix in the UI. It updates the member variable that stores the
     * value.
     */
    void on_filePrefixLineEdit_returnPressed();

    /**
     * Qt slot triggered when auto exposure checkbox is pressed. Handles control
     * of the exposure time to camera.
     */
    void on_autoexposureCheckbox_clicked(bool setAutoexposure);

    /**
     * Qt slot triggered when white balance button is pressed. Records a new white
     * image and sets it in the network model.
     */
    void on_whiteBalanceButton_clicked();

    /**
     * Records the white reference to a folder called "white"
     */
    void RecordReferenceImages(QString referenceType);

    /**
     * Stops the thread responsible for recording the reference images (white and
     * dark)
     */
    void StopReferenceRecordingThread();

    /**
     * Qt slot triggered when the dark correction button is pressed. Records a new
     * dark image and sets it in the network model.
     */
    void on_darkCorrectionButton_clicked();

    /**
     * Qt slot triggered when editing of the blood volume fraction minimum value.
     */
    void on_minVhbLineEdit_textEdited(const QString &newText);

    /**
     * Qt slot triggered when return key is pressed on the minimum vhb element in
     * the UI.
     */
    void on_minVhbLineEdit_returnPressed();

    /**
     * Qt slot triggered when editing of the blood volume fraction maximum value.
     */
    void on_maxVhbLineEdit_textEdited(const QString &newText);

    /**
     * Qt slot triggered when editing of the blood volume fraction maximum value
     * has finished.
     */
    void on_maxVhbLineEdit_returnPressed();

    /**
     * Qt slot triggered when editing of the oxygenation minimum value.
     */
    void on_minSao2LineEdit_textEdited(const QString &newText);

    /**
     * Qt slot triggered when editing of the oxygenation minimum value has
     * finished.
     */
    void on_minSao2LineEdit_returnPressed();

    /**
     * Qt slot triggered when editing of the oxygenation maximum value.
     */
    void on_maxSao2LineEdit_textEdited(const QString &newText);

    /**
     * Qt slot triggered when editing of the oxygenation maximum value has
     * finished.
     */
    void on_maxSao2LineEdit_returnPressed();

    /**
     * Qt slot triggered when editing the top folder name. It changes the
     * appearance of the field in the UI. It does not change the value of hte
     * member variable that contains the top folder name.
     */
    void on_subFolderLineEdit_textEdited(const QString &newText);

    /**
     * Qt slot triggered when the prefix file name is edited. It changes the
     * appearance of the field in the UI. It does not change the value of the
     * member variable that stores the file prefix name.
     */
    void on_filePrefixLineEdit_textEdited(const QString &newText);

    /**
     * Qt slot triggered when the "functional" radio button is pressed. It
     * activates the display corresponding to functional images (model output).
     */
    void on_functionalRadioButton_clicked();

    /**
     * Qt slot triggered when the "raw" display is pressed. Displays only the raw
     * image in higher resolution compared to the "functional" display.
     */
    void on_rawRadioButton_clicked();

    /**
     * Qt slot triggered when the trigger text is edited. It only changes the
     * appearance of the UI element.
     */
    void on_logTextLineEdit_textEdited(const QString &newText);

    /**
     * Qt slot triggered when the return key is pressed on the trigger text field.
     * It logs the message to the log file and displays it on the UI.
     */
    void on_logTextLineEdit_returnPressed();

    /**
     * Qt slot triggered when the spin box containing the number of images to skip
     * while recording is modified. It restyles the appearance of the field.
     */
    void on_skipFramesSpinBox_valueChanged();

    /**
     * Qt slot triggered when a new camera is selected from the drop-down menu.
     */
    void on_cameraListComboBox_currentIndexChanged(int index);

    /**
     * Updates the stile of a Qt LineEdit component.
     *
     * @param lineEdit element to update
     * @param newString new value received from element
     * @param originalString original value of hte element before changes occurred
     */
    void UpdateComponentEditedStyle(QLineEdit *lineEdit, const QString &newString, const QString &originalString);

    /**
     * Restores the appearance of a Qt LineEdit component.
     */
    void RestoreLineEditStyle(QLineEdit *lineEdit);

    /**
     * Qt slot triggered when file name prefix for snapshots is edited on the UI.
     */
    void on_filePrefixExtrasLineEdit_textEdited(const QString &newText);

    /**
     * Qt slot triggered when the return key is pressed on the file prefix field
     * for snapshot images in the UI.
     */
    void on_filePrefixExtrasLineEdit_returnPressed();

    /**
     * Qt slot triggered when extras sub folder field is edited in the UI.
     */
    void on_subFolderExtrasLineEdit_textEdited(const QString &newText);

    /**
     * Qt slot triggered when the return key is pressed on the sub folder field in
     * the extras tab in the UI.
     */
    void on_subFolderExtrasLineEdit_returnPressed();

  private:
    Ui::MainWindow *ui;

    /**
     * @brief Displays a new image
     */
    void Display();

    /**
     * Starts the recording process
     */
    void StartRecording();

    /**
     * Stops the recording process
     */
    void StopRecording();

    /**
     * Starts the thread in charge of polling the images from the camera.
     */
    void StartPollingThread();

    /**
     * Stops the thread in charge of polling the images from the camera.
     */
    void StopPollingThread();

    /**
     * Sets the base folder path where the data is to be stored.
     */
    bool SetBaseFolder(QString baseFolderPath);

    /**
     * Creates a folder if it does not exist
     */
    void CreateFolderIfNecessary(QString folder);

    /**
     * Records image to specified sub folder and using specified file prefix to
     * name the file
     *
     * @param ignoreSkipping ignores the number of frames to skip and stores the
     * image anyways
     */
    void RecordImage(bool ignoreSkipping);

    /**
     * Starts IO service in a thread in charge of saving the images to files.
     */
    void ThreadedRecordImage();

    /**
     * Initializes the file object inside the image container. This object is used
     * to store all images while recording to a single file.
     *
     * @param subFolder folder where data will be stored
     * @param filePrefix file prefix used for the file name
     */
    void InitializeImageFileRecorder(std::string subFolder = "", std::string filePrefix = "");

    /**
     * Displays the number of recorded images in the GUI
     */
    void DisplayRecordCount();

    /**
     * Indicates if an image should be recorded to file or not depending on the
     * frame number and the number of frames to skip
     *
     * @param nSkipFrames number of frames to skip
     * @param ImageID frame number
     * @return true if image should be recorded to file or false if not
     */
    bool ImageShouldBeRecorded(int nSkipFrames, long ImageID);

    /**
     * Updates image counter
     */
    void CountImages();

    /**
     * Updates timer displayed on the UI when recordings are started.
     */
    void updateTimer();

    /**
     * Stops the timer that is displayed in the UI when recordings are started.
     */
    void stopTimer();

    /**
     *  Initializes the DL network in charge of estimating the functional
     * properties.
     */
    void RunNetwork();

    /**
     * @brief RecordSnapshots helper method to take snapshots, basically just
     * created to be able to thread the snapshot making :-)
     */
    void RecordSnapshots();

    /**
     * Updates the blood volume fraction and oxygenation value validators.
     */
    void UpdateVhbSao2Validators();

    /**
     * @brief UpdateExposure Synchronizes the sliders and textedits displaying the
     * current exposure setting
     */
    void UpdateExposure();

    /**
     * Enables and disables elements of the GUI that should not me modified while
     * recordings are in progress
     *
     * @param recordingInProgress
     */
    void HandleElementsWhileRecording(bool recordingInProgress);

    /**
     * @brief MainWindow::GetWritingFolder returns the folder there the image
     * files are written to
     * @return
     */
    QString GetWritingFolder();

    /**
     * @brief GetFullFilenameStandardFormat returns the full filename of the
     * current file which shall be written
     *
     * It automatically add the current write path and puts the name in a standard
     * format including timestamp etc.
     *
     * @param filePrefix the name of the file (snapshot, recording, liver_image,
     * ...)
     * @param frameNumber the acquisition frame number provided by ximea
     * @param extension file extension (.dat or .tif)
     * @param subFolder sometimes we want to add an additional layer of subfolder,
     * specifically when saving white/dark balance images
     * @return
     */
    QString GetFullFilenameStandardFormat(std::string &&filePrefix, const std::string &extension,
                                          std::string &&subFolder);

    /**
     * Queries the base folder path where data is to be stored.
     */
    QString GetBaseFolder() const;

    /**
     * Starts image acquisition by initializing image contained and displayer.
     */
    void StartImageAcquisition(QString camera_identifier);

    /**
     * Stops image acquisition by disconnecting image displayer and stopping image
     * polling to the image container.
     */
    void StopImageAcquisition();

    /**
     * Formats timestamp tag from format  yyyyMMdd_HH-mm-ss-zzz into a human
     * readable format
     *
     * @param timestamp
     * @return
     */
    QString FormatTimeStamp(QString timestamp);

    /**
     * stores the folder name where images are to be stores. This is a folder
     * inside of base folder.
     */
    QString m_subFolder;

    /**
     * file prefix to be appended to each image file name.
     */
    QString m_recPrefixlineEdit;

    /**
     * Folder path where extra images are to be stored (e.g. snapshots). This is a
     * folder inside the base folder.
     */
    QString m_extrasSubFolder;

    /**
     * Trigger text entered to the log function of the UI.
     */
    QString m_triggerText;

    /**
     * Folder path where all data is to be stored.
     */
    QString m_baseFolderLoc;

    /**
     * Value of exposure time for the camera.
     */
    QString m_labelExp;

    /**
     * File prefix used for snapshot images.
     */
    QString m_extrasFilePrefix;

    /**
     * Elapsed timer used for the timer displayed in the UI.
     */
    QElapsedTimer m_elapsedTimer;

    /**
     * Time elapsed since recordings started.
     */
    float m_elapsedTime;

    /**
     * Time elapsed since recordings started as text field.
     */
    QString m_elapsedTimeText;

    /**
     * Text stream used to generate the elapsed time text.
     */
    QTextStream m_elapsedTimeTextStream;

    /**
     * Minimum blood volume fraction value used for display.
     */
    QString m_minVhb;

    /**
     * Maximum blood volume fraction value used for display.
     */
    QString m_maxVhb;

    /**
     * Minimum oxygenation value used for display.
     */
    QString m_minSao2;

    /**
     * Maximum oxygenation value used for display.
     */
    QString m_maxSao2;

    /**
     * Image container where each new image from the camera is stored
     */
    ImageContainer m_imageContainer;

    /**
     * Camera interface. Handles communication with each connected camera.
     */
    CameraInterface m_cameraInterface;

    /**
     * Wrapper to xiAPI, useful for mocking during testing
     */
    std::shared_ptr<XiAPIWrapper> m_xiAPIWrapper = std::make_shared<XiAPIWrapper>();

    /**
     * Display in charge of displaying each image.
     */
    Displayer *m_display;

    /**
     * Handles if test mode should be set for the program. All images are stored
     * to same file.
     */
    bool m_testMode;

    /**
     * Thread in charge of running the Image container.
     */
    boost::thread m_imageContainerThread;

    /**
     * IO service in charge of recording images to files.
     */
    boost::asio::io_service m_IOService;

    /**
     * Work object to control safe finish of IOService
     */
    std::unique_ptr<boost::asio::io_service::work> m_IOWork;

    /**
     * ID service for recording temperature to file
     */
    boost::asio::io_service m_temperatureIOService;

    /**
     * Async IO work. Keeps the IO service alive in the thread in charge of
     * temperature recording.
     */
    std::unique_ptr<boost::asio::io_service::work> m_temperatureIOWork;

    /**
     * Thread pool used for recording the data.
     */
    boost::thread_group m_threadGroup;

    /**
     * Mutual exclusion mechanism in charge of synchronization.
     */
    boost::mutex mtx_;

    /**
     * Camera temperature recording thread.
     */
    boost::thread m_temperatureThread;

    /**
     * Snapshot image recording thread.
     */
    boost::thread m_snapshotsThread;

    /**
     * thread where white and dark references are recorded
     */
    boost::thread m_referenceRecordingThread;

    /**
     * Thread containing the timer for temperature recording at certain intervals.
     */
    std::shared_ptr<boost::asio::steady_timer> m_temperatureThreadTimer;

    /**
     * Counts how many images have been recorded.
     */
    std::atomic<unsigned long> m_recordedCount;

    /**
     * Counts how many images whould have been recorded
     */
    std::atomic<unsigned long> m_imageCounter;

    /**
     * Counts how many images were skipped during the recording process.
     */
    std::atomic<unsigned long> m_skippedCounter;
};

#endif // MAINWINDOW_H
