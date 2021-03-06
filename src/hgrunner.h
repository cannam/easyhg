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

#ifndef HGRUNNER_H
#define HGRUNNER_H

#include "hgaction.h"

#include <QWidget>
#include <QProcess>

class QProgressBar;
class QPushButton;
class QFile;

#include <deque>

class HgRunner : public QWidget
{
    Q_OBJECT

public:
    HgRunner(QString myDirPath, QWidget *parent = 0);
    ~HgRunner();

public slots:
    void requestAction(HgAction action);
    void killCurrentActions(); // kill anything running; clear the queue

signals:
    void commandStarting(HgAction action);
    void commandCompleted(HgAction action, QString stdOut, QString stdErr);
    void commandFailed(HgAction action, QString stdOut, QString stdErr);

    /**
     * Emitted when the currently executing command is cancelled. Note
     * that this clears the queue completely, so all subsequent
     * commands are also discarded.
     */
    void commandCancelled(HgAction action);

private slots:
    void started();
    void error(QProcess::ProcessError);
    void finished(int procExitCode, QProcess::ExitStatus procExitStatus);
    void dataReadyStdout();
    void dataReadyStderr();
    void dataReadyPty();

private:
    void checkQueue();
    void startCommand(HgAction action);
    void closeProcInput();
    void killCurrentCommand();

    void noteUsername(QString);
    void noteRealm(QString);
    void getUsername();
    void getPassword();
    bool checkPrompts(QString);

    void openTerminal();
    void closeTerminal();
    
    QString getHgBinaryName();
    QString getSshBinaryName();
    QString getExtensionLocation();

    QString getAuthFilePath();
    QString getAuthKey();
    void pruneOldAuthFiles();

    QString getUnbundledFileName();
    QString unbundleExtension();

    QProgressBar *m_progress;
    QPushButton *m_cancel;

    HgAction expandEnvironment(HgAction);
    QStringList addExtensionOptions(QStringList);

    int m_ptyMasterFd;
    int m_ptySlaveFd;
    QString m_ptySlaveFilename;
    QFile *m_ptyFile;
    
    bool m_isRunning;
    QProcess *m_proc;
    QString m_stdout;
    QString m_stderr;

    QString m_userName;
    QString m_realm;

    QString m_myDirPath;
    QString m_extensionPath;

    QString m_authKey;
    QString m_authFilePath;

    typedef std::deque<HgAction> ActionQueue;
    ActionQueue m_queue;
    HgAction m_currentAction;
};

#endif // HGRUNNER_H
