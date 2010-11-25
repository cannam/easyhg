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
#include <errno.h>
#include <stdio.h>

#ifndef Q_OS_WIN32
#ifdef Q_OS_MAC
#include <util.h>
#else
#include <pty.h>
#endif
#endif

HgRunner::HgRunner(QWidget * parent): QProgressBar(parent)
{
    m_proc = new QProcess(this);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LANG", "en_US.utf8");
    env.insert("LC_ALL", "en_US.utf8");
    env.insert("HGPLAIN", "1");
    m_proc->setProcessEnvironment(env);

    m_proc->setProcessChannelMode(QProcess::MergedChannels);

    setTextVisible(false);
    setVisible(false);
    m_isRunning = false;

    m_output.clear();

    connect(m_proc, SIGNAL(started()), this, SLOT(started()));
    connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(finished(int, QProcess::ExitStatus)));
    connect(m_proc, SIGNAL(readyRead()), this, SLOT(dataReady()));
}

HgRunner::~HgRunner()
{
    if (m_ptySlaveFilename != "") {
        ::close(m_ptyMasterFd);
    }
    delete m_proc;
}

QString HgRunner::getHgBinaryName()
{
    QSettings settings;
    QString hg = settings.value("hgbinary", "").toString();
    if (hg == "") {
        hg = findExecutable("hg");
    }
    if (hg != "hg") {
        settings.setValue("hgbinary", hg);
    }
    return hg;
}

void HgRunner::started()
{
    DEBUG << "started" << endl;
    /*
    m_proc->write("blah\n");
    m_proc->write("blah\n");
    m_proc -> closeWriteChannel();
    */
}

void HgRunner::setProcExitInfo(int procExitCode, QProcess::ExitStatus procExitStatus)
{
    m_exitCode = procExitCode;
    m_exitStatus = procExitStatus;
}

QString HgRunner::getLastCommandLine()
{
    return QString("Command line: " + m_lastHgCommand + " " + m_lastParams);
}

void HgRunner::noteUsername(QString name)
{
    m_userName = name;
}

void HgRunner::noteRealm(QString realm)
{
    m_realm = realm;
}

void HgRunner::getUsername()
{
    if (m_procInput) {
        bool ok = false;
        QString prompt = tr("User name:");
        if (m_realm != "") {
            prompt = tr("User name for \"%1\":").arg(m_realm);
        }
        QString pwd = QInputDialog::getText
            (qobject_cast<QWidget *>(parent()),
            tr("Enter user name"), prompt,
            QLineEdit::Normal, QString(), &ok);
        if (ok) {
            m_procInput->write(QString("%1\n").arg(pwd).toUtf8());
            m_procInput->flush();
            return;
        }
    }
    // user cancelled or something went wrong
    killCurrentCommand();
}

void HgRunner::getPassword()
{
    if (m_procInput) {
        bool ok = false;
        QString prompt = tr("Password:");
        if (m_userName != "") {
            if (m_realm != "") {
                prompt = tr("Password for \"%1\" at \"%2\":")
                         .arg(m_userName).arg(m_realm);
            } else {
                prompt = tr("Password for user \"%1\":")
                         .arg(m_userName);
            }
        }
        QString pwd = QInputDialog::getText
            (qobject_cast<QWidget *>(parent()),
            tr("Enter password"), prompt,
             QLineEdit::Password, QString(), &ok);
        if (ok) {
            m_procInput->write(QString("%1\n").arg(pwd).toUtf8());
            m_procInput->flush();
            return;
        }
    }
    // user cancelled or something went wrong
    killCurrentCommand();
}

void HgRunner::checkPrompts(QString chunk)
{
    //DEBUG << "checkPrompts: " << chunk << endl;

    QString text = chunk.trimmed();
    QString lower = text.toLower();
    if (lower.endsWith("password:")) {
        getPassword();
        return;
    }
    if (lower.endsWith("user:")) {
        getUsername();
        return;
    }
    QRegExp userRe("\\buser:\\s*([^\\s]+)");
    if (userRe.indexIn(text) >= 0) {
        noteUsername(userRe.cap(1));
    }
    QRegExp realmRe("\\brealmr:\\s*([^\\s]+)");
    if (realmRe.indexIn(text) >= 0) {
        noteRealm(realmRe.cap(1));
    }
}

void HgRunner::dataReady()
{
    DEBUG << "dataReady" << endl;
    QString chunk = QString::fromUtf8(m_proc->readAll());
    m_output += chunk;
    checkPrompts(chunk);
}

void HgRunner::finished(int procExitCode, QProcess::ExitStatus procExitStatus)
{
    setProcExitInfo(procExitCode, procExitStatus);
    m_isRunning = false;

    closeProcInput();

    if (procExitCode == 0 && procExitStatus == QProcess::NormalExit) {
        DEBUG << "HgRunner::finished: Command completed successfully" << endl;
        emit commandCompleted();
    } else {
        DEBUG << "HgRunner::finished: Command failed" << endl;
        emit commandFailed();
    }
}

bool HgRunner::isCommandRunning()
{
    return m_isRunning;
}

void HgRunner::killCurrentCommand()
{
    if (isCommandRunning()) {
        m_proc -> kill();
    }
}

void HgRunner::startHgCommand(QString workingDir, QStringList params)
{
#ifdef Q_OS_WIN32
    // This at least means we won't block on the non-working password prompt
    params.push_front("ui.interactive=false");
#else
    // password prompt should work here
    params.push_front("ui.interactive=true");
#endif
    params.push_front("--config");
    startCommand(getHgBinaryName(), workingDir, params);
}

void HgRunner::startCommand(QString command, QString workingDir, QStringList params)
{
    m_isRunning = true;
    setRange(0, 0);
    setVisible(true);
    m_output.clear();
    m_exitCode = 0;
    m_exitStatus = QProcess::NormalExit;
    m_realm = "";
    m_userName = "";

    if (!workingDir.isEmpty()) {
        m_proc->setWorkingDirectory(workingDir);
    }

    m_procInput = 0;
#ifndef Q_OS_WIN32
    char name[1024];
    if (openpty(&m_ptyMasterFd, &m_ptySlaveFd, name, NULL, NULL)) {
        perror("openpty failed");
    } else {
        DEBUG << "openpty succeeded: master " << m_ptyMasterFd
                << " slave " << m_ptySlaveFd << " filename " << name << endl;
        m_procInput = new QFile;
        m_procInput->open(m_ptyMasterFd, QFile::WriteOnly);
        m_ptySlaveFilename = name;
        m_proc->setStandardInputFile(m_ptySlaveFilename);
        ::close(m_ptySlaveFd);
    }
#endif

    m_lastHgCommand = command;
    m_lastParams = params.join(" ");

    QString cmdline = command;
    foreach (QString param, params) cmdline += " " + param;
    DEBUG << "HgRunner: starting: " << cmdline << " with cwd "
          << workingDir << endl;

    m_proc->start(command, params);
}

void HgRunner::closeProcInput()
{
    DEBUG << "closeProcInput" << endl;

    m_proc->closeWriteChannel();
#ifndef Q_OS_WIN32
    if (m_ptySlaveFilename != "") {
        ::close(m_ptyMasterFd);
        m_ptySlaveFilename = "";
    }
#endif
}

int HgRunner::getExitCode()
{
    return m_exitCode;
}

QString HgRunner::getOutput()
{
    return m_output;
}

void HgRunner::hideProgBar()
{
    setVisible(false);
}


