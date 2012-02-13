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

#ifndef FSWATCHER_H
#define FSWATCHER_H

#include <QObject>
#include <QMutex>
#include <QString>
#include <QSet>
#include <QHash>
#include <QMap>
#include <QStringList>
#include <QFileSystemWatcher>

class FsWatcher : public QObject
{
    Q_OBJECT

public:
    FsWatcher();
    virtual ~FsWatcher();

    /**
     * Set the root path of the work directory to be monitored. This
     * directory and all its subdirectories (recursively) will be
     * monitored for changes.
     *
     * Calling this also clears the tracked file path set. Call
     * setTrackedFilePaths afterwards to ensure non-directory files
     * are monitored.
     */
    void setWorkDirPath(QString path);

    /**
     * Provide a set of paths for files which should be tracked. These
     * will be the only non-directory files monitored for changes.
     */
    void setTrackedFilePaths(QStringList paths);
    
    /**
     * Provide a set of prefixes to ignore. Files whose names start
     * with a prefix in this set will be ignored when they change.
     */
    void setIgnoredFilePrefixes(QStringList prefixes);

    /**
     * Provide a set of suffixes to ignore. Files whose names end
     * with a suffix in this set will be ignored when they change.
     */
    void setIgnoredFileSuffixes(QStringList suffixes);

    /**
     * Return a token to identify the current caller in subsequent
     * calls to getChangedPaths().  Only changes that occur after this
     * has been called can be detected by the caller.
     */
    int getNewToken();
    
    /**
     * Return a list of all non-ignored file paths that have been
     * observed to have changed since the last call to getChangedPaths
     * with the same token.
     */
    QSet<QString> getChangedPaths(int token);
    
signals:
    /**
     * Emitted when something has changed. Use the asynchronous
     * interface to find out what.
     */
    void changed();

private slots:
    void fsDirectoryChanged(QString);
    void fsFileChanged(QString);

private:
    // call with lock already held
    void addWorkDirectory(QString path);

    // call with lock already held
    bool shouldIgnore(QString path);

    // call with lock already held. Returns set of non-ignored files in dir
    QSet<QString> scanDirectory(QString path);

    // call with lock already held
    void debugPrint();

private:
    /**
     * A change associates a filename with a counter (the
     * size_t). Each time a file is changed, its counter is assigned
     * from m_lastCounter and m_lastCounter is incremented.
     *
     * This is not especially efficient -- we often want to find "all
     * files whose counter is greater than X" which involves a
     * traversal. Maybe something better later.
     */
    QHash<QString, size_t> m_changes;

    /**
     * Associates a token (the client identifier) with a counter. The
     * counter represents the value of m_lastCounter at the moment
     * getChangedPaths last completed for that token. Any files in
     * m_changes whose counters are larger must have been modified.
     */
    QMap<int, size_t> m_tokenMap;

    /**
     * Associates a directory path with a list of all the files in it
     * that do not match our ignore patterns. When a directory is
     * signalled as having changed, then we need to rescan it and
     * compare against this list before we can determine whether to
     * notify about the change or not.
     */
    QHash<QString, QSet<QString> > m_dirContents;

    QStringList m_ignoredPrefixes;
    QStringList m_ignoredSuffixes;

    /// Everything in this class is synchronised.
    QMutex m_mutex;

    QString m_workDirPath;
    int m_lastToken;
    size_t m_lastCounter;
    QFileSystemWatcher m_watcher;
};

#endif
