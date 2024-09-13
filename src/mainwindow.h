/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QCloseEvent>
#include <QElapsedTimer>
#include <QGraphicsScene>
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
    explicit MainWindow(QWidget *parent = nullptr, const std::shared_ptr<XiAPIWrapper> &xiAPIWrapper = nullptr);

    ~MainWindow() override;

    /**
     * Queries if normalization should be applied to the displayed images.
     */
    bool GetNormalize() const;

    /**
     * Queries the band number to be displayed.
     */
    virtual unsigned GetBand() const;

    /**
     * Queries the normalization factor to be used.
     */
    unsigned GetBGRNorm() const;

    /**
     * Enables the UI elements.
     *
     * @param enable indicates if UI is enabled or not.
     */
    void EnableUi(bool enable);

    /**
     * Configures custom UI elements such as custom icons in buttons, etc.
     */
    void SetUpCustomUiComponents();

    /**
     * Disables the UI elements.
     *
     * @param layout layout where elements will be enabled or disabled.
     * @param enable indicates if elements should ne enabled or disabled.
     */
    void EnableWidgetsInLayout(QLayout *layout, bool enable);

    /**
     * Writes general information as header of the log file.
     */
    void WriteLogHeader();

    /**
     * Logs message to log file and returns the timestamp used during logging.
     *
     * @param message message to be logged.
     * @param logFile file name where the message should be logged.
     * @param logTime whether time should be logged too or not.
     */
    QString LogMessage(const QString &message, const QString &logFile, bool logTime);

    /**
     * Queries the path where the logfile is stored.
     *
     * @param logFile file name.
     * @return path to file.
     */
    QString GetLogFilePath(const QString &logFile);

    /**
     * Gets camera temperature.
     *
     * @return mapper containing the camera temperature with keys identifying the location on the camera where the
     * temperature was queried from.
     */
    QMap<QString, float> GetCameraTemperature() const;

    /**
     * Displays camera temperature on an LCD display.
     */
    void DisplayCameraTemperature();

    /**
     * Creates schedule for the thread in charge of logging temperature of the camera.
     */
    void ScheduleTemperatureThread();

    /**
     * Starts the thread in charge of logging camera temperature.
     */
    void StartTemperatureThread();

    /**
     * Stops thread in charge of logging camera temperature.
     */
    void StopTemperatureThread();

    /**
     * Handle for timer used to schedule camera temperature logging.
     *
     * @param error type of error expected to cancel the timer.
     */
    void HandleTemperatureTimer(const boost::system::error_code &error);

    /**
     * Stops thread in charge of recording snapshot images.
     */
    void StopSnapshotsThread();

    /**
     * Updates the frames per second that are stored to file on the UI.
     */
    void UpdateFPSLCDDisplay();

    /**
     * Updates the raw image displayed in the viewer tab.
     *
     * @param image OpenCV patrix to display.
     */
    void UpdateRawViewerImage(cv::Mat &image);

    /**
     * Waits for the viewer thread to be running and for new values to be available in the queue. It emits a
     * signal indicating that a new value can be processed.
     */
    void ViewerWorkerThreadFunc();

    /**
     * Takes an image, and scales it to the available width in the QtGraphicsView element before displaying it in the
     * provided scene.
     *
     * @param image OpenCV matrix to be displayed.
     * @param format format expected of the image, for example `QImage::Format_RGB888` for an 8bit RGB image.
     * @param view the graphics view element where image will be displayed.
     * @param pixmapItem pixmap item where the image is to be placed.
     * @param scene the scene that will contain the pixmap.
     */
    static void UpdateImage(cv::Mat &image, QImage::Format format, QGraphicsView *view,
                            std::unique_ptr<QGraphicsPixmapItem> &pixmapItem, QGraphicsScene *scene);

    /**
     * Identifies if the saturation tool button is checked or not.
     *
     * @return true if the saturation button is checked, false otherwise.
     */
    bool IsSaturationButtonChecked();

    /**
     * Provides access to the applications user interface.
     *
     * @return pointer tot he `Ui::MainWindow` from which all Qt component in the user interface can be accessed.
     */
    Ui::MainWindow *GetUI() const
    {
        return ui;
    }

    /**
     * Sets the number of images recorded.
     *
     * @param count number of recorded images.
     */
    void SetRecordedCount(int count);

    /**
     * Displays the number of recorded images in the GUI.
     */
    void DisplayRecordCount();

  protected:
    /**
     * Main access point to all Qt components in the user interface. All Qt components in the UI can be accessed through
     * this pointer.
     */
    Ui::MainWindow *ui;

    /**
     * This is the NDArray structure that holds the connection to the data viewed in the Viewer tab of the application.
     */
    b2nd_array_t *m_viewerNDArray = nullptr;

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
    void closeEvent(QCloseEvent *event) override;

  signals:
    /**
     * Qt signal that is emitted when reading an processing of the image to display in viewer tab is finished.
     *
     * @param mat OpenCV matrix containing the image to display. This should be a one channel image.
     */
    void ViewerImageProcessingComplete(cv::Mat &mat);

  public slots:
    /**
     * Qt slot that updates the RGB image displayed in the GUI.
     *
     * @param image OpenCv matrix containing an 8bit (per channel) RGB image to be displayed.
     */
    void UpdateRGBImage(cv::Mat &image);

    /**
     * Qt slot that updates the raw image displayed in the GUI.
     *
     * @param image OpenCV matrix containing an 8bit single channel image to be displayed.
     */
    void UpdateRawImage(cv::Mat &image);

    /**
     * Qt slot that updates the saturation percentage on the LCD displays.
     *
     * @param image The input image of type CV_8UC1. It must be non-empty.
     * @throws std::invalid_argument if the input matrix is empty or of the wrong type.
     */
    void UpdateSaturationPercentageLCDDisplays(cv::Mat &image) const;

  private slots:

    /**
     * Qt slot triggered when the snapshot button is pressed. Triggers the
     * recording of snapshot images or stops it when pressed a second time.
     */
    void HandleSnapshotButtonClicked();

    /**
     * Qt slot triggered when the camera exposure value is modified either from the slider or the spinbox.
     *
     * @param value exposure value.
     */
    void HandleExposureValueChanged(int value);

    /**
     * Qt slot triggered when the image index slider in the Viewer tab of the application changes value.
     *
     * @param value The new value of the slider.
     */
    void HandleViewerImageSliderValueChanged(int value);

    /**
     * Qt slot triggered when the record button is pressed. Stars the continuous
     * recording of images to files and stops it when pressed a second time. This
     * is synchronized with the exposure time label.
     *
     * @param clicked indicates if the button is clicked.
     */
    void HandleRecordButtonClicked(bool clicked);

    /**
     * Qt slot triggered when the button to choose a base folder is clicked. Opens
     * a dialog where a folder can be selected.
     */
    void HandleBaseFolderButtonClicked();

    /**
     * Qt slot triggered when the file button in the viewer tab is clicked. Opens
     * a dialog where a file can be selected.
     */
    void HandleViewerFileButtonClicked();

    /**
     * Qt slot triggered when the return key is pressed on the field that defines
     * the file name in the UI. It updates the member variable that stores the
     * value.
     */
    void HandleFileNameLineEditReturnPressed();

    /**
     * Qt slot triggered when the file name is edited. It changes the
     * appearance of the field in the UI. It does not change the value of the
     * member variable that stores the file name.
     *
     * @param newText edited text.
     */
    void HandleFileNameLineEditTextEdited(const QString &newText);

    /**
     * Qt slot triggered when auto exposure checkbox is pressed. Handles control
     * of the exposure time to camera.
     */
    void HandleAutoexposureCheckboxClicked(bool setAutoexposure);

    /**
     * Qt slot triggered when white balance button is pressed. Records a new white
     * image and sets it in the network model.
     */
    void HandleWhiteBalanceButtonClicked();

    /**
     * Qt slot triggered when the dark correction button is pressed. Records a new
     * dark image and sets it in the network model.
     */
    void HandleDarkCorrectionButtonClicked();

    /**
     * Qt slot triggered when the trigger text is edited. It only changes the
     * appearance of the UI element.
     *
     * @param newText edited text.
     */
    void HandleLogTextLineEditTextEdited(const QString &newText);

    /**
     * Qt slot triggered when the return key is pressed on the trigger text field.
     * It logs the message to the log file and displays it on the UI.
     */
    void HandleLogTextLineEditReturnPressed();

    /**
     * Qt slot triggered when the spin box containing the number of images to skip
     * while recording is modified. It restyles the appearance of the field.
     */
    void HandleSkipFramesSpinBoxValueChanged();

    /**
     * Qt slot triggered when a new camera is selected from the drop-down menu.
     *
     * @param index index corresponding to the element selected from the combo box.
     */
    void HandleCameraListComboBoxCurrentIndexChanged(int index);

    /**
     * Checks for connected XIMEA cameras and populates the dropdown list of available cameras.
     */
    void HandleReloadCamerasPushButtonClicked();

    /**
     * Qt slot triggered when file name for snapshots is edited on the UI.
     *
     * @param newText edited text.
     */
    void HandleFileNameSnapshotsLineEditTextEdited(const QString &newText);

    /**
     * Qt slot triggered when the return key is pressed on the file name field
     * for snapshot images in the UI.
     * This method will show a en error message box when the name of the snapshot file is the same as the file
     * where the video is to be recorded.
     */
    void HandleFileNameSnapshotsLineEditReturnPressed();

    /**
     * Qt slot triggered when the return key is pressed on the base folder field line edit in the UI.
     */
    void HandleBaseFolderLineEditReturnPressed();

    /**
     * Qt slot triggered when base folder field is edited in the UI.
     *
     * @param newText edited text.
     */
    void HandleBaseFolderLineEditTextEdited(const QString &newText);

    /**
     * Qt slot triggered when the file path in the viewer tab is edited through the UI.
     *
     * @param newText edited text
     */
    void HandleViewerFileLineEditTextEdited(const QString &newText);

    /**
     * Qt slot triggered when the return key is pressed in the file path field of the viewer tab.
     */
    void HandleViewerFileLineEditReturnPressed();

  private:
    /**
     * Setups all UI Qt connections to handle all user interactions with the UI.
     */
    void SetUpConnections();

    /**
     * Handles the result emanating from a Qt connection attempt.
     *
     * @param status status returned by `QObject::connect`.
     * @param file file that calls this method.
     * @param line line where this method is called from.
     * @param func function name that calls this method.
     */
    static void HandleConnectionResult(bool status, const char *file, int line, const char *func);

    /**
     * Records the white reference to a folder called "white".
     *
     * @param referenceType type of reference `white` or `dark`.
     */
    void RecordReferenceImages(const QString &referenceType);

    /**
     * Stops the thread responsible for recording the reference images (white and dark).
     */
    void StopReferenceRecordingThread();

    /**
     * Updates the stile of a Qt LineEdit component.
     *
     * @param lineEdit element to update.
     * @param newString new value received from element.
     * @param originalString original value of hte element before changes occurred.
     */
    static void UpdateComponentEditedStyle(QLineEdit *lineEdit, const QString &newString,
                                           const QString &originalString);

    /**
     * Restores the appearance of a Qt LineEdit component.
     *
     * @param lineEdit line edit for which style should be restored.
     */
    static void RestoreLineEditStyle(QLineEdit *lineEdit);

    /**
     * @brief Displays a new image.
     */
    void Display();

    /**
     * Starts the recording process.
     */
    void StartRecording();

    /**
     * Stops the recording process.
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
     * Creates a folder if it does not exist.
     *
     * @param folder the path to folder that needs to be created.
     */
    static void CreateFolderIfNecessary(const QString &folder);

    /**
     * Records image to specified sub folder and using specified file name.
     *
     * @param ignoreSkipping ignores the number of frames to skip and stores the
     * image anyways.
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
     * @param subFolder folder where data will be stored.
     * @param fileName file name.
     */
    void InitializeImageFileRecorder(std::string subFolder = "", std::string fileName = "");

    /**
     * Indicates if an image should be recorded to file or not depending on the
     * frame number and the number of frames to skip.
     *
     * @param nSkipFrames number of frames to skip.
     * @param ImageID frame number.
     * @return true if image should be recorded to file or false if not.
     */
    static bool ImageShouldBeRecorded(int nSkipFrames, long ImageID);

    /**
     * Updates image counter.
     */
    void CountImages();

    /**
     * Updates timer displayed on the UI when recordings are started.
     */
    void UpdateTimer();

    /**
     * Stops the timer that is displayed in the UI when recordings are started.
     */
    void StopTimer();

    /**
     * @brief MEthod used to record singe snapshot images while recording.
     */
    void RecordSnapshots();

    /**
     * @brief UpdateExposure Synchronizes the sliders and text edits displaying
     * the current exposure setting.
     */
    void UpdateExposure();

    /**
     * Enables and disables elements of the GUI that should not me modified while
     * recordings are in progress.
     *
     * @param recordingInProgress indicates if recordings are happening or not.
     */
    void HandleElementsWhileRecording(bool recordingInProgress);

    /**
     * @brief MainWindow::GetWritingFolder returns the folder there the image
     * files are written to.
     *
     * @return folder where data is to be stored.
     */
    QString GetWritingFolder();

    /**
     * @brief GetFullFilenameStandardFormat returns the full filename of the
     * current file which shall be written.
     *
     * It automatically add the current write path and puts the name in a standard
     * format including timestamp etc.
     *
     * @param fileName the name of the file (snapshot, recording, liver_image, ...).
     * @param frameNumber the acquisition frame number provided by ximea.
     * @param extension file extension (.b2nd).
     * @param subFolder sometimes we want to add an additional layer of subfolder.
     * specifically when saving white/dark balance images.
     * @return
     */
    QString GetFullFilenameStandardFormat(std::string &&fileName, const std::string &extension,
                                          std::string &&subFolder);

    /**
     * Queries the base folder path where data is to be stored.
     */
    QString GetBaseFolder() const;

    /**
     * Starts image acquisition by initializing image contained and displayer.
     */
    void StartImageAcquisition(QString cameraIdentifier);

    /**
     * Stops image acquisition by disconnecting image displayer and stopping image
     * polling to the image container.
     */
    void StopImageAcquisition();

    /**
     * Formats timestamp tag from format  yyyyMMdd_HH-mm-ss-zzz into a human
     * readable format.
     *
     * @param timestamp time stamp to be formatted.
     * @return formatted timestamp.
     */
    static QString FormatTimeStamp(const QString &timestamp);

    /**
     * Opens the N-dimensional array and adjusts UI components based on the contents of the file:
     *
     * - Adjusts range of viewer image slider.
     * - Triggers the display of the first image in the file.
     *
     * @param filePath path to the file to open.
     */
    void OpenFileInViewer(const QString &filePath);

    /**
     * Reads a single image slice from file and creates an OpenCv matrix containing the data of the image.
     * It emits a signal indicating that the processing finished and provides the processed image through the signal.
     *
     * @param value image index to load from file
     */
    void ProcessViewerImageSliderValueChanged(int value);

    /**
     * Sets the scene for RGB and raw image viewers. It defines antialiasing and smooth pixmap transformations.
     */
    void SetGraphicsViewScene();

    /**
     * Appends the current time to que of recorded time stamps that can be used to calculate the frames per second.
     */
    void RegisterTimeImageRecorded();

    /**
     * The file name where videos are to be stored.
     */
    QString m_fileName;

    /**
     * Trigger text entered to the log function of the UI.
     */
    QString m_triggerText;

    /**
     * Folder path where all data is to be stored.
     */
    QString m_baseFolderPath;

    /**
     * Path to a .b2nd file to be displayed in the viewer tab.
     */
    QString m_viewerFilePath;

    /**
     * Value of exposure time for the camera.
     */
    QString m_labelExp;

    /**
     * File name used for snapshot images.
     */
    QString m_snapshotsFileName;

    /**
     * Elapsed timer used for the timer displayed in the UI.
     */
    QElapsedTimer m_elapsedTimer;

    /**
     * Time elapsed since recordings started.
     */
    double m_elapsedTime;

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
     * Image container where each new image from the camera is stored.
     */
    ImageContainer m_imageContainer;

    /**
     * Camera interface. Handles communication with each connected camera.
     */
    CameraInterface m_cameraInterface;

    /**
     * Wrapper to xiAPI, useful for mocking during testing.
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
     * Thread in charge of handling the image viewer processing before displaying it in the UI.
     */
    boost::thread m_viewerThread;

    /**
     * Queue used to store image indices that are then processed to load the corresponding images.
     */
    std::queue<int> m_viewerSliderQueue;

    /**
     * Mutex used as a locking mechanism to avoid raises when processing images for the Viewer tab.
     */
    boost::mutex m_mutexImageViewer;

    /**
     * Primitive used to lock viewer thread execution until the que is not empty and when the thread has to stop.
     */
    boost::condition_variable m_viewerQueueCondition;

    /**
     * Indicates if the thread in charge of processing and displaying images in the viewer tab is running.
     */
    bool m_viewerThreadRunning;

    /**
     * IO service in charge of recording images to files.
     */
    boost::asio::io_service m_IOService;

    /**
     * Work object to control safe finish of IOService.
     */
    std::unique_ptr<boost::asio::io_service::work> m_IOWork;

    /**
     * ID service for recording temperature to file.
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
    boost::mutex m_mutexImageRecording;

    /**
     * Camera temperature recording thread.
     */
    boost::thread m_temperatureThread;

    /**
     * Snapshot image recording thread.
     */
    boost::thread m_snapshotsThread;

    /**
     * thread where white and dark references are recorded.
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
     * Counts how many images would have been recorded.
     */
    std::atomic<unsigned long> m_imageCounter;

    /**
     * Counts how many images were skipped during the recording process.
     */
    std::atomic<unsigned long> m_skippedCounter;

    /**
     * Container to store the time stamps when a new image is recorded. This is used to calculate the FPS.
     */
    std::deque<std::chrono::steady_clock::time_point> m_recordedTimestamps;

    /**
     * Smart pointer to the RGB scene where the RGB images will be displayed.
     */
    std::unique_ptr<QGraphicsScene> m_rgbScene = std::make_unique<QGraphicsScene>(this);

    /**
     * Smart pointer to raw scene where the raw unprocessed images will be displayed.
     */
    std::unique_ptr<QGraphicsScene> m_rawScene = std::make_unique<QGraphicsScene>(this);

    /**
     * Smart pointer to a scene where the images for the Viewer tab are displayed.
     */
    std::unique_ptr<QGraphicsScene> m_rawViewerScene = std::make_unique<QGraphicsScene>(this);

    /**
     * Smart pointer to pixmap used to display the RGB images.
     */
    std::unique_ptr<QGraphicsPixmapItem> m_rgbPixMapItem;

    /**
     * Smart pointer to pixmap where raw unprocessed images will be displayed.
     */
    std::unique_ptr<QGraphicsPixmapItem> m_rawPixMapItem;

    /**
     * Smart Pointer to a pixmap used to display the images in the RawViewer.
     */
    std::unique_ptr<QGraphicsPixmapItem> m_rawViewerPixMapItem;

    /**
     * Timer that sets the rate of updates for the FPS LCD Display in the UI.
     */
    QTimer *m_updateFPSDisplayTimer;
};

#endif // MAINWINDOW_H
