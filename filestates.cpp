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

#include "filestates.h"

#include "debug.h"

#include <QMap>

FileStates::FileStates()
{
}

void FileStates::clearBuckets()
{
    m_modified.clear();
    m_added.clear();
    m_removed.clear();
    m_missing.clear();
    m_unknown.clear();
}

FileStates::State FileStates::charToState(QChar c, bool *ok)
{
    if (ok) *ok = true;
    if (c == 'M') return Modified;
    if (c == 'A') return Added;
    if (c == 'R') return Removed;
    if (c == '!') return Missing;
    if (c == '?') return Unknown;
    if (c == 'C') return Clean;
    if (ok) *ok = false;
    return Unknown;
}

QStringList *FileStates::stateToBucket(State s)
{
    switch (s) {
    case Clean: default: return 0; // not implemented yet
    case Modified: return &m_modified;
    case Added: return &m_added;
    case Unknown: return &m_unknown;
    case Removed: return &m_removed;
    case Missing: return &m_missing;
    }
}

void FileStates::parseStates(QString text)
{
    text.replace("\r\n", "\n");

    clearBuckets();

    QStringList lines = text.split("\n", QString::SkipEmptyParts);

    foreach (QString line, lines) {

        if (line.length() < 3 || line[1] != ' ') {
            continue;
        }

        QChar c = line[0];
        bool ok = false;
        State s = charToState(c, &ok);
        if (!ok) continue;

        QString file = line.right(line.length() - 2);
        QStringList *bucket = stateToBucket(s);
        bucket->push_back(file);
        m_stateMap[file] = s;
    }

    DEBUG << "FileStates: " << m_modified.size() << " modified, " << m_added.size()
            << " added, " << m_removed.size() << " removed, " << m_missing.size()
            << " missing, " << m_unknown.size() << " unknown" << endl;
}

QStringList FileStates::getFilesInState(State s) const
{
    QStringList *sl = const_cast<FileStates *>(this)->stateToBucket(s);
    if (sl) return *sl;
    else return QStringList();
}

FileStates::State FileStates::getStateOfFile(QString file) const
{
    if (m_stateMap.contains(file)) {
        return m_stateMap[file];
    }
    DEBUG << "FileStates: WARNING: getStateOfFile: file "
            << file << " is unknown to us: returning Unknown state, "
            << "but unknown to us is not supposed to be the same "
            << "thing as unknown state..."
            << endl;
    return Unknown;
}
