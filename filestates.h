/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef FILESTATES_H
#define FILESTATES_H

#include <QStringList>
#include <QMap>
#include <QString>

class FileStates
{
public:
    FileStates();

    enum State {

        // These are in the order in which they want to be listed in
        // the interface

        Modified,
        Added,
        Removed,
        InConflict,
        Missing,
        Unknown,
        Clean,
        Ignored,

        FirstState = Modified,
        LastState = Ignored
    };

    void parseStates(QString text);

    void clearBuckets();

    QStringList getFilesInState(State) const;

    QStringList modified() const { return m_modified; }
    QStringList added() const { return m_added; }
    QStringList unknown() const { return m_unknown; }
    QStringList removed() const { return m_removed; }
    QStringList missing() const { return m_missing; }
    QStringList inConflict() const { return m_inConflict; }
    QStringList clean() const { return m_clean; }
    QStringList ignored() const { return m_ignored; }

    State getStateOfFile(QString file) const;

private:
    QStringList m_modified;
    QStringList m_added;
    QStringList m_unknown;
    QStringList m_removed;
    QStringList m_missing;
    QStringList m_inConflict;
    QStringList m_clean;
    QStringList m_ignored;
    QMap<QString, State> m_stateMap;

    State charToState(QChar, bool * = 0);
    QStringList *stateToBucket(State);
};

#endif // FILESTATES_H
