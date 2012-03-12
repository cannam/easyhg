/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2012 Chris Cannam
    Copyright (c) 2012 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <QMutexLocker>
#include <QDir>

#ifdef Q_OS_MAC
// Must include this before debug.h
#include <CoreServices/CoreServices.h>
#endif

#include "fswatcher.h"
#include "debug.h"

#include <deque>

//#define DEBUG_FSWATCHER 1

/*
 * Watching the filesystem is trickier than it seems at first glance.
 *
 * We ideally should watch every directory, and every file that is
 * tracked by Hg. If a new file is created in a directory, then we
 * need to respond in order to show it as a potential candidate to be
 * added.
 *
 * Complicating matters though is that Hg itself might modify the
 * filesystem. This can happen even in "read-only" operations: for
 * example, hg stat creates files called hg-checklink and hg-checkexec
 * to test properties of the filesystem. So we need to know to ignore
 * those files; unfortunately, when watching a directory (which is how
 * we find out about the creation of new files) we are notified only
 * that the directory has changed -- we aren't told what changed.
 *
 * This means that, when a directory changes, we need to rescan the
 * directory to learn whether the set of files in it _excluding_ files
 * matching our ignore patterns differs from the previous scan, and
 * ignore the change if it doesn't.
 *
 */

/*
 * 20120312 -- Another complication. The documentation for
 * QFileSystemWatcher says:
 *
 *     On Mac OS X 10.4 [...] an open file descriptor is required for
 *     each monitored file. [...] This means that addPath() and
 *     addPaths() will fail if your process tries to add more than 256
 *     files or directories to the file system monitor [...] Mac OS X
 *     10.5 and up use a different backend and do not suffer from this
 *     issue.
 * 
 * Unfortunately, the last sentence above is not true:
 * http://qt.gitorious.org/qt/qt/commit/6d1baf9979346d6f15da81a535becb4046278962
 * ("Removing the usage of FSEvents-based backend for now as it has a
 * few bugs...").  It can't be restored without hacking the Qt source,
 * which we don't want to do in this context. The commit log doesn't
 * make clear how serious the bugs were -- an example is given but it
 * doesn't indicate whether it's an edge case or a common case and
 * whether the result was a crash or failure to notify.
 *
 * This means the Qt class uses kqueue instead on OS/X, but that
 * doesn't really work for us -- it can only monitor 256 files (or
 * whatever the fd ulimit is set to, but that's the default) and it
 * doesn't notify if a file within a directory is modified unless the
 * metadata changes. The main limitation of FSEvents is that it only
 * notifies with directory granularity, but that's OK for us so long
 * as notifications are actually provoked by file changes as well.
 *
 * One other problem with FSEvents is that the API only exists on OS/X
 * 10.5 or newer -- on older versions we would have no option but to
 * use kqueue via QFileSystemWatcher. But we can't ship a binary
 * linked with the FSEvents API to run on 10.4 without some fiddling,
 * and I'm not really keen to do that either.  That may be our cue to
 * drop 10.4 support for EasyMercurial.
 */

FsWatcher::FsWatcher() :
    m_lastToken(0),
    m_lastCounter(0)
{
#ifdef Q_OS_MAC
    m_stream = 0; // create when we have a path
#else
    connect(&m_watcher, SIGNAL(directoryChanged(QString)),
	    this, SLOT(fsDirectoryChanged(QString)));
    connect(&m_watcher, SIGNAL(fileChanged(QString)),
	    this, SLOT(fsFileChanged(QString)));
#endif
}

FsWatcher::~FsWatcher()
{
}

void
FsWatcher::setWorkDirPath(QString path)
{
    QMutexLocker locker(&m_mutex);
    if (m_workDirPath == path) return;
    clearWatchedPaths();
    m_workDirPath = path;
    addWorkDirectory(path);
    debugPrint();
}

void
FsWatcher::clearWatchedPaths()
{
#ifdef Q_OS_MAC
    FSEventStreamRef stream = (FSEventStreamRef)m_stream;
    if (stream) {
        FSEventStreamStop(stream);
        FSEventStreamInvalidate(stream);
        FSEventStreamRelease(stream);
    }
    m_stream = 0;
#else
    // annoyingly, removePaths prints a warning if given an empty list
    if (!m_watcher.directories().empty()) {
        m_watcher.removePaths(m_watcher.directories());
    }
    if (!m_watcher.files().empty()) {
        m_watcher.removePaths(m_watcher.files());
    }
#endif
}

#ifdef Q_OS_MAC
static void
fsEventsCallback(FSEventStreamRef streamRef,
                 void *clientCallBackInfo,
                 int numEvents,
                 const char *const eventPaths[],
                 const FSEventStreamEventFlags *eventFlags,
                 const uint64_t *eventIDs)
{
}
#endif

void
FsWatcher::addWorkDirectory(QString path)
{
#ifdef Q_OS_MAC
    FSEventStreamRef stream =
        FSEventStreamCreate(kCFAllocatorDefault,
                            (FSEventStreamCallback)&fsEventsCallback,
                            this,
                            cfPaths,
                            kFSEventStreamEventIdSinceNow,
                            1.0, // latency, seconds
                            kFSEventStreamCreateFlagNone);

    m_stream = stream;
    
    FSEventStreamScheduleWithRunLoop(stream,
                                     CFRunLoopGetCurrent(),
                                     kCFRunLoopDefaultMode);

    if (!FSEventStreamStart(stream)) {
        std::cerr << "ERROR: FsWatcher::addWorkDirectory: Failed to start FSEvent stream" << std::endl;
    }
#else
    // QFileSystemWatcher will refuse to add a file or directory to
    // its watch list that it is already watching -- fine -- but it
    // prints a warning when this happens, which we wouldn't want.  So
    // we'll check for duplicates ourselves.
    QSet<QString> alreadyWatched = 
	QSet<QString>::fromList(m_watcher.directories());
    
    std::deque<QString> pending;
    pending.push_back(path);

    while (!pending.empty()) {

        QString path = pending.front();
        pending.pop_front();
        if (!alreadyWatched.contains(path)) {
            m_watcher.addPath(path);
            m_dirContents[path] = scanDirectory(path);
        }

        QDir d(path);
        if (d.exists()) {
            d.setFilter(QDir::Dirs | QDir::NoDotAndDotDot |
                        QDir::Readable | QDir::NoSymLinks);
            foreach (QString entry, d.entryList()) {
                if (entry.startsWith('.')) continue;
                QString entryPath = d.absoluteFilePath(entry);
                pending.push_back(entryPath);
            }
        }
    }
#endif
}

void
FsWatcher::setTrackedFilePaths(QStringList paths)
{
#ifdef Q_OS_MAC
    // We don't need to do anything here, so long as addWorkDirectory
    // has been called -- FSEvents monitors files within directories
    // as well as the directories themselves (even though it only
    // notifies with directory granularity)
#else
    QMutexLocker locker(&m_mutex);

    QSet<QString> alreadyWatched = 
	QSet<QString>::fromList(m_watcher.files());

    foreach (QString path, paths) {
        path = m_workDirPath + QDir::separator() + path;
        if (!alreadyWatched.contains(path)) {
            m_watcher.addPath(path);
        } else {
            alreadyWatched.remove(path);
        }
    }

    // Remove the remaining paths, those that were being watched
    // before but that are not in the list we were given
    foreach (QString path, alreadyWatched) {
        m_watcher.removePath(path);
    }

    debugPrint();
#endif
}

void
FsWatcher::setIgnoredFilePrefixes(QStringList prefixes)
{
    QMutexLocker locker(&m_mutex);
    m_ignoredPrefixes = prefixes;
}

void
FsWatcher::setIgnoredFileSuffixes(QStringList suffixes)
{
    QMutexLocker locker(&m_mutex);
    m_ignoredSuffixes = suffixes;
}

int
FsWatcher::getNewToken()
{
    QMutexLocker locker(&m_mutex);
    int token = ++m_lastToken;
    m_tokenMap[token] = m_lastCounter;
    return token;
}

QSet<QString>
FsWatcher::getChangedPaths(int token)
{
    QMutexLocker locker(&m_mutex);
    size_t lastUpdatedAt = m_tokenMap[token];
    QSet<QString> changed;
    for (QHash<QString, size_t>::const_iterator i = m_changes.begin();
	 i != m_changes.end(); ++i) {
	if (i.value() > lastUpdatedAt) {
	    changed.insert(i.key());
	}
    }
    m_tokenMap[token] = m_lastCounter;
    return changed;
}

void
FsWatcher::fsDirectoryChanged(QString path)
{
    {
	QMutexLocker locker(&m_mutex);

	if (shouldIgnore(path)) return;

        QSet<QString> files = scanDirectory(path);
        if (files == m_dirContents[path]) {
#ifdef DEBUG_FSWATCHER
            std::cerr << "FsWatcher: Directory " << path << " has changed, but not in a way that we are monitoring" << std::endl;
#endif
            return;
        } else {
#ifdef DEBUG_FSWATCHER
            std::cerr << "FsWatcher: Directory " << path << " has changed" << std::endl;
#endif
            m_dirContents[path] = files;
        }

        size_t counter = ++m_lastCounter;
        m_changes[path] = counter;
    }

    emit changed();
}

void
FsWatcher::fsFileChanged(QString path)
{
    {
        QMutexLocker locker(&m_mutex);

        // We don't check whether the file matches an ignore pattern,
        // because we are only notified for file changes if we are
        // watching the file explicitly, i.e. the file is in the
        // tracked file paths list. So we never want to ignore them

#ifdef DEBUG_FSWATCHER
        std::cerr << "FsWatcher: Tracked file " << path << " has changed" << std::endl;
#endif

        size_t counter = ++m_lastCounter;
        m_changes[path] = counter;
    }

    emit changed();
}

bool
FsWatcher::shouldIgnore(QString path)
{
    QFileInfo fi(path);
    QString fn(fi.fileName());
    foreach (QString pfx, m_ignoredPrefixes) {
        if (fn.startsWith(pfx)) {
#ifdef DEBUG_FSWATCHER
            std::cerr << "(ignoring: " << path << ")" << std::endl;
#endif
            return true;
        }
    }
    foreach (QString sfx, m_ignoredSuffixes) {
        if (fn.endsWith(sfx)) {
#ifdef DEBUG_FSWATCHER
            std::cerr << "(ignoring: " << path << ")" << std::endl;
#endif
            return true;
        }
    }
    return false;
}

QSet<QString>
FsWatcher::scanDirectory(QString path)
{
    QSet<QString> files;
    QDir d(path);
    if (d.exists()) {
        d.setFilter(QDir::Files | QDir::NoDotAndDotDot |
                    QDir::Readable | QDir::NoSymLinks);
        foreach (QString entry, d.entryList()) {
            if (entry.startsWith('.')) continue;
            if (shouldIgnore(entry)) continue;
            files.insert(entry);
        }
    }
//    std::cerr << "scanDirectory:" << std::endl;
//    foreach (QString f, files) std::cerr << f << std::endl;
    return files;
}

void
FsWatcher::debugPrint()
{
#ifdef DEBUG_FSWATCHER
    std::cerr << "FsWatcher: Now watching " << m_watcher.directories().size()
              << " directories and " << m_watcher.files().size()
              << " files" << std::endl;
#endif
}
