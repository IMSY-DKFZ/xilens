/*******************************************************
 * Author: Intelligent Medical Systems
 * License: see LICENSE.md file
*******************************************************/
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
