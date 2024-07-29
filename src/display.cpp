#include "display.h"

Displayer::Displayer() {}

Displayer::~Displayer() {}

void Displayer::StopDisplayer() { this->m_stop = true; }

void Displayer::StartDisplayer() { this->m_stop = false; }
