/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2013 Chris Cannam
    Copyright (c) 2013 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _VERSION_TESTER_H_
#define _VERSION_TESTER_H_

#include <QStringList>
#include <QString>
#include <QObject>
#include <QNetworkReply>

class VersionTester : public QObject
{
    Q_OBJECT

public:
    VersionTester(QString hostname, QString versionFilePath, QString myVersion);
    virtual ~VersionTester();
    
    static bool isVersionNewerThan(QString, QString);

signals:
    void newerVersionAvailable(QString);

protected slots:
    void finished();
    void error(QNetworkReply::NetworkError);

private:
    QString m_myVersion;
    QNetworkReply *m_reply;
    bool m_httpFailed;
};

#endif

