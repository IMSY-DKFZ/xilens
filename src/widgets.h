/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#ifndef XILENS_WIDGETS_H
#define XILENS_WIDGETS_H

#include <QColor>
#include <QEvent>
#include <QSlider>
#include <QVBoxLayout>

/**
 * @brief Custom slider widget used to display text labels along the slider
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
     * Constructor of the labeled QSlider.
     *
     * @param parent parent class
     */
    explicit QSliderLabeled(QWidget *parent = nullptr);

    /**
     * Sets the margin of the groove of the slider.
     *
     * @param value
     */
    void SetGrooveMargin(int value);

    /**
     * Sets Maximum number of labels to display in the slider.
     *
     * @param value maximum number of labels.
     */
    void SetMaxNumberOfLabels(int value);

    /**
     * Applies a custom style sheet that defines the width and height of the slider based on the orientation
     * of the slider.
     */
    void ApplyStyleSheet();

    /**
     * Sets the maximum spread of the slider. This will represent the maximum height when slider is horizontal and
     * the maximum width when it is vertical.
     *
     * @param value the slider spread.
     */
    void SetSliderSpread(int value);

  protected:
    /**
     * paint event used to draw the labels on the slider. This overrides the paint event, but it calls the original
     * method before drawing the text.
     *
     * @param event paint event parameters
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * show event that overwrites the original QSlider show event to apply a custom style sheet before  showing the
     * slider.
     *
     * @param event show event parameters
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
     * Triggered when the mouse is moved over the labeled QSlider.
     * It shows a tooltip with the corresponding value of the slider at the current mouse position using
     * the `QToolTip` class. It then calls the `mouseMoveEvent()` method of the parent class to handle any other events.
     *
     * @param event A pointer to the `QMouseEvent` that contains information about the mouse move event.
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * Slider groove margin
     */
    int m_grooveMargin = 12;

    /**
     * Maximum number of labels to display
     */
    int m_maxNumberOfLabels = 8;

    /**
     * Size of the slider in pixels. Represents the maximum height when slider is horizontal and the maximum width when
     * it is vertical.
     */
    int m_sliderSpread = 48;

  private:
    /**
     * The color of the pen to use when drawing the text.
     */
    QColor m_penColor;

    /**
     * Updates the painter's pen color based on the enabled state of the QSliderLabeled widget.
     * The pen color is set to a specific color if the widget is enabled, and to a different color if it is disabled.
     */
    void UpdatePainterPen();
};

/**
 * @brief Popup widget containing a labeled slider
 *
 * This class implements a popup widget that encapsulates a custom labeled slider widget.
 * It provides functionality to access the slider's value as well as modify its minimum and maximum bounds.
 * The slider is displayed within a vertical layout to ensure proper placement within the widget.
 */
class QSliderPopup : public QWidget
{
    /**
     * @brief Creates a popup widget with an integrated custom labeled slider and layout.
     *
     * This constructor initializes a frameless popup widget containing a custom horizontal `QSliderLabeled` component
     * with a defined range of values (from 1 to 16). The slider is embedded in a horizontal-oriented layout
     * with specified content margins. The widget is set to be displayed as a frameless window with popup behavior.
     *
     * @param parent The parent widget, it can be null.
     */
  public:
    explicit QSliderPopup(QWidget *parent = nullptr);

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
     * @brief returns the value of the slider
     *
     * @return value of hte slider
     */
    int value() const
    {
        return slider->value();
    }

    /**
     * @brief Sets the minimum value of the slider.
     *
     * @param min minimum value.
     */
    void setMinimum(const int min) const
    {
        slider->setMinimum(min);
    }

    /**
     * Sets the maximum value of the slider.
     *
     * @param max maximum value.
     */
    void setMaximum(const int max) const
    {
        slider->setMaximum(max);
    }

  private:
    /**
     * Custom labeled slider component to be displayed inside the pop-up page.
     */
    QSliderLabeled *slider;

    /**
     * Layout to be used for the pop-up page.
     */
    QVBoxLayout *layout;
};

#endif // XILENS_WIDGETS_H
