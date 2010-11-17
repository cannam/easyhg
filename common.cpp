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

#include <sys/types.h>
#include <pwd.h>

QString findExecutable(QString name)
{
    if (name != "") {
        if (name[0] != '/') {
            name = QFileInfo(name).fileName();
            QString path =
                QProcessEnvironment::systemEnvironment().value("PATH");
            DEBUG << "findExecutable: seeking location for binary " << name
                  << ": system path is " << path;
#ifdef Q_OS_WIN32
            QChar pathSep = ';';
#else
            QChar pathSep = ':';
#endif
            QStringList elements = path.split(pathSep, QString::SkipEmptyParts);
            foreach (QString element, elements) {
                QString full = QString("%1/%2").arg(element).arg(name);
                QFileInfo fi(full);
                if (fi.exists() && fi.isFile() && fi.isExecutable()) {
                    name = full;
                    break;
                }
            }
        }
    }
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
    const int maxlen = 1023;
    TCHAR buf[maxlen + 2];
    LPTSTR info = buf;

    if (!GetUserNameEx(NameDisplay, info, maxlen)) {
        return "";
    }

    return QString::fromUcs2(info);
}
#elif Q_OS_MAC
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

