/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QCloseEvent>
#include <QGraphicsScene>
#include <QMainWindow>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "cameraInterface.h"
#include "display.h"
#include "widgets.h"
#include "xiAPIWrapper.h"

/**
 * @brief Forward declares MainWindow class inside UI namespace.
 *
 * This namespace contains auto-generated classes that correspond to UI forms designed in `Qt Designer`.
 * This is the main access point for all UI components of the application.
 * The full definition of this namespace can be found in the auto-generated file `ui_mainwindow.h`, which is available
 * after compilation in the build directory.
 */
namespace Ui
{
class MainWindow;
}

/**
 * @brief Class used to manage all UI component interactions as well as displaying the images queried from cameras.
 *
 * This class is in charge of initializing the UI components and handle any interactions with the user as well as
 * displaying the images queried from the camera. During initialization of the UI, this class performs the following
 * steps:
 *
 * - Initialize camera interface and image container.
 * - Setup native and custom UI components.
 * - Initialize BLOSC2.
 * - Populates list of available cameras.
 * - Established `Qt` connections.
 * - Disables UI components until a camera is selected by the user.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr, const std::shared_ptr<XiAPIWrapper> &xiAPIWrapper = nullptr);

    ~MainWindow() override;

    /**
     * @brief Queries if normalization should be applied to the displayed images.
     */
    bool GetNormalize() const;

    /**
     * @brief Queries the band number to be displayed.
     */
    virtual unsigned GetBand() const;

    /**
     * @brief Retrieves the minimum saturation value allowed by the saturation spin boxes.
     *
     * This method provides the lower boundary of the saturation adjustment range
     * stored by the saturation spin boxes in the UI.
     *
     * @return The minimum saturation value as an integer.
     */
    virtual int GetSaturationMinValue() const;

    /**
     * @brief Retrieves the maximum allowable value for the saturation adjustment.
     *
     * This function queries and returns the highest saturation value stored
     * by the saturation control component. It is used to determine the upper
     * limit of the saturation adjustment range in the UI.
     *
     * @return The maximum saturation value supported.
     */
    virtual int GetSaturationMaxValue() const;

    /**
     * Queries the normalization factor to be used.
     */
    unsigned GetBGRNorm() const;

    /**
     * @brief Enables the UI elements.
     *
     * @param enable indicates if UI is enabled or not.
     */
    void EnableUi(bool enable) const;

    /**
     * @brief Configures custom UI elements such as custom icons in buttons, etc.
     */
    void SetUpCustomUiComponents() const;

    /**
     * @brief Disables the UI elements.
     *
     * @param layout layout where elements will be enabled or disabled.
     * @param enable indicates if elements should ne enabled or disabled.
     */
    static void EnableWidgetsInLayout(const QLayout *layout, bool enable);

    /**
     * Writes general information as header of the log file.
     */
    void WriteLogHeader() const;

    /**
     * @brief Logs message to log file and returns the timestamp used during logging.
     *
     * @param message message to be logged.
     * @param logFile file name where the message should be logged.
     * @param logTime whether time should be logged too or not.
     */
    QString LogMessage(const QString &message, const QString &logFile, bool logTime) const;

    /**
     * @brief Queries the path where the logfile is stored.
     *
     * @param logFile file name.
     * @return path to file.
     */
    QString GetLogFilePath(const QString &logFile) const;

    /**
     * @brief Gets camera temperature.
     *
     * @return mapper containing the camera temperature with keys identifying the location on the camera where the
     * temperature was queried from.
     */
    QMap<QString, float> GetCameraTemperature() const;

    /**
     * @brief Displays camera temperature on an LCD display.
     */
    void DisplayCameraTemperature() const;

    /**
     * @brief Creates schedule for the thread in charge of logging temperature of the camera.
     */
    void ScheduleTemperatureThread();

    /**
     * @brief Starts the thread in charge of logging camera temperature.
     */
    void StartTemperatureThread();

    /**
     * @brief Stops thread in charge of logging camera temperature.
     */
    void StopTemperatureThread();

    /**
     * @brief Handle for timer used to schedule camera temperature logging.
     *
     * @param error type of error expected to cancel the timer.
     */
    void HandleTemperatureTimer(const boost::system::error_code &error);

    /**
     * @brief Stops thread in charge of recording snapshot images.
     */
    void StopSnapshotsThread();

    /**
     * @brief Updates the frames per second that are stored to file on the UI.
     */
    void UpdateFPSLCDDisplay() const;

    /**
     * @brief Updates the raw image displayed in the viewer tab.
     *
     * @param image Qt image to display.
     */
    void UpdateRawViewerImage(const QImage &image);

    /**
     * @brief Waits for the viewer thread to be running and for new values to be available in the queue. It emits a
     * signal indicating that a new value can be processed.
     */
    void ViewerWorkerThreadFunc();

    /**
     * @brief Takes an image, and scales it to the available width in the QtGraphicsView element before displaying it in
     * the provided scene.
     *
     * @param image Qt image to be displayed.
     * @param view the graphics view element where image will be displayed.
     * @param pixmapItem pixmap item where the image is to be placed.
     * @param scene the scene that will contain the pixmap.
     */
    static void UpdateImage(QImage image, const QGraphicsView *view, std::unique_ptr<QGraphicsPixmapItem> &pixmapItem,
                            QGraphicsScene *scene);

    /**
     * @brief Identifies if the saturation tool button is checked or not.
     *
     * @return true if the saturation button is checked, false otherwise.
     */
    bool IsSaturationButtonChecked() const;

    /**
     * @brief Provides access to the applications user interface.
     *
     * @return pointer tot he `Ui::MainWindow` from which all Qt component in the user interface can be accessed.
     */
    Ui::MainWindow *GetUI() const
    {
        return ui;
    }

    /**
     * @brief Sets the number of images recorded.
     *
     * @param count number of recorded images.
     */
    void SetRecordedCount(int count);

    /**
     * @brief Displays the number of recorded images in the GUI.
     */
    void DisplayRecordCount() const;

  protected:
    /**
     * @brief Main access point to all Qt components in the user interface. All Qt components in the UI can be accessed
     * through this pointer.
     */
    Ui::MainWindow *ui;

    /**
     * @brief This is the NDArray structure that holds the connection to the data viewed in the Viewer tab of the
     * application.
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
     * @brief Qt signal that is emitted when reading an processing of the image to display in viewer tab is finished.
     *
     * @param image Qt image containing the image to display. This should be a one channel image.
     */
    void ViewerImageProcessingComplete(QImage image);

  public slots:
    /**
     * @brief Qt slot that updates the RGB image displayed in the GUI.
     *
     * @param image Qt image containing an 8bit (per channel) RGB image to be displayed.
     */
    void UpdateRGBImage(const QImage &image);

    /**
     * Qt slot that updates the raw image displayed in the GUI.
     *
     * @param image Qt image containing an 8bit single channel image to be displayed.
     */
    void UpdateRawImage(const QImage &image);

    /**
     * @brief Qt slot that updates the saturation percentage on the LCD displays.
     *
     * @param percentageBelowThreshold Percentage of under-exposed pixels.
     * @param percentageAboveThreshold Percentage of overexposed pixels.
     */
    void UpdateSaturationPercentageLCDDisplays(double percentageBelowThreshold, double percentageAboveThreshold) const;

  private slots:

    /**
     * @brief Qt slot triggered when the snapshot button is pressed. Triggers the
     * recording of snapshot images or stops it when pressed a second time.
     *
     * If the name of the snapshot file is the same as the name of the file
     * where a video is to be recorded, an error box is displayed.
     */
    void HandleSnapshotToolButtonClicked();

    /**
     * @brief Qt slot triggered when the camera exposure value is modified either from the slider or the spinbox.
     *
     * @param value exposure value.
     */
    void HandleExposureValueChanged(int value) const;

    /**
     * @brief Qt slot triggered when the image index slider in the Viewer tab of the application changes value.
     *
     * @param value The new value of the slider.
     */
    void HandleViewerImageSliderValueChanged(int value);

    /**
     * @brief Qt slot triggered when the record button is pressed. Stars the continuous
     * recording of images to files and stops it when pressed a second time. This
     * is synchronized with the exposure time label.
     *
     * @param clicked indicates if the button is clicked.
     *
     * @throws XiLensError when an error occurs while trying to initialize the file for recording the data.
     */
    void HandleRecordButtonClicked(bool clicked);

    /**
     * @brief Updates the icons of the record button based on the recording state.
     *
     * This function modifies the appearance of the record button to reflect whether recording is active or inactive.
     * The icon changes dynamically to provide visual feedback about the current recording state.
     *
     * @param isRecording A boolean indicating the recording status.
     *                    If true, the record button will display an "active recording" icon;
     *                    if false, an "inactive recording" icon will be shown.
     */
    void SetRecordButtonIcons(bool isRecording) const;

    /**
     * @brief Qt slot triggered when the button to choose a base folder is clicked. Opens
     * a dialog where a folder can be selected.
     */
    void HandleBaseFolderButtonClicked();

    /**
     * @brief Qt slot triggered when the file button in the viewer tab is clicked. Opens
     * a dialog where a file can be selected.
     */
    void HandleViewerFileButtonClicked();

    /**
     * @brief Qt slot triggered when the file name is edited. It changes the
     * appearance of the field in the UI. It does not change the value of the
     * member variable that stores the file name.
     *
     * @param newText edited text.
     */
    void HandleFileNameLineEditTextEdited(const QString &newText);

    /**
     * @brief Qt slot triggered when the auto exposure button is pressed. Handles control
     * of the exposure time to camera.
     */
    void HandleAutoexposureToolButtonClicked(bool setAutoexposure) const;

    /**
     * @brief Qt slot triggered when the band selector tool button is clicked.
     * This displays a pop-up page where a custom slider is shown.
     * Clicking outside the pop-up page will hide it.
     */
    void HandleBandSelectorToolButtonClicked() const;

    /**
     * @brief Handles the click event for the RGB normalization tool button.
     *
     * This method is responsible for displaying a popup associated with the RGB normalization
     * functionality when the user interacts with the corresponding tool button in the UI.
     * It ensures the popup is properly sized and positioned relative to the tool button.
     */
    void HandleRGBNormToolButtonClicked() const;

    /**
     * @brief Handles the arrow click event for the Snapshot Tool Button.
     *
     * This method is responsible for displaying a popup when the arrow portion of the
     * snapshot tool button is clicked. The popup is positioned and configured with specific
     * dimensions and attributes, ensuring a proper interface interaction for the user.
     */
    void HandleSnapshotToolButtonArrowClicked() const;

    /**
     * @brief Qt slot triggered when white balance button is pressed.
     *
     * Records a new white image and sets it in the network model.
     */
    void HandleWhiteBalanceButtonClicked();

    /**
     * @brief Qt slot triggered when the dark correction button is pressed.
     *
     * Records a new dark image and sets it in the network model.
     */
    void HandleDarkCorrectionButtonClicked();

    /**
     * @brief Qt slot triggered when the trigger text is edited. It only changes the
     * appearance of the UI element.
     *
     * @param newText edited text.
     */
    void HandleLogTextLineEditTextEdited(const QString &newText) const;

    /**
     * @brief Qt slot triggered when the return key is pressed on the trigger text field.
     * It logs the message to the log file and displays it on the UI.
     */
    void HandleLogTextLineEditReturnPressed();

    /**
     * @brief Qt slot triggered when the spin box containing the number of images to skip
     * while recording is modified. It restyles the appearance of the field.
     */
    void HandleSkipFramesSpinBoxValueChanged() const;

    /**
     * @brief Qt slot triggered when a new camera is selected from the drop-down menu.
     *
     * @param index index corresponding to the element selected from the combo box.
     */
    void HandleCameraListComboBoxCurrentIndexChanged(int index);

    /**
     * @brief Checks for connected XIMEA cameras and populates the dropdown list of available cameras.
     */
    void HandleReloadCamerasToolButtonClicked();

    /**
     * @brief Qt slot triggered when file name for snapshots is edited on the UI.
     *
     * @param newText edited text.
     * @return 0 if file name is valid, 1 otherwise.
     */
    int HandleFileNameSnapshotsLineEditTextEdited(const QString &newText);

    /**
     * @brief Qt slot triggered when base folder field is edited in the UI.
     *
     * @param newText edited text.
     */
    void HandleBaseFolderLineEditTextEdited(const QString &newText);

    /**
     * @brief Qt slot triggered when the file path in the viewer tab is edited through the UI.
     *
     * @param newText edited text
     */
    void HandleViewerFileLineEditTextEdited(const QString &newText) const;

    /**
     * @brief Qt slot triggered when the return key is pressed in the file path field of the viewer tab.
     */
    void HandleViewerFileLineEditReturnPressed();

    /**
     * @brief Qt slot triggered when the saturation tool button arrow is clicked.
     *
     * This method is responsible for managing the behavior and display of the saturation tool button's popup menu
     * when its arrow is clicked. It ensures the popup is displayed with the correct dimensions and alignment.
     * The popup is shown dynamically relative to the tool button.
     */
    void HandleSaturationToolButtonArrowClicked() const;

    /**
     * @brief Qt slot triggered when the minimum saturation value is changed.
     *
     * This method is triggered when the minimum saturation value is adjusted by the user. It updates
     * the lookup table (LUT) used by the display to reflect the new minimum saturation value, ensuring
     * the displayed image is updated appropriately.
     *
     * @param value The new minimum saturation value set by the user.
     */
    void HandleSaturationMinValueChanged(int value) const;

    /**
     * @brief Qt slot triggered when the maximum saturation value is changed.
     *
     * This method is responsible for updating the LUT (Lookup Table) whenever the maximum saturation value is modified.
     * It ensures that the display reflects the correct range of saturation values by updating based on the provided
     * maximum value.
     *
     * @param value The new maximum value for saturation.
     */
    void HandleSaturationMaxValueChanged(int value) const;

    /**
     * @brief Updates the LUT (Lookup Table) in the display based on the provided dark color and other UI parameters.
     *
     * This method reacts to changes in the dark color selection by the user. It updates the corresponding LUT
     * in the display using the minimum and maximum values from the saturation spin boxes along with the provided color
     * and the right color from the spin boxes popup.
     *
     * @param color The new dark color selected by the user.
     */
    void HandleSaturationDarkColorChanged(const QColor &color) const;

    /**
     * @brief Updates the LUT (Lookup Table) in the display based on the provided saturated color and other UI
     * parameters.
     *
     * This method reacts to changes in the saturated color selection by the user. It updates the corresponding LUT
     * in the display using the minimum and maximum values from the saturation spin boxes along with the provided color
     * and the right color from the spin boxes popup.
     *
     * @param color The new saturated color selected by the user.
     */
    void HandleSaturationSaturatedColorChanged(const QColor &color) const;

    /**
     * @brief Handles the event when the arrow button of the RGB Channel Tool Button is clicked.
     *
     * This method manages the display of a popup menu or container in response to an interaction
     * with the RGB Channel Tool Button. It ensures proper positioning and dimensions of the popup
     * and aligns it with the related UI element for seamless user interaction.
     */
    void HandleRgbChannelToolButtonClicked() const;

    /**
     * @brief Handles the "About" action triggered by the user to display application information.
     */
    void HandleAboutActionTriggered();

    /**
     * @brief Handles the action triggered for opening the documentation URL.
     */
    static void HandleDocumentationActionTriggered();

    /**
     * @brief Handles the action triggered for "How to Cite" to display information on how to cite XiLens.
     */
    void HandleHowToCiteActionTriggered();

  private:
    /**
     * @brief Sets up all UI Qt connections to handle all user interactions with the UI.
     */
    void SetUpConnections();

    /**
     * @brief Handles the result emanating from a Qt connection attempt.
     *
     * @param status status returned by `QObject::connect`.
     * @param file file that calls this method.
     * @param line line where this method is called from.
     * @param func function name that calls this method.
     */
    static void HandleConnectionResult(bool status, const char *file, int line, const char *func);

    /**
     * @brief Records the white reference to a folder called "white".
     *
     * @param referenceType type of reference `white` or `dark`.
     */
    void RecordReferenceImages(const QString &referenceType);

    /**
     * @brief Stops the thread responsible for recording the reference images (white and dark).
     */
    void StopReferenceRecordingThread();

    /**
     * @brief Updates the stile of a Qt LineEdit component.
     *
     * @param lineEdit element to update.
     * @param newString new value received from element.
     * @param originalString original value of hte element before changes occurred.
     */
    static void UpdateComponentEditedStyle(QLineEdit *lineEdit, const QString &newString,
                                           const QString &originalString);

    /**
     * @brief Restores the appearance of a Qt LineEdit component.
     *
     * @param lineEdit line edit for which style should be restored.
     */
    static void RestoreLineEditStyle(QLineEdit *lineEdit);

    /**
     * @brief Displays a new image.
     */
    void Display();

    /**
     * @brief Starts the recording process.
     */
    void StartRecording();

    /**
     * @brief Stops the recording process.
     */
    void StopRecording();

    /**
     * @brief Starts the thread in charge of polling the images from the camera.
     */
    void StartPollingThread();

    /**
     * @brief Stops the thread in charge of polling the images from the camera.
     */
    void StopPollingThread();

    /**
     * @brief Creates a folder if it does not exist.
     *
     * @param folder the path to folder that needs to be created.
     */
    static void CreateFolderIfNecessary(const QString &folder);

    /**
     * @brief Records image to specified sub folder and using specified file name.
     *
     * @param ignoreSkipping ignores the number of frames to skip and stores the
     * image anyways.
     */
    void RecordImage(bool ignoreSkipping);

    /**
     * @brief Starts IO service in a thread in charge of saving the images to files.
     */
    void ThreadedRecordImage();

    /**
     * @brief Initializes the file object inside the image container. This object is used
     * to store all images while recording to a single file.
     *
     * @param subFolder folder where data will be stored.
     * @param fileName file name.
     *
     * @throws XiLensError when the initialization of the file cannot be completed.
     */
    void InitializeImageFileRecorder(std::string subFolder = "", std::string fileName = "");

    /**
     * @brief Indicates if an image should be recorded to file or not depending on the
     * frame number and the number of frames to skip.
     *
     * @param nSkipFrames number of frames to skip.
     * @param ImageID frame number.
     * @return true if image should be recorded to file or false if not.
     */
    static bool ImageShouldBeRecorded(int nSkipFrames, long ImageID);

    /**
     * @brief Updates image counter.
     */
    void CountImages();

    /**
     * @brief Updates timer displayed on the UI when recordings are started.
     */
    void UpdateTimer();

    /**
     * @brief Stops the timer displayed in the UI when recordings are started.
     */
    void StopTimer() const;

    /**
     * @brief Method used to record a specific number of images.
     *
     * The main different to recording a video, this function records a specific number of images instead of a
     * continuous stream.
     * This method terminates after the specified number of images has been recorded.
     */
    void RecordSnapshots();

    /**
     * @brief Captures an image from the camera and stores it in the specified file, while updating the progress bar.
     *
     * This method performs the following operations:
     * - Retrieves the camera's exposure time and calculates the wait time accordingly.
     * - Captures the current image from the `m_imageContainer` and stores it in the specified `snapshotsFile`.
     * - Updates the progress bar to reflect the percentage of images captured out of the total images.
     *
     * @param snapshotsFile A reference to the file where the captured image data will be stored.
     * @param currentIndex The index of the current image being captured (used for progress calculation).
     * @param totalImages The total number of images to be captured (used for progress calculation).
     */
    void CaptureAndStoreSnapshotImage(FileImage &snapshotsFile, int currentIndex, int totalImages);

    /**
     * @brief Opens a file for saving snapshot images and initializes the file with the current image's dimensions.
     *
     * This method attempts to create a new file to save snapshot images using the specified file path. It retrieves the
     * height and width of the current image and initializes the file accordingly. If the operation fails, an error is
     * logged, an error dialog is shown to the user, and a null pointer is returned.
     *
     * @param filePath The path of the file to be created for saving snapshot images.
     * @return A unique pointer to the initialized `FileImage` object if successful, or a null pointer if the operation
     * fails.
     */
    std::unique_ptr<FileImage> OpenFileForSnapshots(const QString &filePath);

    /**
     * @brief Display an error window with a title and message.
     *
     * @param text title of the error.
     * @param informativeText additional error message to be displayed in the window.
     */
    static void ShowErrorDialog(const QString &text, const QString &informativeText);

    /**
     * @brief Enables or disables UI components related to snapshot functionality.
     *
     * This method adjusts the enabling state of UI components associated with the snapshot feature.
     * It modifies the interaction capabilities of these components based on the provided parameter.
     *
     * @param enabled A boolean value indicating whether the snapshot-related UI components
     * should be enabled (true) or disabled (false).
     */
    void ToggleSnapshotUI(bool enabled) const;

    /**
     * @brief Resets the state of the snapshot-related UI components.
     *
     * This method resets the snapshot UI to its initial state by setting the progress bar value to 0
     * and re-enabling the snapshot UI components. It ensures the snapshot interface is properly
     * prepared for a new operation or interaction.
     */
    void ResetSnapshotUI() const;

    /**
     * @brief UpdateExposure Synchronizes the sliders and text edits displaying
     * the current exposure setting.
     */
    void UpdateExposure() const;

    /**
     * @brief Enables and disables elements of the GUI that should not me modified while
     * recordings are in progress.
     *
     * @param recordingInProgress indicates if recordings are happening or not.
     */
    void HandleElementsWhileRecording(bool recordingInProgress) const;

    /**
     * @brief MainWindow::GetWritingFolder returns the folder there the image
     * files are written to.
     *
     * @return folder where data is to be stored.
     */
    QString GetWritingFolder() const;

    /**
     * @brief GetFullFilenameStandardFormat returns the full filename of the
     * current file which shall be written.
     *
     * It automatically add the current write path and puts the name in a standard
     * format including timestamp etc.
     *
     * @param fileName the name of the file (snapshot, recording, liver_image, ...).
     * @param extension file extension (.b2nd).
     * @param subFolder sometimes we want to add an additional layer of subfolder.
     * specifically when saving white/dark balance images.
     * @return
     */
    QString GetFullFilenameStandardFormat(std::string &&fileName, const std::string &extension,
                                          std::string &&subFolder) const;

    /**
     * @brief Queries the base folder path where data is to be stored.
     */
    QString GetBaseFolder() const;

    /**
     * @brief Starts image acquisition by initializing image contained and displayer.
     */
    void StartImageAcquisition(QString cameraIdentifier);

    /**
     * @brief Stops image acquisition by disconnecting image displayer and stopping image
     * polling to the image container.
     */
    void StopImageAcquisition();

    /**
     * @brief Formats timestamp tag from format  yyyyMMdd_HH-mm-ss-zzz into a human
     * readable format.
     *
     * @param timestamp time stamp to be formatted.
     * @return formatted timestamp.
     */
    static QString FormatTimeStamp(const QString &timestamp);

    /**
     * @brief Opens the N-dimensional array and adjusts UI components based on the contents of the file.
     *
     *In summary, this method performs the following operations:
     *
     * - Adjusts range of viewer image slider.
     * - Triggers the display of the first image in the file.
     *
     * @param filePath path to the file to open.
     */
    void OpenFileInViewer(const QString &filePath);

    /**
     * @brief Reads a single image slice from file and creates an OpenCv matrix containing the data of the image.
     *
     * It emits a signal indicating that the processing finished and provides the processed image through the signal.
     *
     * @param value image index to load from file
     */
    void ProcessViewerImageSliderValueChanged(int value);

    /**
     * @brief Sets the scene for RGB and raw image viewers. It defines antialiasing and smooth pixmap transformations.
     */
    void SetGraphicsViewScene() const;

    /**
     * @brief Appends the current time to que of recorded time stamps that can be used to calculate the frames per
     * second.
     */
    void RegisterTimeImageRecorded();

    /**
     * @brief Displays a popup widget when interacting with a specific tool button.
     *
     * This function positions and displays a popup widget anchored to a given tool button.
     * The position and alignment of the popup are determined by the provided parameters.
     *
     * The placement process includes:
     * - Calculating the global position of the provided tool button.
     * - Determining the pop-up widget's position based on the button's position and alignment.
     * - Resizing the popup widget to the specified dimensions.
     * - Displaying the popup at the calculated position.
     *
     * @param button The tool button widget that triggers the popup.
     * @param popup The popup widget to be displayed.
     * @param popupWidth The desired width of the popup widget.
     * @param popupHeight The desired height of the popup widget.
     * @param centerAlign A boolean indicating whether the popup should be horizontally centered relative to the button.
     */
    static void ShowPopupOnToolButtonInteraction(const QWidget *button, QWidget *popup, int popupWidth, int popupHeight,
                                                 bool centerAlign);

    /**
     * @brief Adjusts UI components and settings based on the specific camera type and model.
     *
     * This method is responsible for enabling or disabling spectral-specific UI components depending on
     * whether the provided camera type supports spectral imaging.
     *
     * If the camera model is invalid or not found in the mapping system, the function logs an error and
     * throws a runtime exception.
     *
     * @param cameraType A string representing the type of the camera (e.g., spectral or other types).
     * @param cameraModel A string representing the specific model of the camera to be handled.
     *
     * @throws std::runtime_error If the camera model is not found in the camera mapping system.
     */
    void HandleCameraSpecificUiComponents(const QString &cameraType, const QString &cameraModel) const;

    /**
     * @brief The file name where videos are to be stored.
     */
    QString m_fileName;

    /**
     * @brief Trigger text entered to the log function of the UI.
     */
    QString m_triggerText;

    /**
     * @brief Folder path where all data is to be stored.
     */
    QString m_baseFolderPath;

    /**
     * @brief Path to a .b2nd file to be displayed in the viewer tab.
     */
    QString m_viewerFilePath;

    /**
     * @brief Value of exposure time for the camera.
     */
    QString m_labelExp;

    /**
     * @brief File name used for snapshot images.
     */
    QString m_snapshotsFileName;

    /**
     * @brief Elapsed timer used for the timer displayed in the UI.
     */
    QElapsedTimer m_elapsedTimer;

    /**
     * @brief Time elapsed since recordings started.
     */
    double m_elapsedTime;

    /**
     * @brief Time elapsed since recordings started as text field.
     */
    QString m_elapsedTimeText;

    /**
     * @brief Text stream used to generate the elapsed time text.
     */
    QTextStream m_elapsedTimeTextStream;

    /**
     * @brief Minimum blood volume fraction value used for display.
     */
    QString m_minVhb;

    /**
     * @brief Maximum blood volume fraction value used for display.
     */
    QString m_maxVhb;

    /**
     * @brief Minimum oxygenation value used for display.
     */
    QString m_minSao2;

    /**
     * @brief Maximum oxygenation value used for display.
     */
    QString m_maxSao2;

    /**
     * @brief Image container where each new image from the camera is stored.
     */
    ImageContainer m_imageContainer;

    /**
     * @brief Camera interface. Handles communication with each connected camera.
     */
    CameraInterface m_cameraInterface;

    /**
     * @brief Wrapper to xiAPI, useful for mocking during testing.
     */
    std::shared_ptr<XiAPIWrapper> m_xiAPIWrapper = std::make_shared<XiAPIWrapper>();

    /**
     * @brief Display in charge of displaying each image.
     */
    Displayer *m_display;

    /**
     * @brief Handles if test mode should be set for the program. All images are stored
     * to same file.
     */
    bool m_testMode;

    /**
     * @brief Thread in charge of running the Image container.
     */
    boost::thread m_imageContainerThread;

    /**
     * @brief Thread in charge of handling the image viewer processing before displaying it in the UI.
     */
    boost::thread m_viewerThread;

    /**
     * @brief Queue used to store image indices that are then processed to load the corresponding images.
     */
    std::queue<int> m_viewerSliderQueue;

    /**
     * @brief Mutex used as a locking mechanism to avoid raises when processing images for the Viewer tab.
     */
    boost::mutex m_mutexImageViewer;

    /**
     * @brief Primitive used to lock viewer thread execution until the que is not empty and when the thread has to stop.
     */
    boost::condition_variable m_viewerQueueCondition;

    /**
     * @brief Indicates if the thread in charge of processing and displaying images in the viewer tab is running.
     */
    bool m_viewerThreadRunning;

    /**
     * @brief IO service in charge of recording images to files.
     */
    boost::asio::io_service m_IOService;

    /**
     * @brief Work object to control safe finish of IOService.
     */
    std::unique_ptr<boost::asio::io_service::work> m_IOWork;

    /**
     * @brief ID service for recording temperature to file.
     */
    boost::asio::io_service m_temperatureIOService;

    /**
     * @brief Async IO work. Keeps the IO service alive in the thread in charge of
     * temperature recording.
     */
    std::unique_ptr<boost::asio::io_service::work> m_temperatureIOWork;

    /**
     * @brief Thread pool used for recording the data.
     */
    boost::thread_group m_threadGroup;

    /**
     * @brief Mutual exclusion mechanism in charge of synchronization.
     */
    boost::mutex m_mutexImageRecording;

    /**
     * @brief Camera temperature recording thread.
     */
    boost::thread m_temperatureThread;

    /**
     * @brief Snapshot image recording thread.
     */
    boost::thread m_snapshotsThread;

    /**
     * @brief Thread where white and dark references are recorded.
     */
    boost::thread m_referenceRecordingThread;

    /**
     * @brief Thread containing the timer for temperature recording at certain intervals.
     */
    std::shared_ptr<boost::asio::steady_timer> m_temperatureThreadTimer;

    /**
     * @brief Counts how many images have been recorded.
     */
    std::atomic<unsigned long> m_recordedCount;

    /**
     * @brief Counts how many images would have been recorded.
     */
    std::atomic<unsigned long> m_imageCounter;

    /**
     * @brief Counts how many images were skipped during the recording process.
     */
    std::atomic<unsigned long> m_skippedCounter;

    /**
     * @brief Container to store the time stamps when a new image is recorded. This is used to calculate the FPS.
     */
    std::deque<std::chrono::steady_clock::time_point> m_recordedTimestamps;

    /**
     * @brief Smart pointer to the RGB scene where the RGB images will be displayed.
     */
    std::unique_ptr<QGraphicsScene> m_rgbScene = std::make_unique<QGraphicsScene>(this);

    /**
     * @brief Smart pointer to raw scene where the raw unprocessed images will be displayed.
     */
    std::unique_ptr<QGraphicsScene> m_rawScene = std::make_unique<QGraphicsScene>(this);

    /**
     * @brief Smart pointer to a scene where the images for the Viewer tab are displayed.
     */
    std::unique_ptr<QGraphicsScene> m_rawViewerScene = std::make_unique<QGraphicsScene>(this);

    /**
     * @brief Smart pointer to pixmap used to display the RGB images.
     */
    std::unique_ptr<QGraphicsPixmapItem> m_rgbPixMapItem;

    /**
     * @brief Smart pointer to pixmap where raw unprocessed images will be displayed.
     */
    std::unique_ptr<QGraphicsPixmapItem> m_rawPixMapItem;

    /**
     * @brief Smart Pointer to a pixmap used to display the images in the RawViewer.
     */
    std::unique_ptr<QGraphicsPixmapItem> m_rawViewerPixMapItem;

    /**
     * @brief Timer that sets the rate of updates for the FPS LCD Display in the UI.
     */
    QTimer *m_updateFPSDisplayTimer;

    /**
     * @brief Custom pop-up slider page used to display the band selector slider.
     */
    QSliderPopup *m_bandSelectorSliderPopup;

    /**
     * @brief custom pop-up slider page used to display the image brightness slider.
     */
    QSliderPopup *m_rgbNormSliderPopup;

    /**
     * @brief custom pop-up window to display snapshot configuration such as file name and number of images
     */
    QLineSpinPopup *m_snapshotPopup;

    /**
     * @brief Popup widget containing two spin boxes to adjust saturation values and the corresponding colors.
     */
    QDoubleSpinBoxesWithColorPickersPopup *m_saturationSpinBoxesPopup;

    /**
     * @brief Popup widget containing spinners that control the channels used to reconstruct an RGB image for a
     * spectral camera.
     */
    QRgbChannelSpinBoxesPopup *m_rgbChannelSpinBoxesPopup;
};

#endif // MAINWINDOW_H
