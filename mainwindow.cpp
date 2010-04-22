/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
** Copyright (C) Jari Korhonen, 2010 (HgExplorer specific parts, under lgpl)
*************************************************************************************/

#include <QtGui>
#include <QStringList>
#include <QDir>

#include "mainwindow.h"
#include "settingsdialog.h"


MainWindow::MainWindow()
{
    QString wndTitle;

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    timerId = startTimer(200);
    runner = new HgRunner(this);
    runningAction = ACT_NONE;
    statusBar()->addPermanentWidget(runner);

    wndTitle.sprintf("%s %s", APPNAME, APPVERSION);
    setWindowTitle(wndTitle);

    remoteRepoPath = "";
    workFolderPath = "";

    readSettings();

    tabPage = 0;
    hgExp = new HgExpWidget((QWidget *) this, remoteRepoPath, workFolderPath, initialFileTypesBits);
    setCentralWidget(hgExp);

    setUnifiedTitleAndToolBarOnMac(true);
    connectActions();
    enableDisableActions();

    if (firstStart)
    {
        QMessageBox::information(this, tr("First start todo"), tr("Going to \"Settings\" first."));
        settings();
    }

    hgStat();
}


void MainWindow::closeEvent(QCloseEvent *)
{
    writeSettings();
}


void MainWindow::about()
{
   QMessageBox::about(this, tr("About HgExplorer"),
                      tr("<b>HgExplorer</b> tries to be Mercurial's <b>VSS Explorer:</b> ;-)<br><br>"
                        "-Hides command line in normal use<br>"
                        "-Makes common operations easier<br><br>"
                        "(c) 2010 (lgpl), Jari Korhonen (jtkorhonen@gmail.com)<br><br>"
                        "-Needs Mercurial ;-) (thanks Matt Mackall, Bryan O'Sullivan and others !)<br>"
                        "-Uses excellent Nuvola icons (c) David Vignoni (Thanks, David !)<br>"
                        "-Needs Qt4, mingw (in windows), python, kdiff3 (Thanks to all of you !)<br>"));
}


void MainWindow::hgStat()
{
    if (hgStatAct -> isEnabled())
    {
        if (runningAction == ACT_NONE)
        {
            QStringList params;

            QString statFlags = hgExp -> getStatFlags();
            if (statFlags.isEmpty())
            {
                params << "stat";
            }
            else
            {
                params << "stat" << "-" + statFlags;
            }


            runner -> startProc(getHgBinaryName(), workFolderPath, params);
            runningAction = ACT_STAT;
        }
    }
}

void MainWindow::hgHeads()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;
        params << "heads";

        //on empty repos, "hg heads" will fail, don't care of that.
        runner -> startProc(getHgBinaryName(), workFolderPath, params, false);
        runningAction = ACT_HEADS;
    }
}

void MainWindow::hgLog()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;
        params << "glog" << "--verbose";

        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_LOG;
    }
}


void MainWindow::hgParents()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;
        params << "parents";

        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_PARENTS;
    }
}



void MainWindow::hgRemove()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;
        QString currentFile = hgExp -> getCurrentFileListLine();

        if (!currentFile.isEmpty())
        {
            params << "remove" << currentFile.mid(2);   //Jump over status marker characters (e.g "M ")

            runner -> startProc(getHgBinaryName(), workFolderPath, params);
            runningAction = ACT_REMOVE;
        }
    }
}

void MainWindow::hgAnnotate()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;
        QString currentFile = hgExp -> getCurrentFileListLine();

        if (!currentFile.isEmpty())
        {
            params << "annotate" << currentFile.mid(2);   //Jump over status marker characters (e.g "M ")

            runner -> startProc(getHgBinaryName(), workFolderPath, params);
            runningAction = ACT_ANNOTATE;
        }
    }
}


void MainWindow::hgResolveMark()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;
        QString currentFile = hgExp -> getCurrentFileListLine();

        if (!currentFile.isEmpty())
        {
            params << "resolve" << "--mark" << currentFile.mid(2);   //Jump over status marker characters (e.g "M ")

            runner -> startProc(getHgBinaryName(), workFolderPath, params);
            runningAction = ACT_RESOLVE_MARK;
        }
    }
}



void MainWindow::hgResolveList()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        params << "resolve" << "--list";
        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_RESOLVE_LIST;
    }
}



void MainWindow::hgAdd()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        params << "add";

        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_ADD;
    }
}

int MainWindow::getCommitComment(QString& comment)
{
    int ret;

    QDialog dlg(this);

    QLabel *commentLabel = new QLabel(tr("Comment:"));
    QLineEdit *commentEdit = new QLineEdit;
    commentEdit -> setFixedWidth(400);
    QHBoxLayout *commentLayout = new QHBoxLayout;
    commentLayout -> addWidget(commentLabel);
    commentLayout -> addWidget(commentEdit);

    QPushButton *btnOk = new QPushButton(tr("Ok"));
    QPushButton *btnCancel = new QPushButton(tr("Cancel"));
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout -> addWidget(btnOk);
    btnLayout -> addWidget(btnCancel);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout -> addLayout(commentLayout);
    mainLayout -> addLayout(btnLayout);

    dlg.setLayout(mainLayout);

    dlg.setWindowTitle(tr("Save (commit)"));

    connect(btnOk, SIGNAL(clicked()), &dlg, SLOT(accept()));
    connect(btnCancel, SIGNAL(clicked()), &dlg, SLOT(reject()));

    ret = dlg.exec();
    comment = commentEdit -> text();
    return ret;
}

void MainWindow::hgCommit()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;
        QString comment;
        
        if (QDialog::Accepted == getCommitComment(comment))
        {
            if (!comment.isEmpty())
            {
                params << "commit" << "--message" << comment << "--user" << userInfo;

                runner -> startProc(getHgBinaryName(), workFolderPath, params);
                runningAction = ACT_COMMIT;
            }
        }
    }
}

void MainWindow::hgFileDiff()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        QString currentFile = hgExp -> getCurrentFileListLine();

        if (!currentFile.isEmpty())
        {
            //Diff parent file against working folder file
            params << "kdiff3" << currentFile.mid(2);
            runner -> startProc(getHgBinaryName(), workFolderPath, params, false);
            runningAction = ACT_FILEDIFF;
        }
    }
}


void MainWindow::hgFolderDiff()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        //Diff parent against working folder (folder diff)
        params << "kdiff3";
        runner -> startProc(getHgBinaryName(), workFolderPath, params, false);
        runningAction = ACT_FOLDERDIFF;
    }
}


void MainWindow::hgChgSetDiff()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        //Diff 2 history log versions against each other
        QString revA;
        QString revB;

        hgExp -> getHistoryDiffRevisions(revA, revB);

        if ((!revA.isEmpty()) && (!revB.isEmpty()))
        {
            params << "kdiff3" << "--rev" << revA << "--rev" << revB;
            runner -> startProc(getHgBinaryName(), workFolderPath, params, false);
            runningAction = ACT_CHGSETDIFF;
        }
        else
        {
            QMessageBox::information(this, tr("Changeset diff"), tr("Please select two changesets from history list or heads list first."));
        }
    }
}



void MainWindow::hgUpdate()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;


        params << "update";


        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_UPDATE;
    }
}


void MainWindow::hgUpdateToRev()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;
        QString rev;

        hgExp -> getUpdateToRevRevision(rev);

        hgExp -> setCurrentIndex(WORKTAB);
        enableDisableActions();

        params << "update" << "--rev" << rev << "--clean";

        runner -> startProc(getHgBinaryName(), workFolderPath, params);

        runningAction = ACT_UPDATE;
    }
}


void MainWindow::hgRevert()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;
        QString currentFile = hgExp -> getCurrentFileListLine();

        params << "revert" << currentFile.mid(2);

        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_REVERT;
    }
}

void MainWindow::hgMerge()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        params << "merge";

        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_MERGE;
    }
}


void MainWindow::hgCloneFromRemote()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        params << "clone" << remoteRepoPath << workFolderPath;

        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_CLONEFROMREMOTE;
    }
}


void MainWindow::hgInit()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        params << "init";

        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_INIT;
    }
}


void MainWindow::hgIncoming()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        params << "incoming" << "--newest-first" << remoteRepoPath;

        runner -> startProc(getHgBinaryName(), workFolderPath, params, false);
        runningAction = ACT_INCOMING;
    }
}


void MainWindow::hgPull()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        params << "pull" << remoteRepoPath;

        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_PULL;
    }
}


void MainWindow::hgPush()
{
    if (runningAction == ACT_NONE)
    {
        QStringList params;

        params << "push" << remoteRepoPath;

        runner -> startProc(getHgBinaryName(), workFolderPath, params);
        runningAction = ACT_PUSH;
    }
}


void MainWindow::settings()
{
    SettingsDialog *settingsDlg = new SettingsDialog(this);
    settingsDlg->setModal(true);
    settingsDlg->exec();
    hgExp -> clearLists();
    enableDisableActions();
    hgStat();
}

void MainWindow::presentLongStdoutToUser(QString stdo, int w, int h)
{
    QDialog dlg;
    dlg.setMinimumWidth(w);
    dlg.setMinimumHeight(h);

    QVBoxLayout *box = new QVBoxLayout;
    QListWidget *list = new QListWidget;
    list-> addItems(stdo.split("\n"));
    QPushButton *btn = new QPushButton(tr("Ok"));
    connect(btn, SIGNAL(clicked()), &dlg, SLOT(accept()));

    box -> addWidget(list);
    box -> addWidget(btn);
    dlg.setLayout(box);

    dlg.exec();
}


bool MainWindow::isSelectedLocallyDeleted(QListWidget *workList)
{
    QList<QListWidgetItem *> selList = workList -> selectedItems();
    if (selList.count() == 1)
    {
        if (selList.at(0)->text().mid(0, 1) == "!")
        {
            return true;
        }
    }
    return false;
}


bool MainWindow::isSelectedUntracked(QListWidget *workList)
{
    QList<QListWidgetItem *> selList = workList -> selectedItems();
    if (selList.count() == 1)
    {
        if (selList.at(0)->text().mid(0, 1) == "?")
        {
            return true;
        }
    }
    return false;
}


bool MainWindow::isSelectedModified(QListWidget *workList)
{
    QList<QListWidgetItem *> selList = workList -> selectedItems();
    if (selList.count() == 1)
    {
        if (selList.at(0)->text().mid(0, 1) == "M")
        {
            return true;
        }
    }
    return false;
}

void MainWindow::countAMRModifications(QListWidget *workList, int& a, int& m, int& r)
{
    int itemCount = workList -> count();
    if (itemCount > 0)
    {
        int A= 0;
        int M=0;
        int R=0;
        for (int i = 0; i < workList -> count(); i++)
        {
            QListWidgetItem *currItem = workList -> item(i);

            QString tmp = currItem->text().mid(0, 1);
            if (tmp == "M")
            {
                M++;
            }
            else if (tmp == "R")
            {
                R++;
            }
            else if (tmp == "A")
            {
                A++;
            }
        }

        a = A;
        m = M;
        r = R;
    }
    else
    {
        a = 0;
        m = 0;
        r = 0;
    }
}


void MainWindow::timerEvent(QTimerEvent *)
{
    bool shouldHgStat = false;

    if (runningAction != ACT_NONE)
    {
        //We are running some hg command...
        if (runner -> isProcRunning() == false)
        {
            //Running has just ended.
            int exitCode = runner -> getExitCode();

            runner -> hideProgBar();

            //Clumsy...
            if ((EXITOK(exitCode)) || ((exitCode == 1) && (runningAction == ACT_INCOMING)))
            {
                //Successful running.
                switch(runningAction)
                {
                    case ACT_STAT:
                        {
                            hgExp -> updateWorkFolderFileList(runner -> getStdOut());
                        }
                        break;

                    case ACT_INCOMING:
                    case ACT_ANNOTATE:
                    case ACT_RESOLVE_LIST:
                    case ACT_RESOLVE_MARK:
                        presentLongStdoutToUser(runner -> getStdOut(), 1024, 768);
                        shouldHgStat = true;
                        break;

                    case ACT_PULL:
                        QMessageBox::information(this, "pull", runner -> getStdOut());
                        shouldHgStat = true;
                        break;

                    case ACT_PUSH:
                        QMessageBox::information(this, "push", runner -> getStdOut());
                        shouldHgStat = true;
                        break;

                    case ACT_INIT:
                        enableDisableActions();
                        shouldHgStat = true;
                        break;

                    case ACT_CLONEFROMREMOTE:
                        QMessageBox::information(this, "clone", runner -> getStdOut());
                        enableDisableActions();
                        shouldHgStat = true;
                        break;

                    case ACT_LOG:
                        {
                            hgExp -> updateLocalRepoHgLogList(runner -> getStdOut());
                        }
                        break;

                    case ACT_PARENTS:
                        {
                            hgExp -> updateLocalRepoParentsList(runner -> getStdOut());
                        }
                        break;

                    case ACT_HEADS:
                        {
                            QString stdOut = runner -> getStdOut();
                            hgExp -> updateLocalRepoHeadsList(stdOut);
                        }
                        break;

                    case ACT_REMOVE:
                    case ACT_ADD:
                    case ACT_COMMIT:
                    case ACT_FILEDIFF:
                    case ACT_FOLDERDIFF:
                    case ACT_CHGSETDIFF:
                    case ACT_REVERT:
                        shouldHgStat = true;
                        break;

                    case ACT_UPDATE:
                        QMessageBox::information(this, "update", runner -> getStdOut());
                        shouldHgStat = true;
                        break;

                    case ACT_MERGE:
                        QMessageBox::information(this, "merge", runner -> getStdOut());
                        shouldHgStat = true;
                        break;

                    default:
                        break;
                }
            }

            //Typical sequence goes stat -> heads -> parents -> log
            if (runningAction == ACT_STAT)
            {
                runningAction = ACT_NONE;
                hgHeads();
            }
            else if (runningAction == ACT_HEADS)
            {
                runningAction = ACT_NONE;
                hgParents();
            }
            else if (runningAction == ACT_PARENTS)
            {
                runningAction = ACT_NONE;
                hgLog();
            }
            else
            {
                runningAction = ACT_NONE;
                if (shouldHgStat)
                {
                    hgStat();
                }
            }
        }
    }
    else
    {
        enableDisableActions();
    }
}

void MainWindow::connectActions()
{
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    connect(hgStatAct, SIGNAL(triggered()), this, SLOT(hgStat()));
    connect(hgExp, SIGNAL(workFolderViewTypesChanged()), this, SLOT(hgStat()));
    connect(hgRemoveAct, SIGNAL(triggered()), this, SLOT(hgRemove()));
    connect(hgAddAct, SIGNAL(triggered()), this, SLOT(hgAdd()));
    connect(hgCommitAct, SIGNAL(triggered()), this, SLOT(hgCommit()));
    connect(hgFileDiffAct, SIGNAL(triggered()), this, SLOT(hgFileDiff()));
    connect(hgFolderDiffAct, SIGNAL(triggered()), this, SLOT(hgFolderDiff()));
    connect(hgChgSetDiffAct, SIGNAL(triggered()), this, SLOT(hgChgSetDiff()));
    connect(hgUpdateAct, SIGNAL(triggered()), this, SLOT(hgUpdate()));
    connect(hgRevertAct, SIGNAL(triggered()), this, SLOT(hgRevert()));
    connect(hgMergeAct, SIGNAL(triggered()), this, SLOT(hgMerge()));

    connect(settingsAct, SIGNAL(triggered()), this, SLOT(settings()));

    connect(hgInitAct, SIGNAL(triggered()), this, SLOT(hgInit()));
    connect(hgCloneFromRemoteAct, SIGNAL(triggered()), this, SLOT(hgCloneFromRemote()));
    connect(hgIncomingAct, SIGNAL(triggered()), this, SLOT(hgIncoming()));
    connect(hgPullAct, SIGNAL(triggered()), this, SLOT(hgPull()));
    connect(hgPushAct, SIGNAL(triggered()), this, SLOT(hgPush()));

    connect(hgExp, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    connect(hgUpdateToRevAct, SIGNAL(triggered()), this, SLOT(hgUpdateToRev()));
    connect(hgAnnotateAct, SIGNAL(triggered()), this, SLOT(hgAnnotate()));
    connect(hgResolveListAct, SIGNAL(triggered()), this, SLOT(hgResolveList()));
    connect(hgResolveMarkAct, SIGNAL(triggered()), this, SLOT(hgResolveMark()));
}

void MainWindow::tabChanged(int currTab)
{
    tabPage = currTab;
}

void MainWindow::enableDisableActions()
{
    QDir localRepoDir;
    QDir workFolderDir;
    bool workFolderExist;
    bool localRepoExist;

    remoteRepoActionsEnabled = true;
    if (remoteRepoPath.isEmpty())
    {
        remoteRepoActionsEnabled = false;
    }

    localRepoActionsEnabled = true;
    if (workFolderPath.isEmpty())
    {
        localRepoActionsEnabled = false;
        workFolderExist = false;
    }

    if (!workFolderDir.exists(workFolderPath))
    {
        localRepoActionsEnabled = false;
        workFolderExist = false;
    }
    else
    {
        workFolderExist = true;
    }

    if (!localRepoDir.exists(workFolderPath + getHgDirName()))
    {
        localRepoActionsEnabled = false;
        localRepoExist = false;
    }

    hgCloneFromRemoteAct -> setEnabled(remoteRepoActionsEnabled);
    hgIncomingAct -> setEnabled(remoteRepoActionsEnabled && remoteRepoActionsEnabled);
    hgPullAct -> setEnabled(remoteRepoActionsEnabled && remoteRepoActionsEnabled);
    hgPushAct -> setEnabled(remoteRepoActionsEnabled && remoteRepoActionsEnabled);

    if (tabPage != WORKTAB)
    {
        localRepoActionsEnabled = false;
    }

    hgInitAct -> setEnabled((localRepoExist == false) && (workFolderExist==true));
    hgStatAct -> setEnabled(localRepoActionsEnabled);
    hgFileDiffAct -> setEnabled(localRepoActionsEnabled);
    hgFolderDiffAct -> setEnabled(localRepoActionsEnabled);
    hgRevertAct -> setEnabled(localRepoActionsEnabled);
    hgAddAct -> setEnabled(localRepoActionsEnabled);
    hgRemoveAct -> setEnabled(localRepoActionsEnabled);
    hgUpdateAct -> setEnabled(localRepoActionsEnabled);
    hgCommitAct -> setEnabled(localRepoActionsEnabled);
    hgMergeAct -> setEnabled(localRepoActionsEnabled);

    hgExp -> enableDisableOtherTabs();

    int a, m, r;
    countAMRModifications(hgExp -> workFolderFileList, a, m, r);

    if (tabPage == WORKTAB)
    {
        //Enable / disable actions according to workFolderFileList selections / currentSelection / count
        hgChgSetDiffAct -> setEnabled(false);
        hgUpdateToRevAct -> setEnabled(false);

        if (localRepoActionsEnabled)
        {
            if ((a == 0) && (m == 0) && (r == 0))
            {
                hgCommitAct -> setEnabled(false);
                hgRevertAct -> setEnabled(false);
            }

            if (m == 0)
            {
                hgFolderDiffAct -> setEnabled(false);
            }

            if (!isSelectedModified(hgExp -> workFolderFileList))
            {
                hgFileDiffAct -> setEnabled(false);
                hgRevertAct -> setEnabled(false);
            }

            if (!isSelectedUntracked(hgExp -> workFolderFileList))
            {
                hgAddAct -> setEnabled(false);
            }

            if (!isSelectedLocallyDeleted(hgExp -> workFolderFileList))
            {
                hgRemoveAct -> setEnabled(false);
            }

            if (hgExp -> localRepoHeadsList->count() == 1)
            {
                hgMergeAct -> setEnabled(false);
            }

        }
    }
    else
    {
        QList <QListWidgetItem *> headSelList = hgExp -> localRepoHeadsList->selectedItems();
        QList <QListWidgetItem *> historySelList = hgExp -> localRepoHgLogList->selectedItems();

        if ((historySelList.count() == 2) || (headSelList.count() == 2))
        {
            hgChgSetDiffAct -> setEnabled(true);
        }
        else
        {
            hgChgSetDiffAct -> setEnabled(false);
        }

        if ((a == 0) && (m == 0) && (r == 0))
        {
            if (historySelList.count() == 1)
            {
                hgUpdateToRevAct -> setEnabled(true);
            }
            else
            {
                hgUpdateToRevAct -> setEnabled(false);
            }
        }
        else
        {
            hgUpdateToRevAct -> setEnabled(false);
        }
    }
}

void MainWindow::createActions()
{
    //File actions
    hgInitAct = new QAction(tr("Init local repository"), this);
    hgInitAct->setStatusTip(tr("Create an empty local repository in selected folder"));

    hgCloneFromRemoteAct = new QAction(tr("Clone from remote"), this);
    hgCloneFromRemoteAct->setStatusTip(tr("Clone from remote repository into local repository in selected folder"));

    settingsAct = new QAction(QIcon(":/images/settings.png"), tr("Settings..."), this);
    settingsAct -> setStatusTip(tr("View and change application settings"));
    settingsAct -> setIconVisibleInMenu(true);

    exitAct = new QAction(QIcon(":/images/exit.png"), tr("Exit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit application"));
    exitAct -> setIconVisibleInMenu(true);

    //Repository actions
    hgIncomingAct = new QAction(QIcon(":/images/incoming.png"), tr("View incoming changesets"), this);
    hgIncomingAct -> setStatusTip(tr("View info of changesets incoming to us from remote repository (on pull operation)"));

    hgPullAct = new QAction(QIcon(":/images/pull.png"), tr("Pull from remote"), this);
    hgPullAct -> setStatusTip(tr("Pull changesets from remote repository to local repository"));

    hgPushAct = new QAction(QIcon(":/images/push.png"), tr("Push to remote"), this);
    hgPushAct->setStatusTip(tr("Push local changesets to remote repository"));

    //Workfolder actions
    hgStatAct = new QAction(QIcon(":/images/status.png"), tr("Refresh status"), this);
    hgStatAct->setStatusTip(tr("Refresh (info of) status of workfolder files (A=Added, M=Mofified, R=Removed, C=Clean, !=Locally deleted, ?=Not tracked, I=Ignored)"));

    hgFileDiffAct   = new QAction(QIcon(":/images/diff.png"), tr("View filediff"), this);
    hgFileDiffAct->setStatusTip(tr("Filediff: View differences between selected working folder file and local repository file"));

    hgFolderDiffAct   = new QAction(QIcon(":/images/folderdiff.png"), tr("View folderdiff"), this);
    hgFolderDiffAct->setStatusTip(tr("Folderdiff: View all differences between working folder files and local repository files"));

    hgChgSetDiffAct   = new QAction(QIcon(":/images/chgsetdiff.png"), tr("View changesetdiff"), this);
    hgChgSetDiffAct->setStatusTip(tr("Change set diff: View differences between all files of 2 repository changesets"));

    hgRevertAct = new QAction(QIcon(":/images/undo.png"), tr("Undo changes"), this);
    hgRevertAct->setStatusTip(tr("Undo selected working folder file changes (return to local repository version)"));

    hgAddAct = new QAction(QIcon(":/images/add.png"), tr("Add files"), this);
    hgAddAct -> setStatusTip(tr("Add working folder files to local repository (on next commit)"));

    hgRemoveAct = new QAction(QIcon(":/images/remove.png"), tr("Remove file"), this);
    hgRemoveAct -> setStatusTip(tr("Remove selected working folder file from local repository (on next commit)"));

    hgUpdateAct = new QAction(QIcon(":/images/update.png"), tr("Update working folder"), this);
    hgUpdateAct->setStatusTip(tr("Update working folder from local repository"));

    hgCommitAct = new QAction(QIcon(":/images/commit.png"), tr("Commit / Save changes"), this);
    hgCommitAct->setStatusTip(tr("Save all changes in working folder (and all subfolders) to local repository"));

    hgMergeAct = new QAction(QIcon(":/images/merge.png"), tr("Merge"), this);
    hgMergeAct->setStatusTip(tr("Merge two local repository changesets to working folder"));

    //Advanced actions
    hgUpdateToRevAct = new QAction(tr("Update to revision"), this);
    hgUpdateToRevAct -> setStatusTip(tr("Update working folder to version selected from history list"));

    hgAnnotateAct = new QAction(tr("Annotate"), this);
    hgAnnotateAct -> setStatusTip(tr("Show line-by-line version information for selected file"));

    hgResolveListAct = new QAction(tr("Resolve (list)"), this);
    hgResolveListAct -> setStatusTip(tr("Resolve (list): Show list of files needing merge"));

    hgResolveMarkAct = new QAction(tr("Resolve (mark)"), this);
    hgResolveMarkAct -> setStatusTip(tr("Resolve (mark): Mark selected file status as resolved"));

    //Help actions
    aboutAct = new QAction(tr("About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));

    aboutQtAct = new QAction(tr("About Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu -> addAction(hgInitAct);
    fileMenu -> addAction(hgCloneFromRemoteAct);
    fileMenu -> addSeparator();
    fileMenu -> addAction(settingsAct);
    fileMenu -> addSeparator();
    fileMenu -> addAction(exitAct);

    advancedMenu = menuBar()->addMenu(tr("Advanced"));
    advancedMenu -> addAction(hgUpdateToRevAct);
    advancedMenu -> addSeparator();
    advancedMenu -> addAction(hgAnnotateAct);
    advancedMenu -> addSeparator();
    advancedMenu -> addAction(hgResolveListAct);
    advancedMenu -> addAction(hgResolveMarkAct);

    helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    fileToolBar -> setIconSize(QSize(MY_ICON_SIZE, MY_ICON_SIZE));
    fileToolBar -> addAction(settingsAct);
    fileToolBar -> addAction(exitAct);
    fileToolBar -> addSeparator();
    fileToolBar -> addAction(hgChgSetDiffAct);
    fileToolBar -> setMovable(false);

    repoToolBar = addToolBar(tr(REPOMENU_TITLE));
    repoToolBar -> setIconSize(QSize(MY_ICON_SIZE, MY_ICON_SIZE));
    repoToolBar->addAction(hgIncomingAct);
    repoToolBar->addAction(hgPullAct);
    repoToolBar->addAction(hgPushAct);
    repoToolBar -> setMovable(false);

    workFolderToolBar = addToolBar(tr(WORKFOLDERMENU_TITLE));
    addToolBar(Qt::LeftToolBarArea, workFolderToolBar);
    workFolderToolBar -> setIconSize(QSize(MY_ICON_SIZE, MY_ICON_SIZE));
    workFolderToolBar->addAction(hgStatAct);
    workFolderToolBar->addSeparator();
    workFolderToolBar->addAction(hgFileDiffAct);
    workFolderToolBar->addAction(hgFolderDiffAct);
    workFolderToolBar->addSeparator();
    workFolderToolBar->addAction(hgRevertAct);
    workFolderToolBar->addAction(hgUpdateAct);
    workFolderToolBar->addAction(hgCommitAct);
    workFolderToolBar->addAction(hgMergeAct);
    workFolderToolBar->addSeparator();
    workFolderToolBar->addAction(hgAddAct);
    workFolderToolBar->addAction(hgRemoveAct);
    workFolderToolBar -> setMovable(false);

}


void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QDir workFolder;

    QSettings settings("hgexplorer", "hgexplorer");

    remoteRepoPath = settings.value("remoterepopath", "").toString() ;
    workFolderPath = settings.value("workfolderpath", "").toString();
    if (!workFolder.exists(workFolderPath))
    {
        workFolderPath = "";
    }

    userInfo = settings.value("userinfo", "").toString();

    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    firstStart = settings.value("firststart", QVariant(true)).toBool();

    initialFileTypesBits = (unsigned char) settings.value("viewFileTypes", QVariant(DEFAULT_HG_STAT_BITS)).toInt();
    resize(size);
    move(pos);
}

void MainWindow::writeSettings()
{
    QSettings settings("hgexplorer", "hgexplorer");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("remoterepopath", remoteRepoPath);
    settings.setValue("workfolderpath", workFolderPath);
    settings.setValue("userinfo", userInfo);
    settings.setValue("firststart", firstStart);
    settings.setValue("viewFileTypes", hgExp -> getFileTypesBits());
}




