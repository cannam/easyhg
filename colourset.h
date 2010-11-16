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

#ifndef _COLOURSET_H_
#define _COLOURSET_H_

#include <QSet>
#include <QMap>
#include <QColor>
#include <QString>

class ColourSet
{
public:
    void clearDefaultNames() { m_defaultNames.clear(); }
    void addDefaultName(QString n) { m_defaultNames.insert(n); }

    QColor getColourFor(QString n);

    static ColourSet *instance();

private:
    ColourSet();
    QSet<QString> m_defaultNames;
    QMap<QString, QColor> m_colours;
    QColor m_lastColour;

    static ColourSet m_instance;
};

#endif
