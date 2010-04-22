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
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "hgexpwidget.h"
#include "hgrunner.h"
#include "common.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

enum HGACTIONS
{
    ACT_NONE,
    ACT_STAT,
    ACT_HEADS,
    ACT_PARENTS,
    ACT_LOG,
    ACT_REMOVE,
    ACT_ADD,
    ACT_INCOMING,
    ACT_PUSH,
    ACT_PULL,
    ACT_CLONEFROMREMOTE,
    ACT_INIT,
    ACT_COMMIT,
    ACT_ANNOTATE,
    ACT_FILEDIFF,
    ACT_FOLDERDIFF,
    ACT_CHGSETDIFF,
    ACT_UPDATE,
    ACT_REVERT,
    ACT_MERGE,
    ACT_RESOLVE_LIST,
    ACT_RESOLVE_MARK
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    HgExpWidget *hgExp;
    void writeSettings();
    void enableDisableActions();

    //Paths to remote repo & workfolder
    //Local repo is directory "./hg/" under work folder
    QString remoteRepoPath;
    QString workFolderPath;

    //User info for commits
    QString userInfo;
    bool        firstStart;

protected:
    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent *event);

public slots:
    void hgStat();
    void tabChanged(int currTab);

private slots:
    void about();
    void hgRemove();
    void hgAdd();
    void hgCommit();
    void hgFileDiff();
    void hgFolderDiff();
    void hgChgSetDiff();
    void hgUpdate();
    void hgRevert();
    void hgMerge();
    void settings();
    void hgCloneFromRemote();
    void hgInit();
    void hgIncoming();
    void hgPush();
    void hgPull();
    void hgUpdateToRev();
    void hgAnnotate();
    void hgResolveList();
    void hgResolveMark();

private:
    void hgHeads();
    void hgParents();
    void hgLog();
    void createActions();
    void connectActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void splitChangeSets(QStringList *list, QString hgLogOutput);
    int getCommitComment(QString& comment);
    void presentLongStdoutToUser(QString stdo, int w, int h);
    void countAMRModifications(QListWidget *workList, int& a, int& m, int& r);
    bool isSelectedModified(QListWidget *workList);
    bool isSelectedUntracked(QListWidget *workList);
    bool isSelectedLocallyDeleted(QListWidget *workList);


    //Actions enabled flags
    bool remoteRepoActionsEnabled;
    bool localRepoActionsEnabled;

    //File menu actions
    QAction *hgInitAct;
    QAction *hgCloneFromRemoteAct;
    QAction *settingsAct;
    QAction *exitAct;

    QAction *hgIncomingAct;
    QAction *hgPushAct;
    QAction *hgPullAct;
    QAction *hgStatAct;
    QAction *hgFileDiffAct;
    QAction *hgFolderDiffAct;
    QAction *hgChgSetDiffAct;
    QAction *hgRevertAct;
    QAction *hgAddAct;
    QAction *hgRemoveAct;
    QAction *hgUpdateAct;
    QAction *hgCommitAct;
    QAction *hgMergeAct;
    QAction *hgUpdateToRevAct;
    QAction *hgAnnotateAct;
    QAction *hgResolveListAct;
    QAction *hgResolveMarkAct;

    //Menus
    QMenu   *fileMenu;
    QMenu   *advancedMenu;
    QMenu   *helpMenu;

    //Help menu actions
    QAction *aboutAct;
    QAction *aboutQtAct;

    QToolBar *fileToolBar;
    QToolBar *repoToolBar;
    QToolBar *workFolderToolBar;

    int         timerId;
    HGACTIONS   runningAction;
    HgRunner    *runner;

    int         tabPage;
    unsigned char   initialFileTypesBits;
};

#endif
