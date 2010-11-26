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

#include "hgaction.h"

#include <QProgressBar>
#include <QProcess>
#include <QByteArray>
#include <QRect>
#include <QFile>

#include <deque>

class HgRunner : public QProgressBar
{
    Q_OBJECT

public:
    HgRunner(QWidget * parent = 0);
    ~HgRunner();

    void requestAction(HgAction action);
/*
    bool isCommandRunning();
    void killCurrentCommand();

    void hideProgBar();
*/    
signals:
    void commandCompleted(HgAction action, QString stdout);
    void commandFailed(HgAction action, QString stderr);

private slots:
    void started();
    void finished(int procExitCode, QProcess::ExitStatus procExitStatus);
    void dataReady();

private:
    void checkQueue();
    void startCommand(HgAction action);
    QString getHgBinaryName();
    void closeProcInput();
    void killCurrentCommand();

    void noteUsername(QString);
    void noteRealm(QString);
    void getUsername();
    void getPassword();
    void checkPrompts(QString);

    int m_ptyMasterFd;
    int m_ptySlaveFd;
    QString m_ptySlaveFilename;
    QFile *m_procInput;
    
    bool m_isRunning;
    QProcess *m_proc;
    QString m_output;

    QString m_userName;
    QString m_realm;

    typedef std::deque<HgAction> ActionQueue;
    ActionQueue m_queue;
    HgAction m_currentAction;
};

#endif // HGRUNNER_H
