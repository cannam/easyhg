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

#include "fswatcher.h"
#include "debug.h"

#include <QMutexLocker>
#include <QDir>

#include <deque>

#define DEBUG_FSWATCHER 1

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
 */

FsWatcher::FsWatcher() :
    m_lastToken(0),
    m_lastCounter(0)
{
    connect(&m_watcher, SIGNAL(directoryChanged(QString)),
	    this, SLOT(fsDirectoryChanged(QString)));
    connect(&m_watcher, SIGNAL(fileChanged(QString)),
	    this, SLOT(fsFileChanged(QString)));
}

FsWatcher::~FsWatcher()
{
}

void
FsWatcher::setWorkDirPath(QString path)
{
    QMutexLocker locker(&m_mutex);
    if (m_workDirPath == path) return;
    m_watcher.removePaths(m_watcher.directories());
    m_watcher.removePaths(m_watcher.files());
    m_workDirPath = path;
    addWorkDirectory(path);
    debugPrint();
}

void
FsWatcher::setTrackedFilePaths(QStringList paths)
{
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
}

void
FsWatcher::addWorkDirectory(QString path)
{
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

        std::cerr << "FsWatcher: Tracked file " << path << " has changed" << std::endl;

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
            std::cerr << "(ignoring: " << path << ")" << std::endl;
            return true;
        }
    }
    foreach (QString sfx, m_ignoredSuffixes) {
        if (fn.endsWith(sfx)) {
            std::cerr << "(ignoring: " << path << ")" << std::endl;
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
