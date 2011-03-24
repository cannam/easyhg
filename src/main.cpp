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

#include "mainwindow.h"
#include "common.h"
#include "debug.h"

#include <QApplication>
#include <QTranslator>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setOrganizationName("easymercurial");
    QApplication::setOrganizationDomain("easymercurial.org");
    QApplication::setApplicationName(QApplication::tr("EasyMercurial"));

    // Lose our controlling terminal (so we can provide a new pty to
    // capture password requests)
    loseControllingTerminal();

    installSignalHandlers();

    QTranslator translator;
    QString language = QLocale::system().name();
    QString trname = QString("easyhg_%1").arg(language);
    translator.load(trname, ":");
    app.installTranslator(&translator);

    QStringList args = app.arguments();

    QString myDirPath = QFileInfo(QDir::current().absoluteFilePath(args[0]))
        .canonicalPath();

    MainWindow mainWin(myDirPath);
    mainWin.show();

    if (args.size() == 2) {
        QString path = args[1];
        DEBUG << "Opening " << args[1] << endl;
        if (QDir(path).exists()) {
            path = QDir(path).canonicalPath();
            mainWin.open(path);
        }
    }

    return app.exec();
}
