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

#include "hgrunner.h"
#include "common.h"
#include "debug.h"

#include <QPushButton>
#include <QListWidget>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QSettings>

#include <iostream>
#include <unistd.h>

HgRunner::HgRunner(QWidget * parent): QProgressBar(parent)
{
    proc = new QProcess(this);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LANG", "en_US.utf8");
    env.insert("LC_ALL", "en_US.utf8");
    proc->setProcessEnvironment(env);

    setTextVisible(false);
    setVisible(false);
    isRunning = false;

    stdOut.clear();
    stdErr.clear();

    connect(proc, SIGNAL(started()), this, SLOT(started()));
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(finished(int, QProcess::ExitStatus)));

    reportErrors = false;
}

HgRunner::~HgRunner()
{
    delete proc;
}

QString HgRunner::getHgBinaryName()
{
    QSettings settings;
    QString hg = settings.value("hgbinary", "hg").toString();
    if (hg == "") hg = "hg";
    hg = findExecutable(hg);
    settings.setValue("hgbinary", hg);
    return hg;
}

void HgRunner::started()
{
    proc -> closeWriteChannel();
}

void HgRunner::saveOutput()
{
    stdOut = QString::fromUtf8(proc -> readAllStandardOutput());
    stdErr = QString::fromUtf8(proc -> readAllStandardError());
}

void HgRunner::setProcExitInfo(int procExitCode, QProcess::ExitStatus procExitStatus)
{
    exitCode = procExitCode;
    exitStatus = procExitStatus;

    DEBUG << "setProcExitInfo: " << stdOut.split("\n").size() << " line(s) of stdout, " << stdErr.split("\n").size() << " line(s) of stderr";

//    std::cerr << "stdout was " << stdOut.toStdString() << std::endl;
}

QString HgRunner::getLastCommandLine()
{
    return QString("Command line: " + lastHgCommand + " " + lastParams);
}

void HgRunner::finished(int procExitCode, QProcess::ExitStatus procExitStatus)
{
    setProcExitInfo(procExitCode, procExitStatus);
    isRunning = false;

    if (procExitCode == 0 || procExitStatus == QProcess::NormalExit) {
        emit commandCompleted();
    } else {
        emit commandFailed();
    }
}

bool HgRunner::isCommandRunning()
{
    return isRunning;
}

void HgRunner::killCurrentCommand()
{
    if (isCommandRunning()) {
        proc -> kill();
    }
}

void HgRunner::startHgCommand(QString workingDir, QStringList params)
{
    startCommand(getHgBinaryName(), workingDir, params);
}

void HgRunner::startCommand(QString command, QString workingDir, QStringList params)
{
    isRunning = true;
    setRange(0, 0);
    setVisible(true);
    stdOut.clear();
    stdErr.clear();
    exitCode = 0;
    exitStatus = QProcess::NormalExit;

    if (!workingDir.isEmpty())
    {
        proc -> setWorkingDirectory(workingDir);
    }

    lastHgCommand = command;
    lastParams = params.join(" ");

    QString cmdline = command;
    foreach (QString param, params) cmdline += " " + param;
    DEBUG << "HgRunner: starting: " << cmdline;

    proc -> start(command, params);
}

int HgRunner::getExitCode()
{
    return exitCode;
}

QString HgRunner::getStdOut()
{
    return stdOut;
}

void HgRunner::hideProgBar()
{
    setVisible(false);
}


