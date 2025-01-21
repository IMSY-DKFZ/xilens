/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#ifndef XILENS_WIDGETS_H
#define XILENS_WIDGETS_H

#include <QColor>
#include <QEvent>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

/**
 * @brief Custom slider widget used to display text labels along the slider.
 *
 * Custom slider widget that displays text corresponding to values in the slider.
 * By default the number of text labels in the slider is set to a maximum of `8`.
 * If the calculated number of labels based on the interval and the min and max values in the slider exceed this value,
 * the number of labels is caped.
 *
 * The text is centered on the corresponding value of the slider while taking into account the groove margins.
 * The margin takes a custom value of `12`, this value can be modified depending on what is specified in the
 * style sheet.
 */
class QSliderLabeled : public QSlider
{
  public:
    /**
     * @brief Constructor of the labeled QSlider.
     *
     * @param parent parent class.
     */
    explicit QSliderLabeled(QWidget *parent = nullptr);

    /**
     * @brief Sets the margin of the groove of the slider.
     *
     * @param value
     */
    void SetGrooveMargin(const int value)
    {
        m_grooveMargin = value;
    }

    /**
     * @brief Sets Maximum number of labels to display in the slider.
     *
     * @param value maximum number of labels.
     */
    void SetMaxNumberOfLabels(const int value)
    {
        m_maxNumberOfLabels = value;
    }

    /**
     * @brief Applies a custom style sheet that defines the width and height of the slider based on the orientation
     * of the slider.
     */
    void ApplyStyleSheet();

    /**
     * @brief Sets the maximum spread of the slider. This will represent the maximum height when slider is horizontal
     * and the maximum width when it is vertical.
     *
     * @param value the slider spread.
     */
    void SetSliderSpread(const int value)
    {
        m_sliderSpread = value;
    }

    /**
     * @brief Toggles the display of text labels on the slider.
     *
     * This method enables or disables the display of text labels along the slider
     * based on the provided value. When enabled, labels corresponding to values are shown.
     *
     * @param value `true` to enable the display of labels, `false` to disable them.
     */
    void SetDisplayLabels(const bool value)
    {
        m_displayLabels = value;
    }

    /**
     * @brief Sets the interval for displaying labels on the slider.
     *
     * This method configures the interval between the labels displayed along the slider.
     * The labels are generated based on the set interval and the current minimum and
     * maximum values of the slider.
     *
     * @param value The interval value specifying the spacing between labels. Must be a positive integer.
     */
    void SetLabelInterval(const int value)
    {
        m_labelInterval = value;
    }

  protected:
    /**
     * @brief paint event used to draw the labels on the slider. This overrides the paint event, but it calls the
     * original method before drawing the text.
     *
     * @param event paint event parameters.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief show event that overwrites the original QSlider show event to apply a custom style sheet before  showing
     * the slider.
     *
     * @param event show event parameters.
     */
    void showEvent(QShowEvent *event) override
    {
        QSlider::showEvent(event);
        ApplyStyleSheet();
    }

    /**
     * @brief Overrides the event() method from the parent class.
     *
     * This method is triggered when an event is received by the widget. It specifically handles the
     * `QEvent::EnabledChange` event and calls the `UpdatePainterPen()` method to update the painter pen. It then calls
     * the event() method of the parent class to handle any other events. Finally, it returns a boolean value indicating
     * whether the event was handled.
     *
     * @param e The event object.
     * @return True if the event was handled, false otherwise.
     */
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::EnabledChange)
        {
            UpdatePainterPen();
        }
        return QSlider::event(e);
    }

    /**
     * @brief Triggered when the mouse is moved over the labeled QSlider.
     *
     * It shows a tooltip with the corresponding value of the slider at the current mouse position using
     * the `QToolTip` class. It then calls the `mouseMoveEvent()` method of the parent class to handle any other events.
     *
     * @param event A pointer to the `QMouseEvent` that contains information about the mouse move event.
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * Slider groove margin.
     */
    int m_grooveMargin = 12;

    /**
     * @brief Interval between labels displayed on a slider.
     *
     * Determines the step interval for displaying text labels along the slider.
     * This value is used to control the spacing between consecutive labels,
     * allowing customization of the labeling frequency on the slider.
     */
    int m_labelInterval;

    /**
     * @brief Maximum number of labels to display.
     */
    int m_maxNumberOfLabels = 8;

    /**
     * @brief Size of the slider in pixels. Represents the maximum height when slider is horizontal and the maximum
     * width when it is vertical.
     */
    int m_sliderSpread = 48;

  private:
    /**
     * @brief The color of the pen to use when drawing the text.
     */
    QColor m_penColor;

    /**
     * @brief Updates the painter's pen color based on the enabled state of the QSliderLabeled widget.
     *
     * The pen color is set to a specific color if the widget is enabled, and to a different color if it is disabled.
     */
    void UpdatePainterPen();

    /**
     * @brief Computes and returns the label interval for the slider.
     *
     * Determines the interval at which text labels should be displayed along the slider.
     * If no custom label interval (`m_labelInterval`) is specified, the method calculates the
     * appropriate interval based on the slider's tick interval and ensures that the number
     * of labels does not exceed the maximum allowed (`m_maxNumberOfLabels`).
     *
     * The computed interval ensures proper alignment of the labels with the slider's range,
     * taking into account the minimum and maximum values.
     *
     * @return The interval value between consecutive labels on the slider.
     */
    int GetLabelInterval() const;

    /**
     * @brief Indicates whether text labels should be displayed on the slider.
     *
     * This variable controls the visibility of text labels along the slider. When set to `true`,
     * labels corresponding to slider values are rendered. If set to `false`, the labels are not shown.
     */
    bool m_displayLabels;
};

/**
 * @brief Popup widget containing a labeled slider.
 *
 * This class implements a popup widget that encapsulates a custom labeled slider widget.
 * It provides functionality to access the slider's value as well as modify its minimum and maximum bounds.
 * The slider is displayed within a vertical layout to ensure proper placement within the widget.
 */
class QSliderPopup : public QWidget
{
  public:
    /**
     * @brief Constructs a frameless popup widget containing a labeled slider with a specified range and orientation.
     *
     * This constructor initializes a `QSliderPopup` containing a `QSliderLabeled` widget integrated within a vertical
     * layout. The slider is configured with the provided minimum value, maximum value, and default value. The
     * orientation of the slider (horizontal or vertical) is set based on the given parameter. The widget is styled as a
     * frameless popup window.
     *
     * @param min The minimum value of the slider.
     * @param max The maximum value of the slider.
     * @param value The initial value of the slider.
     * @param orientation The orientation of the slider (`Qt::Horizontal` or `Qt::Vertical`).
     * @param parent The parent widget of the popup, can be null.
     * @return An instance of the QSliderPopup class.
     */
    QSliderPopup(int min, int max, int value, Qt::Orientation orientation, QWidget *parent = nullptr);

    /**
     * @brief returns the value of the slider.
     *
     * @return value of hte slider.
     */
    int value() const
    {
        return m_slider->value();
    }

    /**
     * @brief Sets the minimum value of the slider.
     *
     * @param min minimum value.
     */
    void SetMinimum(const int min) const
    {
        m_slider->setMinimum(min);
    }

    /**
     * @brief Sets the maximum value of the slider.
     *
     * @param max maximum value.
     */
    void SetMaximum(const int max) const
    {
        m_slider->setMaximum(max);
    }

    /**
     * @brief Sets whether text labels are displayed along the slider.
     *
     * This method enables or disables the display of labels on the slider
     * based on the input value. When set to true, labels corresponding
     * to values in the slider are shown, otherwise, labels are hidden.
     *
     * @param value Boolean indicating whether to display labels (`true`) or not (`false`).
     */
    void SetDisplayLabels(const bool value) const
    {
        m_slider->SetDisplayLabels(value);
    }

    /**
     * @brief Sets the interval for displaying labels on the slider.
     *
     * Defines the interval between adjacent labels displayed along the slider.
     * This allows customization of how frequently labels appear on the slider
     * based on the specified interval value.
     *
     * @param value The numerical value representing the interval between labels
     */
    void SetLabelInterval(const int value) const
    {
        m_slider->SetLabelInterval(value);
    }

  private:
    /**
     * @brief Custom labeled slider component to be displayed inside the pop-up page.
     */
    QSliderLabeled *m_slider;

    /**
     * @brief Layout to be used for the pop-up page.
     */
    QVBoxLayout *m_layout;

    /**
     * @brief Layout used for the background frame where all components are placed.
     */
    QHBoxLayout *m_backgroundFrameLayout;

    /**
     * @brief Frame where all UI components will be placed.
     */
    QFrame *m_frame;

    /**
     * @brief Content margin used for spacing UI components inside a frame.
     */
    static constexpr int m_contentMargin = 10;

    /**
     * @brief Default border radius value for the popup window frame styling.
     */
    static constexpr int m_windowBorderRadius = 5;
};

/**
 * @brief Popup widget containing a line input and a spin box.
 *
 * This class implements a popup widget that encapsulates a QLineEdit and a QSpinBox widget.
 * It provides functionality to access and modify their values.
 * The widgets are displayed next to each other in a horizontal layout with proper spacing.
 */
class QLineSpinPopup : public QWidget
{
  public:
    /**
     * @brief Constructs a popup widget containing a QLineEdit and a QSpinBox.
     *
     * The line edit and spin box are embedded within a horizontal-oriented layout to ensure proper placement.
     * The widget is styled as a frameless popup window.
     *
     * @param parent The parent widget of the popup, can be null.
     */
    explicit QLineSpinPopup(QWidget *parent = nullptr);

    /**
     * @brief Gets the text value from the QLineEdit.
     *
     * @return The text entered into the QLineEdit.
     */
    QString text() const
    {
        return m_lineEdit->text();
    }

    /**
     * @brief Sets the text value in the QLineEdit.
     *
     * @param fileName The file name to set in the QLineEdit.
     */
    void setText(const QString &fileName) const
    {
        m_lineEdit->setText(fileName);
    }

    /**
     * @brief Gets the value from the QSpinBox.
     *
     * @return The current value of the QSpinBox.
     */
    int value() const
    {
        return m_spinBox->value();
    }

    /**
     * @brief Sets the value in the QSpinBox.
     *
     * @param value The value to set in the QSpinBox.
     */
    void setValue(const int value) const
    {
        m_spinBox->setValue(value);
    }

    /**
     * @brief Line edit for entering user specified text.
     */
    QLineEdit *m_lineEdit;

    /**
     * @brief Spin box for setting user-defined integer values.
     */
    QSpinBox *m_spinBox;

  private:
    /**
     * @brief Layout for arranging the widgets.
     */
    QVBoxLayout *m_layout;

    /**
     * @brief Layout used for the background frame where all components are placed.
     */
    QHBoxLayout *m_backgroundFrameLayout;

    /**
     * @brief Frame where all UI components will be placed.
     */
    QFrame *m_frame;

    /**
     * @brief Content margin used for spacing UI components inside a frame.
     */
    static constexpr int m_contentMargin = 10;

    /**
     * @brief Default border radius value for the popup window frame styling.
     */
    static constexpr int m_windowBorderRadius = 5;
};

/**
 * @brief Custom widget providing a pop-up interface for managing two spin box elements.
 *
 * This widget allows the creation of a pop-up window containing multiple spin box widgets.
 * It is designed for use cases where multiple integer input fields need to be displayed and adjusted.
 */
class QDoubleSpinBoxesPopup : public QWidget
{
    Q_OBJECT
  public:
    /**
     * @brief Custom popup widget containing two spin box widgets.
     *
     * It is particularly useful for scenarios where a grouped input of integer values is required, enabling
     * users to input several related integer values within a single popup.
     * The layout and number of spin boxes can be customized.
     */
    explicit QDoubleSpinBoxesPopup(QWidget *parent = nullptr);

    /**
     * @brief Retrieves the minimum value set in the first spin box widget.
     *
     * @return The integer value currently set as the minimum in the first spin box.
     */
    int minValue() const
    {
        return m_spinBox1->value();
    }

    /**
     * @brief Retrieves the maximum value set in the second spin box widget.
     *
     * @return The integer value currently set as the maximum in the second spin box.
     */
    int maxValue() const
    {
        return m_spinBox2->value();
    }

    /**
     * @brief Sets the minimum value for the first spin box widget.
     *
     * @param value The integer value to be set as the minimum in the first spin box.
     */
    void setMinValue(const int value) const
    {
        m_spinBox1->setValue(value);
    }

    /**
     * @brief Sets the maximum value for the second spin box widget.
     *
     * @param value The integer value to be set as the maximum in the second spin box.
     */
    void setMaxValue(const int value) const
    {
        m_spinBox2->setValue(value);
    }

    /**
     * @brief Pointer to the first spin box widget for setting and retrieving minimum integer values.
     */
    QSpinBox *m_spinBox1;

    /**
     * @brief Pointer to the second spin box widget for setting and retrieving maximum integer values.
     */
    QSpinBox *m_spinBox2;

  signals:
    /**
     * @brief Signal emitted when the minimum value in the first spin box widget is changed.
     *
     * @param value The new minimum value set in the spin box.
     */
    void minValueChanged(int value);

    /**
     * @brief Signal emitted when the maximum value of the second spin box widget is changes.
     *
     * @param value The new maximum value of the slider after the change.
     */
    void maxValueChanged(int value);

  private:
    /**
     * @brief Layout for arranging the widgets.
     */
    QHBoxLayout *m_layout;

    /**
     * @brief Layout used for the background frame where all components are placed.
     */
    QHBoxLayout *m_backgroundFrameLayout;

    /**
     * @brief Frame where all UI components will be placed.
     */
    QFrame *m_frame;

    /**
     * @brief Content margin used for spacing UI components inside a frame.
     */
    static constexpr int m_contentMargin = 10;

    /**
     * @brief Default border radius value for the popup window frame styling.
     */
    static constexpr int m_windowBorderRadius = 5;
};

/**
 * @brief A custom tool button with an arrow indicator in the bottom right corner.
 *
 * A specialized tool button designed for use in interfaces where an arrow
 * indication is required. This button can handle arrow orientation and
 * visual styles to assist user interactions in directional or expandable UI
 * elements.
 */
class QArrowToolButton : public QToolButton
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor for QArrowToolButton.
     *
     * Initializes a QArrowToolButton instance with a custom arrow functionality,
     * inheriting from QToolButton. This constructor sets the parent widget
     * of the button for proper widget hierarchy management.
     *
     * @param parent The parent widget for this button. If null, the button
     *               has no parent and acts as a top-level widget.
     */
    explicit QArrowToolButton(QWidget *parent = nullptr);

  signals:
    /**
     * @brief Signal emitted when the arrow on the button is clicked.
     *
     * This signal is emitted whenever the user clicks on the arrow portion of the button.
     * It is primarily used to notify connected slots of this specific interaction, allowing
     * for custom handling or behavior in response to the arrow click.
     */
    void ArrowClicked();

  protected:
    /**
     * @brief Custom paint event for drawing the arrow on the button.
     *
     * This method overrides the default paint event to draw a custom arrow on the button.
     * The arrow's color changes dynamically based on the button's state, such as whether
     * it is enabled, hovered, or pressed. The arrow is drawn using a QPainter instance
     * and takes into account the visual states obtained through QStyleOptionToolButton.
     *
     * @param event A pointer to the QPaintEvent object providing details about the paint event.
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief Handles mouse press events.
     *
     * This method processes mouse press events and checks if the click occurs
     * within the rectangle occupied by the arrow. If the arrow is clicked, it emits
     * an `arrowClicked` signal and prevents the default button behavior.
     * Otherwise, the event is passed to the parent class for standard processing.
     *
     * @param event The pointer to the QMouseEvent containing information about the mouse press.
     */
    void mousePressEvent(QMouseEvent *event) override;

  private:
    /**
     * @brief Calculates the rectangle area for the arrow region on the button.
     *
     * Determines the rectangular area where the arrow is drawn within the button.
     * The size of the arrow is dynamically calculated as a fraction of the button's dimensions
     * (1/5th of the smaller dimension) and ensures it remains an odd number for even alignment.
     * The rectangle is positioned in the lower-right corner of the button.
     *
     * @return QRect representing the bounding rectangle for the arrow region.
     */
    QRect ArrowRect() const;
};

/**
 * @brief Styles a QFrame widget by applying a border, border radius, and content margins.
 *
 * This method applies a stylesheet to the provided QFrame, setting a solid border
 * with a radius specified by the borderRadius parameter. It also adjusts the layout's
 * content margins to the provided contentMargin value.
 *
 * @param frame Pointer to the QFrame to be styled.
 * @param layout Pointer to the QLayout associated with the QFrame.
 * @param borderRadius The radius to be applied to the corners of the QFrame's border.
 * @param contentMargin The margin to be applied around the contents of the QLayout.
 */
void StyleQFrameInPopupWindow(QFrame *frame, QLayout *layout, int borderRadius, int contentMargin);

#endif // XILENS_WIDGETS_H
