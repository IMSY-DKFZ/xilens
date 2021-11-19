/*
 * ===================================================================
 * Surgical Spectral Imaging Library (SuSI)
 *
 * Copyright (c) German Cancer Research Center,
 * Division of Medical and Biological Informatics.
 * All rights reserved.
 *
 * This software is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 *
 * See LICENSE.txt for details.
 * ===================================================================
 */


#ifndef DISPLAY_H
#define DISPLAY_H


#include <QObject>

#include "xiApi.h"


class Displayer : public QObject
{
    Q_OBJECT

public:

    explicit Displayer();
    ~Displayer();


protected:

    virtual void CreateWindows() = 0;
    virtual void DestroyWindows() = 0;


public slots:

    virtual void Display(XI_IMG& image) = 0;
};
#endif // DISPLAY_H
