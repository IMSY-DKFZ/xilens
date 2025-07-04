/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
 *******************************************************/

#include <QColorDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionToolButton>
#include <QToolTip>

#include "constants.h"
#include "widgets.h"

QSliderLabeled::QSliderLabeled(QWidget *parent) : QSlider(parent), m_labelInterval(0), m_displayLabels(true)
{
    m_penColor = QColor(255, 215, 64);
}

void QSliderLabeled::ApplyStyleSheet()
{
    const QFontMetrics fm(font());
    const QString maxLabel = QString::number(maximum());
    const int textWidth = fm.horizontalAdvance(maxLabel);
    const int textHeight = fm.height();

    if (orientation() == Qt::Orientation::Horizontal)
    {
        setStyleSheet(QString("QSlider{"
                              " min-height: %1px;"
                              " max-height: %2px;"
                              " padding-top: %3px;"
                              "}")
                          .arg(m_displayLabels ? m_sliderSpread + textHeight : m_sliderSpread)
                          .arg(m_displayLabels ? m_sliderSpread + textHeight : m_sliderSpread)
                          .arg(m_displayLabels ? -m_sliderSpread / 2 : 0));
    }
    else if (orientation() == Qt::Orientation::Vertical)
    {
        setStyleSheet(QString("QSlider{"
                              " min-width: %1px;"
                              " max-width: %2px;"
                              " padding-right: %3px;"
                              "}")
                          .arg(m_displayLabels ? m_sliderSpread + textWidth : m_sliderSpread)
                          .arg(m_displayLabels ? m_sliderSpread + textWidth : m_sliderSpread)
                          .arg(m_displayLabels ? -m_sliderSpread / 2 : 0));
    }
}

void QSliderLabeled::paintEvent(QPaintEvent *event)
{
    QSlider::paintEvent(event);
    if (!m_displayLabels)
    {
        return;
    }
    QPainter painter(this);
    painter.setPen(m_penColor);

    const int min = minimum();
    const int max = maximum();
    const auto interval = GetLabelInterval();

    if (orientation() == Qt::Horizontal)
    {
        for (int i = min; i <= max; i += interval)
        {
            int xpos = QStyle::sliderPositionFromValue(min, max, i, width() - 2 * m_grooveMargin) + m_grooveMargin;
            QString label = QString::number(i);
            const int textWidth = painter.fontMetrics().horizontalAdvance(label);

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
            const int textHeight = textRect.height();

            // Ensure labels are not drawn off the widget's area
            ypos = qBound(0, ypos, height()) + textHeight / 2;

            // Adjust the x position to properly fit the text to the side of the slider
            painter.drawText(0, ypos, label);
        }
    }
}

int QSliderLabeled::GetLabelInterval() const
{
    const int min = minimum();
    const int max = maximum();
    if (!m_labelInterval)
    {
        int interval = tickInterval();
        const auto intervalAtMaxLabels = (max - min) / m_maxNumberOfLabels;
        // Modify the interval if the current interval would generate too many labels in the slider.
        if (interval == 0 || (max - min) / interval > m_maxNumberOfLabels)
        {
            interval = intervalAtMaxLabels;
        }
        return interval;
    }
    return m_labelInterval;
}

void QSliderLabeled::mouseMoveEvent(QMouseEvent *event)
{
    QToolTip::showText(event->globalPosition().toPoint(), QString::number(value()), this);
    QSlider::mouseMoveEvent(event);
}

void QSliderLabeled::UpdatePainterPen()
{
    const QColor penColor = isEnabled() ? QColor(255, 215, 64) : QColor(79, 91, 98);
    m_penColor = penColor;
}

QSliderPopup::QSliderPopup(const int min, const int max, const int value, const Qt::Orientation orientation,
                           QWidget *parent)
    : QWidget(nullptr), m_slider(new QSliderLabeled(this)), m_layout(new QVBoxLayout(this)), m_frame(new QFrame(this))
{
    // Make the widget a frameless popup
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName("QSliderPopupWindow");

    m_slider->setRange(min, max);
    m_slider->setValue(value);
    m_slider->setOrientation(orientation);

    m_backgroundFrameLayout = new QHBoxLayout(m_frame);
    StyleQFrameInPopupWindow(m_frame, m_backgroundFrameLayout, m_windowBorderRadius, m_contentMargin);
    m_backgroundFrameLayout->addWidget(m_slider);

    m_layout->addWidget(m_frame);
}

void StyleQFrameInPopupWindow(QFrame *frame, QLayout *layout, const int borderRadius, const int contentMargin)
{
    frame->setObjectName("BackgroundFrame");
    frame->setStyleSheet(QString("#BackgroundFrame { border: 3px solid %1; border-radius: %2px; }")
                             .arg(COLOR_UI_PRIMARY, QString::number(borderRadius)));
    layout->setContentsMargins(contentMargin, contentMargin, contentMargin, contentMargin);
}

QLineSpinPopup::QLineSpinPopup(QWidget *parent)
    : QWidget(nullptr), m_lineEdit(new QLineEdit(this)), m_spinBox(new QSpinBox(this)), m_layout(new QVBoxLayout(this)),
      m_frame(new QFrame(this))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName("QLineSpinPopupWindow");

    m_spinBox->setRange(1, std::numeric_limits<int>::max());
    m_spinBox->setValue(1);

    m_backgroundFrameLayout = new QHBoxLayout(m_frame);
    StyleQFrameInPopupWindow(m_frame, m_backgroundFrameLayout, m_windowBorderRadius, m_contentMargin);
    m_backgroundFrameLayout->addWidget(m_lineEdit);
    m_backgroundFrameLayout->addWidget(m_spinBox);
    m_backgroundFrameLayout->setSpacing(10);

    m_layout->addWidget(m_frame);
}

QDoubleSpinBoxesPopup::QDoubleSpinBoxesPopup(QWidget *parent)
    : QWidget(nullptr), m_spinBox1(new QSpinBox(this)), m_spinBox2(new QSpinBox(this)), m_layout(new QHBoxLayout(this)),
      m_frame(new QFrame(this))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName("QDoubleSpinBoxesPopupWindow");

    m_spinBox1->setRange(1, std::numeric_limits<int>::max());
    m_spinBox2->setRange(1, std::numeric_limits<int>::max());
    m_spinBox1->setValue(UNDEREXPOSURE_PIXEL_BOUNDARY_VALUE);
    m_spinBox2->setValue(OVEREXPOSURE_PIXEL_BOUNDARY_VALUE);
    m_spinBox1->setToolTip("Minimum value");
    m_spinBox2->setToolTip("Maximum value");
    m_spinBox1->setMinimumWidth(90);
    m_spinBox2->setMinimumWidth(90);

    m_backgroundFrameLayout = new QHBoxLayout(m_frame);
    StyleQFrameInPopupWindow(m_frame, m_backgroundFrameLayout, m_windowBorderRadius, m_contentMargin);
    m_backgroundFrameLayout->addWidget(m_spinBox1);
    m_backgroundFrameLayout->addWidget(m_spinBox2);
    m_backgroundFrameLayout->setSpacing(10);

    m_layout->addWidget(m_frame);

    connect(m_spinBox1, &QSpinBox::valueChanged, this, &QDoubleSpinBoxesPopup::minValueChanged);
    connect(m_spinBox2, &QSpinBox::valueChanged, this, &QDoubleSpinBoxesPopup::maxValueChanged);
}

QDoubleSpinBoxesWithColorPickersPopup::QDoubleSpinBoxesWithColorPickersPopup(QWidget *parent)
    : QDoubleSpinBoxesPopup(parent), m_leftColorButton(new QPushButton(this)),
      m_rightColorButton(new QPushButton(this)), m_leftColor(DEFAULT_DARK_COLOR), m_rightColor(DEFAULT_SATURATION_COLOR)
{
    // Set a fixed size for color buttons
    m_leftColorButton->setFixedSize(30, 30);
    m_rightColorButton->setFixedSize(30, 30);

    // Update initial button styles
    this->updateColorButtonStyles();

    // Insert color buttons into the layout
    m_backgroundFrameLayout->insertWidget(0, m_leftColorButton);
    m_backgroundFrameLayout->addWidget(m_rightColorButton);

    // Connect signals
    connect(m_leftColorButton, &QPushButton::clicked, this,
            &QDoubleSpinBoxesWithColorPickersPopup::onLeftColorButtonClicked);
    connect(m_rightColorButton, &QPushButton::clicked, this,
            &QDoubleSpinBoxesWithColorPickersPopup::onRightColorButtonClicked);
}

QColor QDoubleSpinBoxesWithColorPickersPopup::getLeftColor() const
{
    return m_leftColor;
}

QColor QDoubleSpinBoxesWithColorPickersPopup::getRightColor() const
{
    return m_rightColor;
}

void QDoubleSpinBoxesWithColorPickersPopup::setLeftColor(const QColor &color)
{
    if (m_leftColor != color)
    {
        m_leftColor = color;
        this->updateColorButtonStyles();
        emit leftColorChanged(color);
    }
}

void QDoubleSpinBoxesWithColorPickersPopup::setRightColor(const QColor &color)
{
    if (m_rightColor != color)
    {
        m_rightColor = color;
        this->updateColorButtonStyles();
        emit rightColorChanged(color);
    }
}

void QDoubleSpinBoxesWithColorPickersPopup::setToolTips(const QString &leftColorTooltip,
                                                        const QString &rightColorTooltip,
                                                        const QString &minValueTooltip,
                                                        const QString &maxValueTooltip) const
{
    m_leftColorButton->setToolTip(leftColorTooltip);
    m_rightColorButton->setToolTip(rightColorTooltip);
    m_spinBox1->setToolTip(minValueTooltip);
    m_spinBox2->setToolTip(maxValueTooltip);
}

void QDoubleSpinBoxesWithColorPickersPopup::onLeftColorButtonClicked()
{
    if (const QColor color = QColorDialog::getColor(m_leftColor, this, "Select Left Color"); color.isValid())
    {
        this->setLeftColor(color);
    }
}

void QDoubleSpinBoxesWithColorPickersPopup::onRightColorButtonClicked()
{
    if (const QColor color = QColorDialog::getColor(m_rightColor, this, "Select Right Color"); color.isValid())
    {
        this->setRightColor(color);
    }
}

void QDoubleSpinBoxesWithColorPickersPopup::updateColorButtonStyles() const
{
    const QString buttonStyle = "QPushButton { background-color: %1; border: 0px solid #666; border-radius: 4px; }";
    m_leftColorButton->setStyleSheet(buttonStyle.arg(m_leftColor.name()));
    m_rightColorButton->setStyleSheet(buttonStyle.arg(m_rightColor.name()));
}

QArrowToolButton::QArrowToolButton(QWidget *parent) : QToolButton(parent)
{
}

void QArrowToolButton::paintEvent(QPaintEvent *event)
{
    QToolButton::paintEvent(event);
    QPainter painter(this);
    QStyleOptionToolButton option;
    initStyleOption(&option);

    // Determine arrow color based on the button state
    QColor arrowColor;
    if (!isEnabled())
    {
        arrowColor = 0x4f5b62;
    }
    else if (option.state == QStyle::State_Sunken || option.state == QStyle::State_MouseOver)
    {
        arrowColor = 0x707070;
    }
    else
    {
        arrowColor = 0xffffff;
    }

    // Draw the arrow
    QPolygon arrow;
    QRect rect = ArrowRect();
    arrow << QPoint(rect.left(), rect.bottom()) << QPoint(rect.right(), rect.top())
          << QPoint(rect.right(), rect.bottom());

    painter.setBrush(QBrush(arrowColor));
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(arrow);
}

void QArrowToolButton::mousePressEvent(QMouseEvent *event)
{
    // Check if the mouse click is inside the arrow's rectangle
    if (ArrowRect().contains(event->pos()))
    {
        emit ArrowClicked();
        return;
    }
    QToolButton::mousePressEvent(event);
}

QRect QArrowToolButton::ArrowRect() const
{
    int arrowSize = qMin(width(), height()) / 5;
    // Ensure arrowSize is always an odd number, makes painted arrow straight
    if (arrowSize % 2 == 0)
    {
        arrowSize++;
    }
    return {width() - arrowSize, height() - arrowSize, arrowSize, arrowSize};
}

QRgbChannelSpinBoxesPopup::QRgbChannelSpinBoxesPopup(QWidget *parent)
    : QWidget(parent), m_spinBox1(new QSpinBox(this)), m_spinBox2(new QSpinBox(this)), m_spinBox3(new QSpinBox(this)),
      m_layout(new QHBoxLayout(this)), m_frame(new QFrame(this))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName("QRgbChannelSpinBoxesPopupWindow");

    m_spinBox1->setRange(1, std::numeric_limits<int>::max());
    m_spinBox2->setRange(1, std::numeric_limits<int>::max());
    m_spinBox3->setRange(1, std::numeric_limits<int>::max());
    m_spinBox1->setToolTip("Red channel");
    m_spinBox2->setToolTip("Green channel");
    m_spinBox3->setToolTip("Blue channel");
    m_spinBox1->setMinimumWidth(90);
    m_spinBox2->setMinimumWidth(90);
    m_spinBox3->setMinimumWidth(90);

    m_backgroundFrameLayout = new QHBoxLayout(m_frame);
    StyleQFrameInPopupWindow(m_frame, m_backgroundFrameLayout, m_windowBorderRadius, m_contentMargin);
    QString labelStyle = QString("<b><font color='%1'>%2:</font></b>");
    m_backgroundFrameLayout->addWidget(new QLabel(labelStyle.arg(COLOR_UI_PRIMARY).arg("R")), 0, Qt::AlignRight);
    m_backgroundFrameLayout->addWidget(m_spinBox1);
    m_backgroundFrameLayout->addWidget(new QLabel(labelStyle.arg(COLOR_UI_PRIMARY).arg("G")), 0, Qt::AlignRight);
    m_backgroundFrameLayout->addWidget(m_spinBox2);
    m_backgroundFrameLayout->addWidget(new QLabel(labelStyle.arg(COLOR_UI_PRIMARY).arg("B")), 0, Qt::AlignRight);
    m_backgroundFrameLayout->addWidget(m_spinBox3);
    m_backgroundFrameLayout->setSpacing(10);

    m_layout->addWidget(m_frame);

    connect(m_spinBox1, &QSpinBox::valueChanged, this, &QRgbChannelSpinBoxesPopup::RedValueChanged);
    connect(m_spinBox2, &QSpinBox::valueChanged, this, &QRgbChannelSpinBoxesPopup::GreenValueChanged);
    connect(m_spinBox3, &QSpinBox::valueChanged, this, &QRgbChannelSpinBoxesPopup::BlueValueChanged);
}

void QRgbChannelSpinBoxesPopup::UpdateRgb(const int red, const int green, const int blue) const
{
    m_spinBox1->setValue(red);
    m_spinBox2->setValue(green);
    m_spinBox3->setValue(blue);
    emit ValueChanged({red, green, blue});
}

void QRgbChannelSpinBoxesPopup::RedValueChanged(const int value) const
{
    emit ValueChanged({m_spinBox1->value(), m_spinBox2->value(), m_spinBox3->value()});
}

void QRgbChannelSpinBoxesPopup::GreenValueChanged(const int value) const
{
    emit ValueChanged({m_spinBox1->value(), m_spinBox2->value(), m_spinBox3->value()});
}

void QRgbChannelSpinBoxesPopup::BlueValueChanged(const int value) const
{
    emit ValueChanged({m_spinBox1->value(), m_spinBox2->value(), m_spinBox3->value()});
}

std::vector<int> QRgbChannelSpinBoxesPopup::getRgb() const
{
    return {m_spinBox1->value(), m_spinBox2->value(), m_spinBox3->value()};
}
