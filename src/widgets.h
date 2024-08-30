/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#ifndef XILENS_WIDGETS_H
#define XILENS_WIDGETS_H

#include <QColor>
#include <QEvent>
#include <QSlider>
#include <QStyle>

/**
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
    void setGrooveMargin(int value);

    /**
     * Sets Maximum number of labels to display in the slider.
     *
     * @param value maximum number of labels.
     */
    void setMaxNumberOfLabels(int value);

    /**
     * Applies a custom style sheet that defines the width and height of the slider based on the orientation
     * of the slider.
     */
    void applyStyleSheet();

    /**
     * Sets the maximum spread of the slider. This will represent the maximum height when slider is horizontal and
     * the maximum width when it is vertical.
     *
     * @param value the slider spread.
     */
    void setSliderSpread(int value);

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
        applyStyleSheet();
    }

    /**
     * @brief Overrides the event() method from the parent class.
     *
     * This method is triggered when an event is received by the widget. It specifically handles the
     * `QEvent::EnabledChange` event and calls the `updatePainterPen()` method to update the painter pen. It then calls
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
            updatePainterPen();
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
    void updatePainterPen();
};

#endif // XILENS_WIDGETS_H
