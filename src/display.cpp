#include "display.h"

Displayer::Displayer(QObject *parent) : QObject(parent)
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
