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

#include "changeset.h"

#include <QVariant>

Changeset::Changeset(const LogEntry &e)
{
    foreach (QString key, e.keys()) {
        if (key == "parents") {
            QStringList parents = e.value(key).split
                (" ", QString::SkipEmptyParts);
            setParents(parents);
        } else if (key == "timestamp") {
            setTimestamp(e.value(key).split(" ")[0].toULongLong());
        } else {
            setProperty(key.toLocal8Bit().data(), e.value(key));
        }
    }
}

