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

#include "common.h"
#include "debug.h"

#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStringList>
#include <QDir>
#include <QRegExp>

#include <sys/types.h>

#ifdef Q_OS_WIN32
#define _WIN32_WINNT 0x0500
#define SECURITY_WIN32 
#include <windows.h>
#include <security.h>
#else
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#endif

QString findInPath(QString name, QString installPath, bool executableRequired)
{
    bool found = false;
    if (name != "") {
        if (name[0] != '/'
#ifdef Q_OS_WIN32
            && (QRegExp("^\\w:").indexIn(name) != 0)
#endif
            ) {
#ifdef Q_OS_WIN32
            QChar pathSep = ';';
#else
            QChar pathSep = ':';
#endif
            name = QFileInfo(name).fileName();
            QString path =
                QProcessEnvironment::systemEnvironment().value("PATH");
            DEBUG << "findInPath: seeking location for binary " << name
                  << ": system path is " << path << endl;
            if (installPath != "") {   
                DEBUG << "findInPath: install path is " << installPath
                      << ", adding to system path" << endl;
                //!!! path = path + pathSep + installPath;
                path = installPath + pathSep + path;
            }
#ifndef Q_OS_WIN32
            //!!!
            path = path + ":/usr/local/bin";
            DEBUG << "... adding /usr/local/bin just in case (fix and add settings dlg please)"
                    << endl;
#endif
            QStringList elements = path.split(pathSep, QString::SkipEmptyParts);
            foreach (QString element, elements) {
                QString full = QDir(element).filePath(name);
                QFileInfo fi(full);
                DEBUG << "findInPath: looking at " << full << endl;
                if (fi.exists() && fi.isFile()) {
                    DEBUG << "findInPath: it's a file" << endl;
                    if (!executableRequired || fi.isExecutable()) {
                        name = full;
                        DEBUG << "findInPath: found at " << name << endl;
                        found = true;
                        break;
                    }
                }
            }
        } else {
            // absolute path given
            QFileInfo fi(name);
            DEBUG << "findInPath: looking at absolute path " << name << endl;
            if (fi.exists() && fi.isFile()) {
                DEBUG << "findInPath: it's a file" << endl;
                if (!executableRequired || fi.isExecutable()) {
                    DEBUG << "findInPath: found at " << name << endl;
                    found = true;
                }
            }
        }
    }
#ifdef Q_OS_WIN32
    if (!found) {
        if (!name.endsWith(".exe")) {
            return findInPath(name + ".exe", installPath, executableRequired);
        }
    }
#endif
    if (found) {
        return name;
    } else {
        return "";
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

    if (!isatty(0)) {
        DEBUG << "stdin is not a terminal" << endl;
    } else {
        DEBUG << "stdin is a terminal, detaching from it" << endl;
        if (ioctl(0, TIOCNOTTY, NULL) < 0) {
            perror("ioctl failed");
            DEBUG << "ioctl for TIOCNOTTY on stdin failed (errno = " << errno << ")" << endl;
        } else {
            DEBUG << "ioctl for TIOCNOTTY on stdin succeeded" << endl;
	    return;
        }
    }

    int ttyfd = open("/dev/tty", O_RDWR);
    if (ttyfd < 0) {
        DEBUG << "failed to open controlling terminal" << endl;
    } else {
        if (ioctl(ttyfd, TIOCNOTTY, NULL) < 0) {
            perror("ioctl failed");
            DEBUG << "ioctl for TIOCNOTTY on controlling terminal failed (errno = " << errno << ")" << endl;
        } else {
            DEBUG << "ioctl for TIOCNOTTY on controlling terminal succeeded" << endl;
	    return;
        }
    }

#endif
}

void installSignalHandlers()
{
#ifndef Q_OS_WIN32
    sigset_t sgnals;
    sigemptyset (&sgnals);
    sigaddset(&sgnals, SIGHUP);
    sigaddset(&sgnals, SIGCONT);
    pthread_sigmask(SIG_BLOCK, &sgnals, 0);
#endif
}

FolderStatus getFolderStatus(QString path)
{
    if (path != "/" && path.endsWith("/")) {
        path = path.left(path.length()-1);
    }
    DEBUG << "getFolderStatus: " << path << endl;
    QFileInfo fi(path);
    if (fi.exists()) {
        DEBUG << "exists" << endl;
        QDir dir(path);
        if (!dir.exists()) { // returns false for files
            DEBUG << "not directory" << endl;
            return FolderIsFile;
        }
        if (QDir(dir.filePath(".hg")).exists()) {
            DEBUG << "has repo" << endl;
            return FolderHasRepo;
        }
        return FolderExists;
    } else {
        QDir parent = fi.dir();
        if (parent.exists()) {
            DEBUG << "parent exists" << endl;
            return FolderParentExists;
        }
        return FolderUnknown;
    }
}

QString getContainingRepoFolder(QString path)
{
    if (getFolderStatus(path) == FolderHasRepo) return "";

    QFileInfo me(path);
    QFileInfo parent(me.dir().absolutePath());

    while (me != parent) {
        QString parentPath = parent.filePath();
        if (getFolderStatus(parentPath) == FolderHasRepo) {
            return parentPath;
        }
        me = parent;
        parent = me.dir().absolutePath();
    }

    return "";
}

QString xmlEncode(QString s)
{
    s
	.replace("&", "&amp;")
	.replace("<", "&lt;")
	.replace(">", "&gt;")
	.replace("\"", "&quot;")
	.replace("'", "&apos;");

    return s;
}
