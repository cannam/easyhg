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


#include "common.h"

QString getSystem()
{
    #ifdef Q_WS_X11
    return QString("Linux");
    #endif

    #ifdef Q_WS_MAC
    return QString("Mac");
    #endif

    #ifdef Q_WS_WIN
    return QString("Windows");
    #endif

    return QString("");
}

QString getHgBinaryName()
{
    if (getSystem() == "Windows")
        return QString("hg.exe");
    else
        return QString("hg");
}


QString getHgDirName()
{
    if (getSystem() == "Windows")
    {
        return QString(".hg\\");
    }
    else
    {
        return QString(".hg/");
    }
}






