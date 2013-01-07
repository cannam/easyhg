/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2013 Chris Cannam
    Copyright (c) 2013 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "colourset.h"

ColourSet
ColourSet::m_instance;

ColourSet::ColourSet() { }

ColourSet *
ColourSet::instance()
{
    return &m_instance;
}

QColor
ColourSet::getColourFor(QString n)
{
    if (m_defaultNames.contains(n)) return Qt::black;
    if (m_colours.contains(n)) return m_colours[n];

    QColor c;

    if (m_colours.empty()) {
	c = QColor::fromHsv(0, 200, 150);
    } else {
        int hue = m_lastColour.hue() - 130;
        if (hue < 0) hue += 360;
	c = QColor::fromHsv(hue, 200, 150);
    }

    m_colours[n] = c;
    m_lastColour = c;
    return c;
}


    
