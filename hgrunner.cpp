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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef Q_OS_WIN32
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

HgRunner::HgRunner(QWidget * parent): QProgressBar(parent)
{
    m_proc = 0;

    setTextVisible(false);
    setVisible(false);
    m_isRunning = false;
}

HgRunner::~HgRunner()
{
    closeTerminal();
    if (m_proc) {
        m_proc->kill();
        m_proc->deleteLater();
    }
}

void HgRunner::requestAction(HgAction action)
{
    DEBUG << "requestAction " << action.action << endl;
    bool pushIt = true;
    if (m_queue.empty()) {
        if (action == m_currentAction) {
            // this request is identical to the thing we're executing
            DEBUG << "requestAction: we're already handling this one, ignoring identical request" << endl;
            pushIt = false;
        }
    } else {
        HgAction last = m_queue.back();
        if (action == last) {
            // this request is identical to the previous thing we
            // queued which we haven't executed yet
            DEBUG << "requestAction: we're already queueing this one, ignoring identical request" << endl;
            pushIt = false;
        }
    }
    if (pushIt) m_queue.push_back(action);
    checkQueue();
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
    if (m_ptyFile) {
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
            m_ptyFile->write(QString("%1\n").arg(pwd).toUtf8());
            m_ptyFile->flush();
            return;
        } else {
            DEBUG << "HgRunner::getUsername: user cancelled" << endl;
            killCurrentCommand();
            return;
        }
    }
    // user cancelled or something went wrong
    DEBUG << "HgRunner::getUsername: something went wrong" << endl;
    killCurrentCommand();
}

void HgRunner::getPassword()
{
    if (m_ptyFile) {
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
            m_ptyFile->write(QString("%1\n").arg(pwd).toUtf8());
            m_ptyFile->flush();
            return;
        } else {
            DEBUG << "HgRunner::getPassword: user cancelled" << endl;
            killCurrentCommand();
            return;
        }
    }
    // user cancelled or something went wrong
    DEBUG << "HgRunner::getPassword: something went wrong" << endl;
    killCurrentCommand();
}

bool HgRunner::checkPrompts(QString chunk)
{
    //DEBUG << "checkPrompts: " << chunk << endl;

    QString text = chunk.trimmed();
    QString lower = text.toLower();
    if (lower.endsWith("password:")) {
        getPassword();
        return true;
    }
    if (lower.endsWith("user:")) {
        getUsername();
        return true;
    }
    QRegExp userRe("\\buser:\\s*([^\\s]+)");
    if (userRe.indexIn(text) >= 0) {
        noteUsername(userRe.cap(1));
    }
    QRegExp realmRe("\\brealmr:\\s*([^\\s]+)");
    if (realmRe.indexIn(text) >= 0) {
        noteRealm(realmRe.cap(1));
    }
    return false;
}

void HgRunner::dataReadyStdout()
{
    DEBUG << "dataReadyStdout" << endl;
    QString chunk = QString::fromUtf8(m_proc->readAllStandardOutput());
    if (!checkPrompts(chunk)) {
        m_stdout += chunk;
    }
}

void HgRunner::dataReadyStderr()
{
    DEBUG << "dataReadyStderr" << endl;
    QString chunk = QString::fromUtf8(m_proc->readAllStandardError());
    DEBUG << chunk;
    if (!checkPrompts(chunk)) {
        m_stderr += chunk;
    }
}

void HgRunner::dataReadyPty()
{
    DEBUG << "dataReadyPty" << endl;
    QString chunk = QString::fromUtf8(m_ptyFile->readAll());
    DEBUG << "chunk of " << chunk.length() << " chars" << endl;
    if (!checkPrompts(chunk)) {
        m_stdout += chunk;
    }
}

void HgRunner::finished(int procExitCode, QProcess::ExitStatus procExitStatus)
{
    // Save the current action and reset m_currentAction before we
    // emit a signal to mark the completion; otherwise we may be
    // resetting the action after a slot has already tried to set it
    // to something else to start a new action

    HgAction completedAction = m_currentAction;

    m_isRunning = false;
    m_currentAction = HgAction();

    closeProcInput();
    delete m_proc;
    m_proc = 0;

    if (completedAction.action == ACT_NONE) {
        DEBUG << "HgRunner::finished: WARNING: completed action is ACT_NONE" << endl;
    } else {
        if (procExitCode == 0 && procExitStatus == QProcess::NormalExit) {
            DEBUG << "HgRunner::finished: Command completed successfully"
                  << endl;
            DEBUG << "stdout is " << m_stdout << endl;
            emit commandCompleted(completedAction, m_stdout);
        } else {
            DEBUG << "HgRunner::finished: Command failed, exit code "
                  << procExitCode << ", exit status " << procExitStatus
                  << ", stderr follows" << endl;
            DEBUG << m_stderr << endl;
            emit commandFailed(completedAction, m_stderr);
        }
    }

    checkQueue();
}

void HgRunner::killCurrentCommand()
{
    if (m_isRunning) {
        m_currentAction.action = ACT_NONE; // so that we don't bother to notify
        m_proc->kill();
    }
}

void HgRunner::checkQueue()
{
    if (m_isRunning) {
        return;
    }
    if (m_queue.empty()) {
        hide();
        return;
    }
    HgAction toRun = m_queue.front();
    m_queue.pop_front();
    DEBUG << "checkQueue: have action: running " << toRun.action << endl;
    startCommand(toRun);
}

void HgRunner::startCommand(HgAction action)
{
    QString executable = action.executable;
    bool interactive = false;
    QStringList params = action.params;

    if (executable == "") {
        // This is a Hg command
        executable = getHgBinaryName();
#ifdef Q_OS_WIN32
        // This at least means we won't block on the non-working password prompt
        params.push_front("--noninteractive");
#else
        // password prompt should work here
        if (action.mayBeInteractive()) {
            params.push_front("ui.interactive=true");
            params.push_front("--config");
            interactive = true;
        } else {
            params.push_front("--noninteractive");
        }
    }
#endif

    m_isRunning = true;
    setRange(0, 0);
    if (!action.shouldBeFast()) show();
    m_stdout.clear();
    m_stderr.clear();
    m_realm = "";
    m_userName = "";

    m_proc = new QProcess;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LANG", "en_US.utf8");
    env.insert("LC_ALL", "en_US.utf8");
    env.insert("HGPLAIN", "1");
    m_proc->setProcessEnvironment(env);

    connect(m_proc, SIGNAL(started()), this, SLOT(started()));
    connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(finished(int, QProcess::ExitStatus)));
    connect(m_proc, SIGNAL(readyReadStandardOutput()),
            this, SLOT(dataReadyStdout()));
    connect(m_proc, SIGNAL(readyReadStandardError()),
            this, SLOT(dataReadyStderr()));

    if (!action.workingDir.isEmpty()) {
        m_proc->setWorkingDirectory(action.workingDir);
    }

    if (interactive) {
        openTerminal();
        if (m_ptySlaveFilename != "") {
            DEBUG << "HgRunner: connecting to pseudoterminal" << endl;
            m_proc->setStandardInputFile(m_ptySlaveFilename);
//            m_proc->setStandardOutputFile(m_ptySlaveFilename);
//            m_proc->setStandardErrorFile(m_ptySlaveFilename);
        }
    }

    QString cmdline = executable;
    foreach (QString param, params) cmdline += " " + param;
    DEBUG << "HgRunner: starting: " << cmdline << " with cwd "
          << action.workingDir << endl;

    m_currentAction = action;

    // fill these out with what we actually ran
    m_currentAction.executable = executable;
    m_currentAction.params = params;

    DEBUG << "set current action to " << m_currentAction.action << endl;
    
    m_proc->start(executable, params);
}

void HgRunner::closeProcInput()
{
    DEBUG << "closeProcInput" << endl;

    m_proc->closeWriteChannel();
}

void HgRunner::openTerminal()
{
#ifndef Q_OS_WIN32
    if (m_ptySlaveFilename != "") return; // already open
    DEBUG << "HgRunner::openTerminal: trying to open new pty" << endl;
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    if (master < 0) {
        DEBUG << "openpt failed" << endl;
        perror("openpt failed");
        return;
    }
    struct termios t;
    if (tcgetattr(master, &t)) {
        DEBUG << "tcgetattr failed" << endl;
        perror("tcgetattr failed");
    }
    cfmakeraw(&t);
    if (tcsetattr(master, TCSANOW, &t)) {
        DEBUG << "tcsetattr failed" << endl;
        perror("tcsetattr failed");
    }
    if (grantpt(master)) {
        perror("grantpt failed");
    }
    if (unlockpt(master)) {
        perror("unlockpt failed");
    }
    char *slave = ptsname(master);
    if (!slave) {
        perror("ptsname failed");
        ::close(master);
        return;
    }
    m_ptyMasterFd = master;
    m_ptyFile = new QFile();
    connect(m_ptyFile, SIGNAL(readyRead()), this, SLOT(dataReadyPty()));
    if (!m_ptyFile->open(m_ptyMasterFd, QFile::ReadWrite)) {
        DEBUG << "HgRunner::openTerminal: Failed to open QFile on master fd" << endl;
    }
    m_ptySlaveFilename = slave;
    DEBUG << "HgRunner::openTerminal: succeeded, slave is "
          << m_ptySlaveFilename << endl;
#endif
}

void HgRunner::closeTerminal()
{
#ifndef Q_OS_WIN32
    if (m_ptySlaveFilename != "") {
        delete m_ptyFile;
        m_ptyFile = 0;
        ::close(m_ptyMasterFd);
        m_ptySlaveFilename = "";
    }
#endif
}
