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
#include <QInputDialog>

#include <iostream>
#include <unistd.h>
#include <pty.h>
#include <errno.h>
#include <stdio.h>

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

    procInput = 0;
    char name[1024];
    if (openpty(&ptyMasterFd, &ptySlaveFd, name, NULL, NULL)) {
        perror("openpty failed");
    } else {
        DEBUG << "openpty succeeded: master " << ptyMasterFd
                << " slave " << ptySlaveFd << " filename " << name << endl;
        procInput = new QFile;
        procInput->open(ptyMasterFd, QFile::WriteOnly);
        ptySlaveFilename = name;
        proc->setStandardInputFile(ptySlaveFilename);
        ::close(ptySlaveFd);
    }

    connect(proc, SIGNAL(started()), this, SLOT(started()));
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(finished(int, QProcess::ExitStatus)));
    connect(proc, SIGNAL(readyReadStandardOutput()),
            this, SLOT(stdOutReady()));
    connect(proc, SIGNAL(readyReadStandardError()),
            this, SLOT(stdErrReady()));

    reportErrors = false;
}

HgRunner::~HgRunner()
{
    if (ptySlaveFilename != "") {
        ::close(ptyMasterFd);
//        ::close(ptySlaveFd);
    }
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
    /*
    if (procInput) procInput->write("blah\n");
    if (procInput) procInput->write("blah\n");
    if (procInput) {
        procInput->close();
//        ::close(ptyMasterFd);
    }
    proc -> closeWriteChannel();
    */
}

void HgRunner::saveOutput()
{
    stdOut += QString::fromUtf8(proc -> readAllStandardOutput());
    stdErr += QString::fromUtf8(proc -> readAllStandardError());

    DEBUG << "saveOutput: " << stdOut.split("\n").size() << " line(s) of stdout, " << stdErr.split("\n").size() << " line(s) of stderr" << endl;

//    std::cerr << "stdout was " << stdOut.toStdString() << std::endl;
}

void HgRunner::setProcExitInfo(int procExitCode, QProcess::ExitStatus procExitStatus)
{
    exitCode = procExitCode;
    exitStatus = procExitStatus;
}

QString HgRunner::getLastCommandLine()
{
    return QString("Command line: " + lastHgCommand + " " + lastParams);
}

void HgRunner::stdOutReady()
{
    DEBUG << "stdOutReady" << endl;
    QString chunk = QString::fromUtf8(proc->readAllStandardOutput());
    DEBUG << "stdout was " << chunk << endl;
    stdOut += chunk;
}

void HgRunner::stdErrReady()
{
    DEBUG << "stdErrReady" << endl;
    QString chunk = QString::fromUtf8(proc->readAllStandardError());
    DEBUG << "stderr was " << chunk << endl;
    stdErr += chunk;
    if (procInput) {
        if (chunk.toLower().trimmed() == "password:") {
            bool ok = false;
            QString pwd = QInputDialog::getText
                (qobject_cast<QWidget *>(parent()),
                 tr("Enter password"), tr("Password (but for what user name and repository??"),
                 QLineEdit::Password, QString(), &ok);
            if (ok) {
                procInput->write(QString("%1\n").arg(pwd).toUtf8());
                procInput->flush();
            } else {
                //!!! do what? close the terminal?
            }
        }
    }
}

void HgRunner::finished(int procExitCode, QProcess::ExitStatus procExitStatus)
{
    setProcExitInfo(procExitCode, procExitStatus);
    saveOutput();
    isRunning = false;

    if (procExitCode == 0 && procExitStatus == QProcess::NormalExit) {
        DEBUG << "HgRunner::finished: Command completed successfully: stderr says: " << stdErr << endl;
        emit commandCompleted();
    } else {
        DEBUG << "HgRunner::finished: Command failed: stderr says: " << stdErr << endl;
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
    DEBUG << "HgRunner: starting: " << cmdline << " with cwd "
          << workingDir << endl;

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


