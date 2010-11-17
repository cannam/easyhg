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

#ifndef HGRUNNER_H
#define HGRUNNER_H

#include <QProgressBar>
#include <QProcess>
#include <QByteArray>
#include <QRect>

class HgRunner : public QProgressBar
{
    Q_OBJECT

public:
    HgRunner(QWidget * parent = 0);
    ~HgRunner();

    void startHgCommand(QString workingDir, QStringList params);
    void startCommand(QString command, QString workingDir, QStringList params);

    bool isCommandRunning();
    void killCurrentCommand();

    int getExitCode();
    QProcess::ExitStatus getExitStatus();

    void hideProgBar();

    QString getStdOut();
    
signals:
    void commandCompleted();
    void commandFailed();

private:
    void saveOutput();
    void setProcExitInfo(int procExitCode, QProcess::ExitStatus procExitStatus);
    QString getLastCommandLine();
    void presentErrorToUser();
    QString getHgBinaryName();
    
    bool                    reportErrors;
    bool                    isRunning;
    QProcess                *proc;
    QString                 stdOut;
    QString                 stdErr;
    int                     exitCode;
    QProcess::ExitStatus    exitStatus;
    QString                 lastHgCommand;
    QString                 lastParams;

private slots:
    void started();
    void finished(int procExitCode, QProcess::ExitStatus procExitStatus);
};

#endif // HGRUNNER_H
