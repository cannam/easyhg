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

    bool isInState(QString file, State s) const;
    QStringList filesInState(State s) const;
    State stateOf(QString file) const;
    bool isKnown(QString file) const;

    enum Activity {

        // These are in the order in which they want to be listed in
        // the context menu

        Diff,
        Annotate,

        Commit,
        Revert,

        Rename,
        Copy,

        Add,
        Remove,

        RedoMerge,
        MarkResolved,

        Ignore,
        UnIgnore,

        FirstActivity = Diff,
        LastActivity = UnIgnore
    };

    typedef QList<Activity> Activities;

    static bool supportsActivity(State s, Activity a);
    static Activities activitiesSupportedBy(State s);
    static int activityGroup(Activity a);
    
    bool supportsActivity(QString file, Activity a) const;
    QStringList filesSupportingActivity(Activity) const;
    Activities activitiesSupportedBy(QString file) const;

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

    void clearBuckets();

    State charToState(QChar, bool * = 0);
    QStringList *stateToBucket(State);
};

#endif // FILESTATES_H
