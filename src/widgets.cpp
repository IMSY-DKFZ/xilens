/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include <QColor>
#include <QMouseEvent>
#include <QPainter>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QToolTip>

#include "widgets.h"

QSliderLabeled::QSliderLabeled(QWidget *parent) : QSlider(parent)
{
}

void QSliderLabeled::ApplyStyleSheet()
{
    QFontMetrics fm(font());
    QString maxLabel = QString::number(maximum());
    int textWidth = fm.horizontalAdvance(maxLabel);
    int textHeight = fm.height();

    if (orientation() == Qt::Orientation::Horizontal)
    {
        setStyleSheet(QString("QSlider{"
                              " min-height: %1px;"
                              " max-height: %2px;"
                              " padding-top: %3px;"
                              "}")
                          .arg(m_sliderSpread + textHeight)
                          .arg(m_sliderSpread + textHeight)
                          .arg(-m_sliderSpread / 2));
    }
    else if (orientation() == Qt::Orientation::Vertical)
    {
        setStyleSheet(QString("QSlider{"
                              " min-width: %1px;"
                              " max-width: %2px;"
                              " padding-right: %3px;"
                              "}")
                          .arg(m_sliderSpread + textWidth)
                          .arg(m_sliderSpread + textWidth)
                          .arg(-m_sliderSpread / 2));
    }
}

void QSliderLabeled::paintEvent(QPaintEvent *event)
{
    QSlider::paintEvent(event);
    QPainter painter(this);
    painter.setPen(m_penColor);

    int min = minimum();
    int max = maximum();
    int interval = tickInterval();
    auto intervalAtMaxLabels = (max - min) / m_maxNumberOfLabels;
    // Modify the interval if the current interval would generate too many labels in the slider.
    if (interval == 0 || (max - min) / interval > m_maxNumberOfLabels)
    {
        interval = intervalAtMaxLabels;
    }

    if (orientation() == Qt::Horizontal)
    {
        for (int i = min; i <= max; i += interval)
        {
            int xpos = QStyle::sliderPositionFromValue(min, max, i, width() - 2 * m_grooveMargin) + m_grooveMargin;
            QString label = QString::number(i);
            int textWidth = painter.fontMetrics().horizontalAdvance(label);

            // Ensure labels are not drawn off the widget's area
            xpos = qBound(0, xpos, width()) - textWidth / 2;

            // Adjust the y position to properly fit the text below the slider
            painter.drawText(xpos, height(), label);
        }
    }
    else if (orientation() == Qt::Vertical)
    {
        for (int i = min; i <= max; i += interval)
        {
            int ypos =
                height() - QStyle::sliderPositionFromValue(min, max, i, height() - 2 * m_grooveMargin) - m_grooveMargin;
            QString label = QString::number(i);

            // to position the text, we need the bounding box and not just the text height
            QRect textRect = painter.fontMetrics().tightBoundingRect(label);
            int textHeight = textRect.height();

            // Ensure labels are not drawn off the widget's area
            ypos = qBound(0, ypos, height()) + textHeight / 2;

            // Adjust the x position to properly fit the text to the side of the slider
            painter.drawText(0, ypos, label);
        }
    }
}

void QSliderLabeled::mouseMoveEvent(QMouseEvent *event)
{
    QToolTip::showText(event->globalPosition().toPoint(), QString::number(value()), this);
    QSlider::mouseMoveEvent(event);
}

void QSliderLabeled::UpdatePainterPen()
{
    QColor penColor = isEnabled() ? QColor(255, 215, 64) : QColor(79, 91, 98);
    m_penColor = penColor;
}

void QSliderLabeled::SetGrooveMargin(int value)
{
    m_grooveMargin = value;
}

void QSliderLabeled::SetMaxNumberOfLabels(int value)
{
    m_maxNumberOfLabels = value;
}

void QSliderLabeled::SetSliderSpread(int value)
{
    m_sliderSpread = value;
}
