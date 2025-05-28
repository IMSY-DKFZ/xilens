#include "display.h"
#include "constants.h"

Displayer::Displayer(QObject *parent)
    : QObject(parent), m_darkColor(DEFAULT_DARK_COLOR), m_saturatedColor(DEFAULT_SATURATION_COLOR)
{
}

Displayer::~Displayer() = default;

void Displayer::StopDisplayer()
{
    this->m_stop = true;
}

void Displayer::StartDisplayer()
{
    this->m_stop = false;
    this->m_displayCondition.notify_one();
}

void Displayer::UpdateLut(int minValue, int maxValue, const QColor &darkColor, const QColor &saturatedColor)
{
}

void Displayer::UpdateBGRChannels(const std::vector<int> &bgrChannels)
{
}
