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

#include "debug.h"

#include <QString>
#include <QUrl>
#include <QMutex>
#include <QMutexLocker>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QStandardPaths>

#include <cstdio>

QDebug &
getEasyHgDebug()
{
    static QFile *logFile = 0;
    static QDebug *debug = 0;
    static QMutex mutex;
    static char *prefix;
    mutex.lock();
    if (!debug) {
        prefix = new char[20];
        sprintf(prefix, "[%lu]", (unsigned long)QCoreApplication::applicationPid());
        QString logFileName = QDir::homePath() + "/.easyhg.log"; // the fallback
#if QT_VERSION >= 0x050400
        QString logDir = QStandardPaths::writableLocation
            (QStandardPaths::AppDataLocation);
#else
        QString logDir = QStandardPaths::writableLocation
            (QStandardPaths::DataLocation);
#endif
        if (logDir != "" &&
            (QDir(logDir).exists() ||
             QDir().mkpath(logDir))) {
            logFileName = logDir + "/debug.log";
        }
        logFile = new QFile(logFileName);
        if (logFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QDebug(QtDebugMsg) << (const char *)prefix
                               << "Opened debug log file "
                               << logFile->fileName();
            debug = new QDebug(logFile);
        } else {
            QDebug(QtWarningMsg) << (const char *)prefix
                                 << "Failed to open debug log file "
                                 << logFile->fileName()
                                 << " for writing, using console debug instead";
            debug = new QDebug(QtDebugMsg);
            delete logFile;
            logFile = 0;
        }
        *debug << endl << (const char *)prefix << "Log started at "
               << QDateTime::currentDateTime().toString();
    }
    mutex.unlock();

    QDebug &dref = *debug;
    return dref << endl << (const char *)prefix;
}

QDebug &
operator<<(QDebug &dbg, const std::string &s)
{
    dbg << QString::fromUtf8(s.c_str());
    return dbg;
}

std::ostream &
operator<<(std::ostream &target, const QString &str)
{
    return target << str.toLocal8Bit().data();
}

std::ostream &
operator<<(std::ostream &target, const QUrl &u)
{
    return target << "<" << u.toString() << ">";
}

