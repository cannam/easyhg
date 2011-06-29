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

#include "hgrunner.h"
#include "common.h"
#include "debug.h"
#include "settingsdialog.h"

#include <QPushButton>
#include <QListWidget>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QSettings>
#include <QInputDialog>
#include <QDesktopServices>
#include <QTemporaryFile>
#include <QDir>

#include <iostream>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef Q_OS_WIN32
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

HgRunner::HgRunner(QString myDirPath, QWidget * parent) :
    QProgressBar(parent),
    m_myDirPath(myDirPath)
{
    m_proc = 0;

    // Always unbundle the extension: even if it already exists (in
    // case we're upgrading) and even if we're not going to use it (so
    // that it's available in case someone wants to use it later,
    // e.g. to fix a malfunctioning setup).  But the path we actually
    // prefer is the one in the settings first, if it exists; then the
    // unbundled one; then anything in the path if for some reason
    // unbundling failed
    unbundleExtension();

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
    if (m_authFilePath != "") {
        QFile(m_authFilePath).remove();
    }
    //!!! and remove any other misc auth file paths...
}

QString HgRunner::getUnbundledFileName()
{
    return SettingsDialog::getUnbundledExtensionFileName();
}

QString HgRunner::unbundleExtension()
{
    // Pull out the bundled Python file into a temporary file, and
    // copy it to our known extension location, replacing the magic
    // text NO_EASYHG_IMPORT_PATH with our installation location

    QString bundled = ":easyhg.py";
    QString unbundled = getUnbundledFileName();

    QString target = QFileInfo(unbundled).path();
    if (!QDir().mkpath(target)) {
        DEBUG << "Failed to make unbundle path " << target << endl;
        std::cerr << "Failed to make unbundle path " << target << std::endl;
        return ""; 
    }

    QFile bf(bundled);
    DEBUG << "unbundle: bundled file will be " << bundled << endl;
    if (!bf.exists() || !bf.open(QIODevice::ReadOnly)) {
        DEBUG << "Bundled extension is missing!" << endl;
        return "";
    }

    QTemporaryFile tmpfile(QString("%1/easyhg.py.XXXXXX").arg(target));
    tmpfile.setAutoRemove(false);
    DEBUG << "unbundle: temp file will be " << tmpfile.fileName() << endl;
    if (!tmpfile.open()) {
        DEBUG << "Failed to open temporary file " << tmpfile.fileName() << endl;
        std::cerr << "Failed to open temporary file " << tmpfile.fileName() << std::endl;
        return "";
    }

    QString all = QString::fromUtf8(bf.readAll());
    all.replace("NO_EASYHG_IMPORT_PATH", m_myDirPath);
    tmpfile.write(all.toUtf8());
    DEBUG << "unbundle: wrote " << all.length() << " characters" << endl;

    tmpfile.close();

    QFile ef(unbundled);
    if (ef.exists()) {
        DEBUG << "unbundle: removing old file " << unbundled << endl;
        ef.remove();
    }
    DEBUG << "unbundle: renaming " << tmpfile.fileName() << " to " << unbundled << endl;
    if (!tmpfile.rename(unbundled)) {
        DEBUG << "Failed to move temporary file to target file " << unbundled << endl;
        std::cerr << "Failed to move temporary file to target file " << unbundled << std::endl;
        return "";
    }
    
    DEBUG << "Unbundled extension to " << unbundled << endl;
    return unbundled;
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
    settings.beginGroup("Locations");
    return settings.value("hgbinary", "").toString();
}

QString HgRunner::getSshBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    return settings.value("sshbinary", "").toString();
}

QString HgRunner::getExtensionLocation()
{
    QSettings settings;
    settings.beginGroup("Locations");
    QString extpath = settings.value("extensionpath", "").toString();
    if (extpath != "" && QFile(extpath).exists()) return extpath;
    return "";
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

    if (!m_currentAction.mayBeInteractive()) return false;

    QString text = chunk.trimmed();
    QString lower = text.toLower();
    if (lower.endsWith("password:")) {
        getPassword();
        return true;
    }
    if (lower.endsWith("user:") || lower.endsWith("username:")) {
        getUsername();
        return true;
    }
    QRegExp userRe("\\buser(name)?:\\s*([^\\s]+)");
    if (userRe.indexIn(text) >= 0) {
        noteUsername(userRe.cap(2));
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
    if (!m_proc) return;
    QString chunk = QString::fromUtf8(m_proc->readAllStandardOutput());
    if (!checkPrompts(chunk)) {
        m_stdout += chunk;
    }
}

void HgRunner::dataReadyStderr()
{
    DEBUG << "dataReadyStderr" << endl;
    if (!m_proc) return;
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

void HgRunner::error(QProcess::ProcessError)
{
    finished(-1, QProcess::CrashExit);
}

void HgRunner::finished(int procExitCode, QProcess::ExitStatus procExitStatus)
{
	if (!m_proc) return;

	// Save the current action and reset m_currentAction before we
    // emit a signal to mark the completion; otherwise we may be
    // resetting the action after a slot has already tried to set it
    // to something else to start a new action

    HgAction completedAction = m_currentAction;

    m_isRunning = false;
    m_currentAction = HgAction();

    //closeProcInput();
    m_proc->deleteLater();
    m_proc = 0;

    if (completedAction.action == ACT_NONE) {
        DEBUG << "HgRunner::finished: WARNING: completed action is ACT_NONE" << endl;
    } else {
        if (procExitCode == 0 && procExitStatus == QProcess::NormalExit) {
            DEBUG << "HgRunner::finished: Command completed successfully"
                  << endl;
//            DEBUG << "stdout is " << m_stdout << endl;
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

void HgRunner::killCurrentActions()
{
    m_queue.clear();
    killCurrentCommand();
}

void HgRunner::killCurrentCommand()
{
    if (m_isRunning) {
        m_currentAction.action = ACT_NONE; // so that we don't bother to notify
        if (m_proc) m_proc->kill();
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

void HgRunner::pruneOldAuthFiles()
{
    QString path = QDesktopServices::storageLocation
        (QDesktopServices::CacheLocation);
    QDir d(path);
    if (!d.exists()) return;
    QStringList filters;
    filters << "easyhg.*.dat";
    QStringList fl = d.entryList(filters);
    foreach (QString f, fl) {
        QStringList parts = f.split('.');
        if (parts.size() > 1) {
            int pid = parts[1].toInt();
            DEBUG << "Checking pid " << pid << " for cache file " << f << endl;

            ProcessStatus ps = GetProcessStatus(pid);
            if (ps == ProcessNotRunning) {
                DEBUG << "Removing stale cache file " << f << endl;
                QDir(d).remove(f);
            }
        }
    }
}

QString HgRunner::getAuthFilePath()
{
    if (m_authFilePath == "") {

        pruneOldAuthFiles();

        QByteArray fileExt = randomKey();
        if (fileExt == QByteArray()) {
            DEBUG << "HgRunner::addExtensionOptions: Failed to get proper auth file ext" << endl;
            return "";
        }
        QString fileExt16 = QString::fromLocal8Bit(fileExt.toBase64()).left(16)
            .replace('+', '-').replace('/', '_');
        QString path = QDesktopServices::storageLocation
            (QDesktopServices::CacheLocation);
        QDir().mkpath(path);
        if (path == "") {
            DEBUG << "HgRunner::addExtensionOptions: Failed to get cache location" << endl;
            return "";
        }

        m_authFilePath = QString("%1/easyhg.%2.%3.dat").arg(path)
            .arg(getpid()).arg(fileExt16);
    }

    return m_authFilePath;
}

QString HgRunner::getAuthKey()
{
    if (m_authKey == "") {
        QByteArray key = randomKey();
        if (key == QByteArray()) {
            DEBUG << "HgRunner::addExtensionOptions: Failed to get proper auth key" << endl;
            return "";
        }
        QString key16 = QString::fromLocal8Bit(key.toBase64()).left(16);
        m_authKey = key16;
    }

    return m_authKey;
}

QStringList HgRunner::addExtensionOptions(QStringList params)
{
    QString extpath = getExtensionLocation();
    if (extpath == "") {
        DEBUG << "HgRunner::addExtensionOptions: Failed to get extension location" << endl;
        return params;
    }

    QString afp = getAuthFilePath();
    QString afk = getAuthKey();

    if (afp != "" && afk != "") {
        params.push_front(QString("easyhg.authkey=%1").arg(m_authKey));
        params.push_front("--config");
        params.push_front(QString("easyhg.authfile=%1").arg(m_authFilePath));
        params.push_front("--config");
    }

    // Looks like this one must be without quotes, even though the SSH
    // one above only works on Windows if it has quotes (at least where
    // there is a space in the path).  Odd
    params.push_front(QString("extensions.easyhg=%1").arg(extpath));
    params.push_front("--config");

    return params;
}

void HgRunner::startCommand(HgAction action)
{
    QString executable = action.executable;
    bool interactive = false;
    QStringList params = action.params;

    if (action.workingDir.isEmpty()) {
        // We require a working directory, never just operate in pwd
        emit commandFailed(action, "EasyMercurial: No working directory supplied, will not run Mercurial command without one");
        return;
    }

    QSettings settings;
    settings.beginGroup("General");

    if (executable == "") {
        // This is a Hg command
        executable = getHgBinaryName();
        if (executable == "") executable = "hg";

        QString ssh = getSshBinaryName();
        if (ssh != "") {
            params.push_front(QString("ui.ssh=\"%1\"").arg(ssh));
            params.push_front("--config");
        }

        if (action.mayBeInteractive()) {
            params.push_front("ui.interactive=true");
            params.push_front("--config");
            if (settings.value("useextension", true).toBool()) {
                params = addExtensionOptions(params);
            }
            interactive = true;
        }            

        //!!! want an option to use the mercurial_keyring extension as well
    }

    m_isRunning = true;
    setRange(0, 0);
    if (!action.shouldBeFast()) show();
    m_stdout.clear();
    m_stderr.clear();
    m_realm = "";
    m_userName = "";

    m_proc = new QProcess;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

#ifdef Q_OS_WIN32
    // On Win32 we like to bundle Hg and other executables with EasyHg
    if (m_myDirPath != "") {
        env.insert("PATH", m_myDirPath + ";" + env.value("PATH"));
    }
#endif

#ifdef Q_OS_MAC
    if (settings.value("python32", false).toBool()) {
        env.insert("VERSIONER_PYTHON_PREFER_32_BIT", "1");
    }
#endif

    env.insert("LANG", "en_US.utf8");
    env.insert("LC_ALL", "en_US.utf8");
    env.insert("HGENCODING", "utf8");
    env.insert("HGPLAIN", "1");
    m_proc->setProcessEnvironment(env);

    connect(m_proc, SIGNAL(started()), this, SLOT(started()));
    connect(m_proc, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(error(QProcess::ProcessError)));
    connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(finished(int, QProcess::ExitStatus)));
    connect(m_proc, SIGNAL(readyReadStandardOutput()),
            this, SLOT(dataReadyStdout()));
    connect(m_proc, SIGNAL(readyReadStandardError()),
            this, SLOT(dataReadyStderr()));

    m_proc->setWorkingDirectory(action.workingDir);

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
    
    emit commandStarting(action);

    m_proc->start(executable, params);
}

void HgRunner::closeProcInput()
{
    DEBUG << "closeProcInput" << endl;

    if (m_proc) m_proc->closeWriteChannel();
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
