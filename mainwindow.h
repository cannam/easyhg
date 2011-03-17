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

class WorkStatusWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QString myDirPath);

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
    void recentMenuActivated();
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
    void hgShowSummaryFor(Changeset *);
    void hgFolderDiff();
    void hgDiffToCurrent(QString);
    void hgDiffToParent(QString, QString);
    void hgUpdate();
    void hgRevert();
    void hgMerge();
    void hgRedoMerge();
    void hgCloneFromRemote();
    void hgInit();
    void hgIncoming();
    void hgPush();
    void hgPull();
    void hgUpdateToRev(QString);
    void hgMergeFrom(QString);
    void hgResolveList();
    void hgTag(QString);
    void hgNewBranch();
    void hgNoBranch();
    void hgServe();
    void hgIgnore();

    void hgAnnotateFiles(QStringList);
    void hgDiffFiles(QStringList);
    void hgCommitFiles(QStringList);
    void hgRevertFiles(QStringList);
    void hgRenameFiles(QStringList);
    void hgCopyFiles(QStringList);
    void hgAddFiles(QStringList);
    void hgRemoveFiles(QStringList);
    void hgRedoFileMerges(QStringList);
    void hgMarkFilesResolved(QStringList);
    void hgIgnoreFiles(QStringList);
    void hgUnIgnoreFiles(QStringList);

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

    void updateRecentMenu();
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
    void reportAuthFailed(QString);
    void writeSettings();

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
    QString format1(QString);
    QString format3(QString, QString, QString);

    void clearState();

    void updateFileSystemWatcher();
    void suspendFileSystemWatcher();
    void restoreFileSystemWatcher();

    void updateWorkFolderAndRepoNames();

    WorkStatusWidget *m_workStatus;
    HgTabWidget *m_hgTabs;

    QString m_remoteRepoPath;
    QString m_workFolderPath;
    QString m_currentBranch;
    Changesets m_currentHeads;
    Changesets m_currentParents;
    int m_commitsSincePush;
    bool m_stateUnknown;
    bool m_hgIsOK;
    bool m_needNewLog;

    bool m_firstStart;

    bool m_showAllFiles;

    //Actions enabled flags
    bool m_remoteRepoActionsEnabled;
    bool m_localRepoActionsEnabled;

    QString m_myDirPath;

    // File menu actions
    QAction *m_openAct;
    QAction *m_changeRemoteRepoAct;
    QAction *m_settingsAct;
    QAction *m_exitAct;

    // Repo actions
    QAction *m_hgIncomingAct;
    QAction *m_hgPushAct;
    QAction *m_hgPullAct;
    QAction *m_hgRefreshAct;
    QAction *m_hgFolderDiffAct;
    QAction *m_hgChgSetDiffAct;
    QAction *m_hgRevertAct;
    QAction *m_hgAddAct;
    QAction *m_hgRemoveAct;
    QAction *m_hgUpdateAct;
    QAction *m_hgCommitAct;
    QAction *m_hgMergeAct;
    QAction *m_hgUpdateToRevAct;
    QAction *m_hgAnnotateAct;
    QAction *m_hgIgnoreAct;
    QAction *m_hgServeAct;

    // Menus
    QMenu *m_fileMenu;
    QMenu *m_recentMenu;
    QMenu *m_advancedMenu;
    QMenu *m_helpMenu;

    // Help menu actions
    QAction *m_aboutAct;

    QToolBar *m_fileToolBar;
    QToolBar *m_repoToolBar;
    QToolBar *m_workFolderToolBar;

    HgRunner *m_runner;

    bool m_shouldHgStat;

    QString getDiffBinaryName();
    QString getMergeBinaryName();
    QString getEditorBinaryName();

    QFileSystemWatcher *m_fsWatcher;
    QTimer *m_fsWatcherGeneralTimer;
    QTimer *m_fsWatcherRestoreTimer;
    bool m_fsWatcherSuspended;

    QString m_lastStatOutput;
    QStringList m_lastRevertedFiles;

    bool m_justMerged;
    QString m_mergeTargetRevision;
    QString m_mergeCommitComment;
};

#endif
