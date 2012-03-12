/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2012 Chris Cannam
    Copyright (c) 2012 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "logparser.h"

#include "common.h"
#include "debug.h"

#include <QStringList>
#include <QRegExp>

LogParser::LogParser(QString text, QString separator) :
    m_text(text), m_sep(separator)
{
    m_text.replace("\r\n", "\n");
}

QStringList LogParser::split()
{
    return m_text.split("\n\n", QString::SkipEmptyParts);
}

LogList LogParser::parse()
{
    LogList results;
    QRegExp re(QString("^(\\w+)\\s*%1\\s+([^\\s].*)$").arg(m_sep));
    QStringList entries = split();
    foreach (QString entry, entries) {
        LogEntry dictionary;
        QStringList lines = entry.split('\n');
        foreach (QString line, lines) {
            if (re.indexIn(line) == 0) {
                QString key = re.cap(1);
                QString value = re.cap(2);
                dictionary[key.trimmed()] = uniDecode(value);
            }
        }
        results.push_back(dictionary);
    }
    return results;
}
