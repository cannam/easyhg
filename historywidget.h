/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2010 Chris Cannam
    Copyright (c) 2010 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef HISTORYWIDGET_H
#define HISTORYWIDGET_H

#include "changeset.h"

#include <QWidget>

class Panned;
class Panner;

class HistoryWidget : public QWidget
{
    Q_OBJECT

public:
    HistoryWidget();
    virtual ~HistoryWidget();

    void parseLog(QString log);
    
private:
    Changesets m_changesets;

    Panned *m_panned;
    Panner *m_panner;

    void clearChangesets();
    Changesets parseChangeSets(QString);
};

#endif