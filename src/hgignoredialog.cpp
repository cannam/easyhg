/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "hgignoredialog.h"
#include "common.h"
#include "debug.h"

HgIgnoreDialog::IgnoreType
HgIgnoreDialog::confirmIgnore(QStringList files, QStringList suffixes)
{
    QString text = "<qt>";

    if (files.size() < 10) {
        text += tr("You have asked to ignore the following files:");
        text += "<p><code>";
        foreach (QString f, files) {
            text += "&nbsp;&nbsp;&nbsp;" + xmlEncode(f) + "<br>";
        }
        text += "</code>";
    } else {
        text += "<p>";
        text += tr("You have asked to ignore %1 file(s).", "", files.size());
        text += "</p>";
    }

    if (suffixes.size() > 0) {

        text += "<p>";
        text += tr("Would you like to:");
        text += "<ul>";


	//...
    
    }


    return IgnoreNothing;
}

