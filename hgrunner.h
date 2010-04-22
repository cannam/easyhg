#ifndef HGRUNNER_H
#define HGRUNNER_H

//** Copyright (C) Jari Korhonen, 2010 (under lgpl)

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

        void startProc(QString hgExePathAndName, QString workingDir, QStringList params, bool reportErrors = true);
        bool isProcRunning();
        void killProc();
        int  getExitCode();
        void hideProgBar();
        QString getStdOut();

    private:
        void setProcExitInfo(int procExitCode, QProcess::ExitStatus procExitStatus);
        QString getLastCommandLine();
        void presentErrorToUser();

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
        void error(QProcess::ProcessError error);
        void finished(int procExitCode, QProcess::ExitStatus procExitStatus);
};

#endif // HGRUNNER_H
