/****************************************************************************
** Copyright (C) Jari Korhonen, 2010 (under lgpl)
****************************************************************************/

#include "hgrunner.h"
#include <QtCore>
#include <QtGui>

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
    connect(proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(finished(int, QProcess::ExitStatus)));
}

HgRunner::~HgRunner()
{
    delete proc;
}

void HgRunner::started()
{
    proc -> closeWriteChannel();
}

void HgRunner::setProcExitInfo(int procExitCode, QProcess::ExitStatus procExitStatus)
{
    exitCode = procExitCode;
    exitStatus = procExitStatus;
    stdOut = QString::fromUtf8(proc -> readAllStandardOutput());
    stdErr = QString::fromUtf8(proc -> readAllStandardError());
    std::cerr << "stdout was " << stdOut.toStdString() << std::endl;
}

void HgRunner::presentErrorToUser()
{
    QPushButton *okButton;
    QListWidget *stdoL;
    QListWidget *stdeL;
    QString tmp;

    QDialog *dlg = new QDialog(this);
    dlg -> setMinimumWidth(800);
    QVBoxLayout layout;

    dlg -> setWindowTitle(tr("Mercurial error / warning"));

    tmp.sprintf("%s: %d, %s: %d", "Exitcode", exitCode, "Exit status", exitStatus);
    layout.addWidget(new QLabel(getLastCommandLine()));
    layout.addWidget(new QLabel(tmp));
    layout.addWidget(new QLabel(tr("Standard out:")));
    stdoL = new QListWidget();
    stdoL -> addItems(stdOut.split("\n"));
    layout.addWidget(stdoL);

    layout.addWidget(new QLabel(tr("Standard error:")));
    stdeL = new QListWidget();
    stdeL -> addItems(stdErr.split("\n"));
    layout.addWidget(stdeL);

    okButton = new QPushButton("Ok");
    layout.addWidget(okButton);

    connect(okButton, SIGNAL(clicked()), dlg, SLOT(accept()));
    dlg -> setLayout(&layout);

    dlg -> setModal(true);
    dlg -> exec();
}



void HgRunner::error(QProcess::ProcessError)
{
    setProcExitInfo(proc -> exitCode(), proc -> exitStatus());

    if (reportErrors)
    {
        presentErrorToUser();
    }

    isRunning = false;
}

QString HgRunner::getLastCommandLine()
{
    return QString("Command line: " + lastHgCommand + " " + lastParams);
}

void HgRunner::finished(int procExitCode, QProcess::ExitStatus procExitStatus)
{
    setProcExitInfo(procExitCode, procExitStatus);

    if (reportErrors)
    {
        if ((exitCode == 0) && (exitStatus == QProcess::NormalExit))
        {
            //All ok
        }
        else
        {
            presentErrorToUser();
        }
    }

    isRunning = false;
}

bool HgRunner::isProcRunning()
{
    return isRunning;
}

void HgRunner::killProc()
{
    if (isProcRunning())
    {
        proc -> kill();
    }
}


void HgRunner::startProc(QString hgExePathAndName, QString workingDir, QStringList params, bool reportErrors)
{
    this -> reportErrors = reportErrors;
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

    lastHgCommand = hgExePathAndName;
    lastParams = params.join(" ");

    std::cerr << "HgRunner: starting: " << hgExePathAndName.toStdString() << " ";
    foreach (QString param, params) std::cerr << param.toStdString() << " ";
    std::cerr << std::endl;

    proc -> start(hgExePathAndName, params);

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


