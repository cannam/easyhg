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

#ifndef EASYHG_DEBUG_H
#define EASYHG_DEBUG_H

#include <QDebug>
#include <QTextStream>
#include <string>
#include <iostream>

class QString;
class QUrl;

QDebug &operator<<(QDebug &, const std::string &);
std::ostream &operator<<(std::ostream &, const QString &);
std::ostream &operator<<(std::ostream &, const QUrl &);

extern QDebug &getEasyHgDebug();

#define DEBUG getEasyHgDebug()

template <typename T>
inline QDebug &operator<<(QDebug &d, const T &t) {
    QString s;
    QTextStream ts(&s);
    ts << t;
    d << s;
    return d;
}

#endif /* !_DEBUG_H_ */

