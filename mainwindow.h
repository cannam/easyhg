/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "hgtabwidget.h"
#include "hgrunner.h"
#include "common.h"
#include "changeset.h"
#include "hgaction.h"

#include <QMainWindow>
#include <QListWidget>
#include <QFileSystemWatcher>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QTimer;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString myDirPath);
    HgTabWidget *hgTabs;
    void writeSettings();

    //Paths to remote repo & workfolder
    //Local repo is directory "./hg/" under work folder
    QString remoteRepoPath;
    QString workFolderPath;
    QString currentBranch;
    Changesets currentHeads;
    Changesets currentParents;
    int commitsSincePush;
    bool stateUnknown;
    bool hgIsOK;
    bool needNewLog;

protected:
    void closeEvent(QCloseEvent *event);

public slots:
    void open(QString local);
    void hgRefresh();
    void commandStarting(HgAction);
    void commandCompleted(HgAction action, QString stdOut);
    void commandFailed(HgAction action, QString stdErr);
    void enableDisableActions();

private slots:
    void about();
    void settings();
    void open();
    void changeRemoteRepo();
    void startupDialog();
    void clearSelections();
    void showAllChanged(bool);

    void hgTest();
    void hgTestExtension();
    void hgQueryPaths();
    void hgStat();
    void hgRemove();
    void hgAdd();
    void hgCommit();
    void hgShowSummary();
    void hgFolderDiff();
    void hgDiffToCurrent(QString);
    void hgDiffToParent(QString, QString);
    void hgUpdate();
    void hgRevert();
    void hgMerge();
    void hgMarkResolved(QStringList);
    void hgRetryMerge();
    void hgCloneFromRemote();
    void hgInit();
    void hgIncoming();
    void hgPush();
    void hgPull();
    void hgUpdateToRev(QString);
    void hgMergeFrom(QString);
    void hgAnnotate();
    void hgResolveList();
    void hgTag(QString);
    void hgNewBranch(QString);
    void hgServe();
    void hgIgnore();

    void fsDirectoryChanged(QString);
    void fsFileChanged(QString);
    void checkFilesystem();
    void actuallyRestoreFileSystemWatcher();

private:
    void hgQueryBranch();
    void hgQueryHeads();
    void hgQueryParents();
    void hgLog();
    void hgLogIncremental(QStringList prune);
    void createActions();
    void connectActions();
    void connectTabsSignals();
    void createMenus();
    void createToolBars();
    void updateToolBarStyle();
    void createStatusBar();
    void readSettings();
    void splitChangeSets(QStringList *list, QString hgLogOutput);
    void reportNewRemoteHeads(QString);
    void presentLongStdoutToUser(QString stdo);

    QStringList listAllUpIpV4Addresses();
    QString filterTag(QString tag);

    QString getUserInfo() const;

    bool openLocal(QString);
    bool openRemote(QString, QString);
    bool openInit(QString);

    bool complainAboutFilePath(QString);
    bool complainAboutUnknownFolder(QString);
    bool complainAboutInitInRepo(QString);
    bool complainAboutInitFile(QString);
    bool complainAboutCloneToExisting(QString);
    bool complainAboutCloneToFile(QString);
    QString complainAboutCloneToExistingFolder(QString local, QString remote); // returns new location, or empty string for cancel

    bool askAboutUnknownFolder(QString);
    bool askToInitExisting(QString);
    bool askToInitNew(QString);
    bool askToOpenParentRepo(QString, QString);
    bool askToOpenInsteadOfInit(QString);

    void showIncoming(QString);
    void showPullResult(QString);
    void showPushResult(QString);
    int extractChangeCount(QString);
    QString format3(QString, QString, QString);

    void clearState();

    void updateFileSystemWatcher();
    void suspendFileSystemWatcher();
    void restoreFileSystemWatcher();

    bool firstStart;

    bool showAllFiles;

    //Actions enabled flags
    bool remoteRepoActionsEnabled;
    bool localRepoActionsEnabled;

    QString m_myDirPath;

    //File menu actions
    QAction *openAct;
    QAction *changeRemoteRepoAct;
    QAction *settingsAct;
    QAction *exitAct;

    //Repo actions
    QAction *hgIncomingAct;
    QAction *hgPushAct;
    QAction *hgPullAct;
    QAction *hgRefreshAct;
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
    QAction *hgIgnoreAct;
    QAction *hgServeAct;

    //Menus
    QMenu   *fileMenu;
    QMenu   *advancedMenu;
    QMenu   *helpMenu;

    //Help menu actions
    QAction *aboutAct;

    QToolBar *fileToolBar;
    QToolBar *repoToolBar;
    QToolBar *workFolderToolBar;

    HgRunner *runner;

    bool shouldHgStat;

    QString getDiffBinaryName();
    QString getMergeBinaryName();
    QString getEditorBinaryName();

    QFileSystemWatcher *fsWatcher;
    QTimer *m_fsWatcherGeneralTimer;
    QTimer *m_fsWatcherRestoreTimer;
    bool m_fsWatcherSuspended;

    QString lastStatOutput;
    QStringList lastRevertedFiles;

    bool justMerged;
    QString mergeTargetRevision;
    QString mergeCommitComment;
};

#endif
