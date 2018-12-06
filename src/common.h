/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2013 Chris Cannam
    Copyright (c) 2013 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef COMMON_H
#define COMMON_H

#include <QString>

extern QString findInPath(QString name, QString installPath, bool executable);

extern QString getUserRealName();

extern void loseControllingTerminal();

void installSignalHandlers();

/**
 * Status used in testing whether a folder argument (received from the
 * user) is valid for particular uses.
 */
enum FolderStatus {
    FolderUnknown,      /// Neither the folder nor its parent exists
    FolderParentExists, /// The folder is absent, but its parent exists
    FolderExists,       /// The folder exists and has no .hg repo in it
    FolderHasRepo,      /// The folder exists and has an .hg repo in it
    FolderIsFile        /// The "folder" is actually a file
};

FolderStatus getFolderStatus(QString path);

enum ProcessStatus {
    ProcessRunning,
    ProcessNotRunning,
    UnknownProcessStatus
};

ProcessStatus GetProcessStatus(int pid);

/**
 * If the given path is somewhere within an existing repository,
 * return the path of the root directory of the repository (i.e. the
 * one with .hg in it).
 *
 * If the given path is _not_ in a repository, or the given path _is_
 * the root directory of a repository, return QString().  Use
 * getFolderStatus to distinguish between these cases.
 */
QString getContainingRepoFolder(QString path);

QString xmlEncode(QString);

QString uniDecode(QString);

/**
 * Generate a 16-byte random key using urandom or equivalent
 */
QByteArray randomKey();

int scalePixelSize(int pixels);

#endif 	//COMMON_H


