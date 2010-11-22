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

#include "common.h"
#include "debug.h"

#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStringList>
#include <QDir>

#include <sys/types.h>

#ifdef Q_OS_WIN32
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <security.h>
#else
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#endif

QString findExecutable(QString name)
{
    bool found = false;
    if (name != "") {
        if (name[0] != '/') {
#ifdef Q_OS_WIN32
            QChar pathSep = ';';
#else
            QChar pathSep = ':';
#endif
            name = QFileInfo(name).fileName();
            QString path =
                QProcessEnvironment::systemEnvironment().value("PATH");
            DEBUG << "findExecutable: seeking location for binary " << name
                  << ": system path is " << path << endl;
#ifndef Q_OS_WIN32
            path = path + ":/usr/local/bin";
            DEBUG << "... adding /usr/local/bin just in case (fix and add settings dlg please)"
                    << endl;
#endif
            QStringList elements = path.split(pathSep, QString::SkipEmptyParts);
            foreach (QString element, elements) {
                QString full = QDir(element).filePath(name);
                QFileInfo fi(full);
                if (fi.exists() && fi.isFile() && fi.isExecutable()) {
                    name = full;
                    found = true;
                    break;
                }
            }
        }
    }
#ifdef Q_OS_WIN32
    if (!found) {
        if (!name.endsWith(".exe")) {
            return findExecutable(name + ".exe");
        }
    }
#endif
    return name;
}
    
QString getSystem()
{
    #ifdef Q_WS_X11
    return QString("Linux");
    #endif

    #ifdef Q_WS_MAC
    return QString("Mac");
    #endif

    #ifdef Q_WS_WIN
    return QString("Windows");
    #endif

    return QString("");
}

QString getHgDirName()
{
    if (getSystem() == "Windows")
    {
        return QString(".hg\\");
    }
    else
    {
        return QString(".hg/");
    }
}

#ifdef Q_OS_WIN32
QString getUserRealName()
{
    TCHAR buf[1024];
    long unsigned int maxlen = 1000;
    LPTSTR info = buf;

    if (!GetUserNameEx(NameDisplay, info, &maxlen)) {
        DEBUG << "GetUserNameEx failed: " << GetLastError() << endl;
        return "";
    }

#ifdef UNICODE
    return QString::fromUtf16((const unsigned short *)info);
#else
    return QString::fromLocal8Bit(info);
#endif
}
#else
#ifdef Q_OS_MAC
// Nothing here: definition is in common_osx.mm
#else
QString getUserRealName()
{
    const int maxlen = 1023;
    char buf[maxlen + 2];

    if (getlogin_r(buf, maxlen)) return "";

    struct passwd *p = getpwnam(buf);
    if (!p) return "";
    
    QString s(p->pw_gecos);
    if (s != "") s = s.split(',')[0];
    return s;
}
#endif
#endif

void loseControllingTerminal()
{
#ifndef Q_OS_WIN32
    pid_t sid;
    switch (fork()) {
    case 0:
        sid = setsid();
        if (sid != (pid_t)-1) {
            DEBUG << "setsid() succeeded, session ID is " << sid << endl;
        } else {
            perror("setsid failed");
            DEBUG << "setsid() failed (errno = " << errno << ")" << endl;
        }
        switch (fork()) {
        case 0:
            return;
        case -1:
            perror("fork failed");
            DEBUG << "second fork failed (errno = " << errno << ")" << endl;
            return;
        default:
            exit(0);
        }
    case -1:
        perror("fork failed");
        DEBUG << "fork failed (errno = " << errno << ")" << endl;
        return;
    default:
        exit(0);
    }
#endif
}
