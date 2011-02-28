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

#include <QStringList>
#include <QDir>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QHostInfo>
#include <QDesktopServices>
#include <QStatusBar>
#include <QMessageBox>
#include <QMenuBar>
#include <QApplication>
#include <QToolBar>
#include <QToolButton>
#include <QSettings>
#include <QInputDialog>
#include <QRegExp>
#include <QShortcut>
#include <QUrl>
#include <QTimer>

#include "mainwindow.h"
#include "multichoicedialog.h"
#include "startupdialog.h"
#include "colourset.h"
#include "debug.h"
#include "logparser.h"
#include "confirmcommentdialog.h"
#include "incomingdialog.h"
#include "settingsdialog.h"
#include "moreinformationdialog.h"
#include "version.h"
#include "workstatuswidget.h"


MainWindow::MainWindow(QString myDirPath) :
    m_myDirPath(myDirPath),
    m_fsWatcherGeneralTimer(0),
    m_fsWatcherRestoreTimer(0),
    m_fsWatcherSuspended(false)
{
    setWindowIcon(QIcon(":images/easyhg-icon.png"));

    QString wndTitle;

    m_showAllFiles = false;

    m_fsWatcher = 0;
    m_commitsSincePush = 0;
    m_shouldHgStat = true;

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    m_runner = new HgRunner(m_myDirPath, this);
    connect(m_runner, SIGNAL(commandStarting(HgAction)),
            this, SLOT(commandStarting(HgAction)));
    connect(m_runner, SIGNAL(commandCompleted(HgAction, QString)),
            this, SLOT(commandCompleted(HgAction, QString)));
    connect(m_runner, SIGNAL(commandFailed(HgAction, QString)),
            this, SLOT(commandFailed(HgAction, QString)));
    statusBar()->addPermanentWidget(m_runner);

    setWindowTitle(tr("EasyMercurial"));

    m_remoteRepoPath = "";
    m_workFolderPath = "";

    readSettings();

    m_justMerged = false;

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QGridLayout *cl = new QGridLayout(central);
    int row = 0;

#ifndef Q_OS_MAC
    cl->setMargin(0);
#endif

    m_workStatus = new WorkStatusWidget(this);
    cl->addWidget(m_workStatus, row++, 0);

    m_hgTabs = new HgTabWidget(central, m_workFolderPath);
    connectTabsSignals();

    cl->addWidget(m_hgTabs, row++, 0);

    connect(m_hgTabs, SIGNAL(selectionChanged()),
            this, SLOT(enableDisableActions()));
    connect(m_hgTabs, SIGNAL(showAllChanged(bool)),
            this, SLOT(showAllChanged(bool)));

    setUnifiedTitleAndToolBarOnMac(true);
    connectActions();
    clearState();
    enableDisableActions();

    if (m_firstStart) {
        startupDialog();
    }

    SettingsDialog::findDefaultLocations(m_myDirPath);

    ColourSet *cs = ColourSet::instance();
    cs->clearDefaultNames();
    cs->addDefaultName("");
    cs->addDefaultName("default");
    cs->addDefaultName(getUserInfo());

    hgTest();
}


void MainWindow::closeEvent(QCloseEvent *)
{
    writeSettings();
    delete m_fsWatcher;
}


QString MainWindow::getUserInfo() const
{
    QSettings settings;
    settings.beginGroup("User Information");
    QString name = settings.value("name", getUserRealName()).toString();
    QString email = settings.value("email", "").toString();

    QString identifier;

    if (email != "") {
	identifier = QString("%1 <%2>").arg(name).arg(email);
    } else {
	identifier = name;
    }

    return identifier;
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About EasyMercurial"),
                      tr("<qt><h2>EasyMercurial v%1</h2>"
#ifdef Q_OS_MAC
                         "<font size=-1>"
#endif
                         "<p>EasyMercurial is a simple user interface for the "
                         "Mercurial</a> version control system.</p>"
                         "<h4>Credits and Copyright</h4>"
                         "<p>Development carried out by Chris Cannam for "
                         "SoundSoftware.ac.uk at the Centre for Digital Music, "
                         "Queen Mary, University of London.</p>"
                         "<p>EasyMercurial is based on HgExplorer by "
                         "Jari Korhonen, with thanks.</p>"
                         "<p style=\"margin-left: 2em;\">"
                         "Copyright &copy; 2011 Queen Mary, University of London.<br>"
                         "Copyright &copy; 2010 Jari Korhonen.<br>"
                         "Copyright &copy; 2011 Chris Cannam."
                         "</p>"
                         "<p style=\"margin-left: 2em;\">"
                         "This program requires Mercurial, by Matt Mackall and others.<br>"
                         "This program uses Qt by Nokia.<br>"
                         "This program uses Nuvola icons by David Vignoni.<br>"
                         "This program may use KDiff3 by Joachim Eibl.<br>"
                         "This program may use PyQt by River Bank Computing.<br>"
                         "Packaging for Mercurial and other dependencies on Windows is derived from TortoiseHg by Steve Borho and others."
                         "</p>"
                         "<h4>License</h4>"
                         "<p>This program is free software; you can redistribute it and/or "
                         "modify it under the terms of the GNU General Public License as "
                         "published by the Free Software Foundation; either version 2 of the "
                         "License, or (at your option) any later version.  See the file "
                         "COPYING included with this distribution for more information.</p>"
#ifdef Q_OS_MAC
                         "</font>"
#endif
                          ).arg(EASYHG_VERSION));
}

void MainWindow::clearSelections()
{
    m_hgTabs->clearSelections();
}

void MainWindow::showAllChanged(bool s)
{
    m_showAllFiles = s;
    hgQueryPaths();
}

void MainWindow::hgRefresh()
{
    clearState();
    hgQueryPaths();
}

void MainWindow::hgTest()
{
    QStringList params;
    //!!! should we test version output? Really we want at least 1.7.x
    //!!! for options such as merge --tool
    params << "--version";
    m_runner->requestAction(HgAction(ACT_TEST_HG, m_myDirPath, params));
}

void MainWindow::hgTestExtension()
{
    QStringList params;
    params << "--version";
    m_runner->requestAction(HgAction(ACT_TEST_HG_EXT, m_myDirPath, params));
}

void MainWindow::hgStat()
{
    QStringList params;

    if (m_showAllFiles) {
        params << "stat" << "-A";
    } else {
        params << "stat" << "-ardum";
    }

    m_lastStatOutput = "";

    m_runner->requestAction(HgAction(ACT_STAT, m_workFolderPath, params));
}

void MainWindow::hgQueryPaths()
{
    // Quickest is to just read the file

    QFileInfo hgrc(m_workFolderPath + "/.hg/hgrc");

    QString path;

    if (hgrc.exists()) {
        QSettings s(hgrc.canonicalFilePath(), QSettings::IniFormat);
        s.beginGroup("paths");
        path = s.value("default").toString();
    }

    m_remoteRepoPath = path;

    // We have to do this here, because commandCompleted won't be called
    MultiChoiceDialog::addRecentArgument("local", m_workFolderPath);
    MultiChoiceDialog::addRecentArgument("remote", m_remoteRepoPath);
    updateWorkFolderAndRepoNames();
    
    hgQueryBranch();
    return;

/* The classic method!

    QStringList params;
    params << "paths";
    m_runner->requestAction(HgAction(ACT_QUERY_PATHS, m_workFolderPath, params));
*/
}

void MainWindow::hgQueryBranch()
{
    // Quickest is to just read the file

    QFile hgbr(m_workFolderPath + "/.hg/branch");

    QString br = "default";

    if (hgbr.exists() && hgbr.open(QFile::ReadOnly)) {
        QByteArray ba = hgbr.readLine();
        br = QString::fromUtf8(ba).trimmed();
    }
    
    m_currentBranch = br;
    
    // We have to do this here, because commandCompleted won't be called
    hgStat();
    return;

/* The classic method!

    QStringList params;
    params << "branch";
    m_runner->requestAction(HgAction(ACT_QUERY_BRANCH, m_workFolderPath, params));
*/
}

void MainWindow::hgQueryHeads()
{
    QStringList params;
    // On empty repos, "hg heads" will fail -- we don't care about
    // that.  Use --closed option so as to include closed branches;
    // otherwise we'll be stuck if the user updates into one, and our
    // incremental log will end up with spurious stuff in it because
    // we won't be pruning at the ends of closed branches
    params << "heads" << "--closed";
    m_runner->requestAction(HgAction(ACT_QUERY_HEADS, m_workFolderPath, params));
}

void MainWindow::hgLog()
{
    QStringList params;
    params << "log";
    params << "--template";
    params << Changeset::getLogTemplate();
    
    m_runner->requestAction(HgAction(ACT_LOG, m_workFolderPath, params));
}

void MainWindow::hgLogIncremental(QStringList prune)
{
    // Sometimes we can be called with prune empty -- it represents
    // the current heads, but if we have none already and for some
    // reason are being prompted for an incremental update, we may run
    // into trouble.  In that case, make this a full log instead

    if (prune.empty()) {
        hgLog();
        return;
    }

    QStringList params;
    params << "log";

    foreach (QString p, prune) {
        params << "--prune" << Changeset::hashOf(p);
    }
        
    params << "--template";
    params << Changeset::getLogTemplate();
    
    m_runner->requestAction(HgAction(ACT_LOG_INCREMENTAL, m_workFolderPath, params));
}

void MainWindow::hgQueryParents()
{
    QStringList params;
    params << "parents";
    m_runner->requestAction(HgAction(ACT_QUERY_PARENTS, m_workFolderPath, params));
}

void MainWindow::hgAnnotate()
{
    QStringList params;
    QString currentFile;//!!! = m_hgTabs -> getCurrentFileListLine();
    
    if (!currentFile.isEmpty())
    {
        params << "annotate" << "--" << currentFile.mid(2);   //Jump over status marker characters (e.g "M ")

        m_runner->requestAction(HgAction(ACT_ANNOTATE, m_workFolderPath, params));
    }
}

void MainWindow::hgResolveList()
{
    QStringList params;

    params << "resolve" << "--list";
    m_runner->requestAction(HgAction(ACT_RESOLVE_LIST, m_workFolderPath, params));
}

void MainWindow::hgAdd()
{
    QStringList params;

    // hgExplorer permitted adding "all" files -- I'm not sure
    // that one is a good idea, let's require the user to select

    QStringList files = m_hgTabs->getSelectedAddableFiles();

    if (!files.empty()) {
        params << "add" << "--" << files;
        m_runner->requestAction(HgAction(ACT_ADD, m_workFolderPath, params));
    }
}


void MainWindow::hgRemove()
{
    QStringList params;

    QStringList files = m_hgTabs->getSelectedRemovableFiles();

    if (!files.empty()) {
        params << "remove" << "--after" << "--force" << "--" << files;
        m_runner->requestAction(HgAction(ACT_REMOVE, m_workFolderPath, params));
    }
}

void MainWindow::hgCommit()
{
    QStringList params;
    QString comment;

    if (m_justMerged) {
        comment = m_mergeCommitComment;
    }

    QStringList files = m_hgTabs->getSelectedCommittableFiles();
    QStringList allFiles = m_hgTabs->getAllCommittableFiles();
    QStringList reportFiles = files;
    if (reportFiles.empty()) {
        reportFiles = allFiles;
    }

    QString subsetNote;
    if (reportFiles != allFiles) {
        subsetNote = tr("<p><b>Note:</b> you are committing only the files you have selected, not all of the files that have been changed!");
    }
    
    QString cf(tr("Commit files"));

    if (ConfirmCommentDialog::confirmAndGetLongComment
        (this,
         cf,
         tr("<h3>%1</h3><p>%2%3").arg(cf)
         .arg(tr("You are about to commit the following files."))
         .arg(subsetNote),
         tr("<h3>%1</h3><p>%2%3").arg(cf)
         .arg(tr("You are about to commit %n file(s).", "", reportFiles.size()))
         .arg(subsetNote),
         reportFiles,
         comment,
         tr("Commit"))) {

        if (!m_justMerged && !files.empty()) {
            // User wants to commit selected file(s) (and this is not
            // merge commit, which would fail if we selected files)
            params << "commit" << "--message" << comment
                   << "--user" << getUserInfo() << "--" << files;
        } else {
            // Commit all changes
            params << "commit" << "--message" << comment
                   << "--user" << getUserInfo();
        }
        
        m_runner->requestAction(HgAction(ACT_COMMIT, m_workFolderPath, params));
        m_mergeCommitComment = "";
    }
}

QString MainWindow::filterTag(QString tag)
{
    for(int i = 0; i < tag.size(); i++) {
        if (tag[i].isLower() || tag[i].isUpper() ||
            tag[i].isDigit() || (tag[i] == QChar('.'))) {
            //ok
        } else {
            tag[i] = QChar('_');
        }
    }
    return tag;
}


void MainWindow::hgNewBranch(QString id)
{
    QStringList params;
    QString branch;

    if (ConfirmCommentDialog::confirmAndGetShortComment
        (this,
         tr("New Branch"),
         tr("Enter new branch name:"),
         branch,
         tr("Start Branch"))) {
        if (!branch.isEmpty()) {//!!! do something better if it is empty

            params << "branch" << filterTag(branch);
            m_runner->requestAction(HgAction(ACT_NEW_BRANCH, m_workFolderPath, params));
        }
    }
}


void MainWindow::hgTag(QString id)
{
    QStringList params;
    QString tag;

    if (ConfirmCommentDialog::confirmAndGetShortComment
        (this,
         tr("Tag"),
         tr("Enter tag:"),
         tag,
         tr("Add Tag"))) {
        if (!tag.isEmpty()) {//!!! do something better if it is empty

            params << "tag" << "--user" << getUserInfo();
            params << "--rev" << Changeset::hashOf(id) << filterTag(tag);
            
            m_runner->requestAction(HgAction(ACT_TAG, m_workFolderPath, params));
        }
    }
}


void MainWindow::hgIgnore()
{
    QString hgIgnorePath;
    QStringList params;
    
    hgIgnorePath = m_workFolderPath;
    hgIgnorePath += "/.hgignore";

    if (!QDir(m_workFolderPath).exists()) return;
    QFile f(hgIgnorePath);
    if (!f.exists()) {
        f.open(QFile::WriteOnly);
        QTextStream *ts = new QTextStream(&f);
        *ts << "syntax: glob\n";
        delete ts;
        f.close();
    }
    
    params << hgIgnorePath;
    
    QString editor = getEditorBinaryName();

    if (editor == "") {
        DEBUG << "Failed to find a text editor" << endl;
        //!!! visible error!
        return;
    }

    HgAction action(ACT_HG_IGNORE, m_workFolderPath, params);
    action.executable = editor;

    m_runner->requestAction(action);
}

QString MainWindow::getDiffBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    return settings.value("extdiffbinary", "").toString();
}

QString MainWindow::getMergeBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    return settings.value("mergebinary", "").toString();
}

QString MainWindow::getEditorBinaryName()
{
    QSettings settings;
    settings.beginGroup("Locations");
    return settings.value("editorbinary", "").toString();
}

void MainWindow::hgShowSummary()
{
    QStringList params;
    
    params << "diff" << "--stat";

    m_runner->requestAction(HgAction(ACT_UNCOMMITTED_SUMMARY, m_workFolderPath, params));
}

void MainWindow::hgFolderDiff()
{
    QString diff = getDiffBinaryName();
    if (diff == "") return;

    QStringList params;

    // Diff parent against working folder (folder diff)

    params << "--config" << "extensions.extdiff=" << "extdiff";
    params << "--program" << diff;

    params << m_hgTabs->getSelectedCommittableFiles(); // may be none: whole dir

    m_runner->requestAction(HgAction(ACT_FOLDERDIFF, m_workFolderPath, params));
}


void MainWindow::hgDiffToCurrent(QString id)
{
    QString diff = getDiffBinaryName();
    if (diff == "") return;

    QStringList params;

    // Diff given revision against working folder

    params << "--config" << "extensions.extdiff=" << "extdiff";
    params << "--program" << diff;
    params << "--rev" << Changeset::hashOf(id);

    m_runner->requestAction(HgAction(ACT_FOLDERDIFF, m_workFolderPath, params));
}


void MainWindow::hgDiffToParent(QString child, QString parent)
{
    QString diff = getDiffBinaryName();
    if (diff == "") return;

    QStringList params;

    // Diff given revision against parent revision

    params << "--config" << "extensions.extdiff=" << "extdiff";
    params << "--program" << diff;
    params << "--rev" << Changeset::hashOf(parent)
           << "--rev" << Changeset::hashOf(child);

    m_runner->requestAction(HgAction(ACT_CHGSETDIFF, m_workFolderPath, params));
}


void MainWindow::hgShowSummaryFor(Changeset *cs)
{
    QStringList params;

    // This will pick a default parent if there is more than one
    // (whereas with diff we need to supply one).  But it does need a
    // bit more parsing
    params << "log" << "--stat" << "--rev" << Changeset::hashOf(cs->id());

    m_runner->requestAction(HgAction(ACT_DIFF_SUMMARY, m_workFolderPath,
                                     params, cs));
}


void MainWindow::hgUpdate()
{
    QStringList params;

    params << "update";
    
    m_runner->requestAction(HgAction(ACT_UPDATE, m_workFolderPath, params));
}


void MainWindow::hgUpdateToRev(QString id)
{
    QStringList params;

    params << "update" << "--rev" << Changeset::hashOf(id) << "--check";

    m_runner->requestAction(HgAction(ACT_UPDATE, m_workFolderPath, params));
}


void MainWindow::hgRevert()
{
    QStringList params;
    QString comment;
    bool all = false;

    QStringList files = m_hgTabs->getSelectedRevertableFiles();
    QStringList allFiles = m_hgTabs->getAllRevertableFiles();
    if (files.empty() || files == allFiles) {
        files = allFiles;
        all = true;
    }
    
    QString subsetNote;
    if (!all) {
        subsetNote = tr("<p><b>Note:</b> you are reverting only the files you have selected, not all of the files that have been changed!");
    }

    QString rf(tr("Revert files"));

    // Set up params before asking for confirmation, because there is
    // a failure case here that we would need to report on early

    DEBUG << "hgRevert: m_justMerged = " << m_justMerged << ", m_mergeTargetRevision = " << m_mergeTargetRevision << endl;

    if (m_justMerged) {

        // This is a little fiddly.  The proper way to "revert" the
        // whole of an uncommitted merge is with "hg update --clean ."
        // But if the user has selected only some files, we're sort of
        // promising to revert only those, which means we need to
        // specify which parent to revert to.  We can only do that if
        // we have a record of it, which we do if you just did the
        // merge from within easyhg but don't if you've exited and
        // restarted, or changed repository, since then.  Hmmm.

        if (all) {
            params << "update" << "--clean" << ".";
        } else {
            if (m_mergeTargetRevision != "") {
                params << "revert" << "--rev"
                       << Changeset::hashOf(m_mergeTargetRevision)
                       << "--" << files;
            } else {
                QMessageBox::information
                    (this, tr("Unable to revert"),
                     tr("<qt><b>Sorry, unable to revert these files</b><br><br>EasyMercurial can only revert a subset of files during a merge if it still has a record of which parent was the original merge target; that information is no longer available.<br><br>This is a limitation of EasyMercurial.  Consider reverting all files, or using hg revert with a specific revision at the command-line instead.</qt>"));
                return;
            }
        }
    } else {
        params << "revert" << "--" << files;
    }

    if (ConfirmCommentDialog::confirmDangerousFilesAction
        (this,
         rf,
         tr("<h3>%1</h3><p>%2%3").arg(rf)
         .arg(tr("You are about to <b>revert</b> the following files to their previous committed state.<br><br>This will <b>throw away any changes</b> that you have made to these files but have not committed."))
         .arg(subsetNote),
         tr("<h3>%1</h3><p>%2%3").arg(rf)
         .arg(tr("You are about to <b>revert</b> %n file(s).<br><br>This will <b>throw away any changes</b> that you have made to these files but have not committed.", "", files.size()))
         .arg(subsetNote),
         files,
         tr("Revert"))) {

        m_lastRevertedFiles = files;
        
        m_runner->requestAction(HgAction(ACT_REVERT, m_workFolderPath, params));
    }
}


void MainWindow::hgMarkResolved(QStringList files)
{
    QStringList params;

    params << "resolve" << "--mark";

    if (files.empty()) {
        params << "--all";
    } else {
        params << "--" << files;
    }

    m_runner->requestAction(HgAction(ACT_RESOLVE_MARK, m_workFolderPath, params));
}


void MainWindow::hgRetryMerge()
{
    QStringList params;

    params << "resolve";

    QString merge = getMergeBinaryName();
    if (merge != "") {
        params << "--tool" << merge;
    }

    QStringList files = m_hgTabs->getSelectedUnresolvedFiles();
    if (files.empty()) {
        params << "--all";
    } else {
        params << "--" << files;
    }

    if (m_currentParents.size() == 1) {
        m_mergeTargetRevision = m_currentParents[0]->id();
    }

    m_runner->requestAction(HgAction(ACT_RETRY_MERGE, m_workFolderPath, params));

    m_mergeCommitComment = tr("Merge");
}


void MainWindow::hgMerge()
{
    if (m_hgTabs->canResolve()) {
        hgRetryMerge();
        return;
    }

    QStringList params;

    params << "merge";

    QString merge = getMergeBinaryName();
    if (merge != "") {
        params << "--tool" << merge;
    }

    if (m_currentParents.size() == 1) {
        m_mergeTargetRevision = m_currentParents[0]->id();
    }

    m_runner->requestAction(HgAction(ACT_MERGE, m_workFolderPath, params));

    m_mergeCommitComment = tr("Merge");
}


void MainWindow::hgMergeFrom(QString id)
{
    QStringList params;

    params << "merge";
    params << "--rev" << Changeset::hashOf(id);

    QString merge = getMergeBinaryName();
    if (merge != "") {
        params << "--tool" << merge;
    }
    
    if (m_currentParents.size() == 1) {
        m_mergeTargetRevision = m_currentParents[0]->id();
    }

    m_runner->requestAction(HgAction(ACT_MERGE, m_workFolderPath, params));

    m_mergeCommitComment = "";

    foreach (Changeset *cs, m_currentHeads) {
        if (cs->id() == id && !cs->isOnBranch(m_currentBranch)) {
            if (cs->branch() == "" || cs->branch() == "default") {
                m_mergeCommitComment = tr("Merge from the default branch");
            } else {
                m_mergeCommitComment = tr("Merge from branch \"%1\"").arg(cs->branch());
            }
        }
    }

    if (m_mergeCommitComment == "") {
        m_mergeCommitComment = tr("Merge from %1").arg(id);
    }
}


void MainWindow::hgCloneFromRemote()
{
    QStringList params;

    if (!QDir(m_workFolderPath).exists()) {
        if (!QDir().mkpath(m_workFolderPath)) {
            DEBUG << "hgCloneFromRemote: Failed to create target path "
                  << m_workFolderPath << endl;
            //!!! report error
            return;
        }
    }

    params << "clone" << m_remoteRepoPath << m_workFolderPath;
    
    updateWorkFolderAndRepoNames();
    m_hgTabs->updateWorkFolderFileList("");

    m_runner->requestAction(HgAction(ACT_CLONEFROMREMOTE, m_workFolderPath, params));
}

void MainWindow::hgInit()
{
    QStringList params;

    params << "init";
    params << m_workFolderPath;

    m_runner->requestAction(HgAction(ACT_INIT, m_workFolderPath, params));
}

void MainWindow::hgIncoming()
{
    QStringList params;

    params << "incoming" << "--newest-first" << m_remoteRepoPath;
    params << "--template" << Changeset::getLogTemplate();

    m_runner->requestAction(HgAction(ACT_INCOMING, m_workFolderPath, params));
}

void MainWindow::hgPull()
{
    if (ConfirmCommentDialog::confirm
        (this, tr("Confirm pull"),
         tr("<qt><h3>Pull from remote repository?</h3></qt>"),
         tr("<qt><p>You are about to pull changes from the remote repository at <code>%1</code>.</p></qt>").arg(xmlEncode(m_remoteRepoPath)),
         tr("Pull"))) {

        QStringList params;
        params << "pull" << m_remoteRepoPath;
        m_runner->requestAction(HgAction(ACT_PULL, m_workFolderPath, params));
    }
}

void MainWindow::hgPush()
{
    if (ConfirmCommentDialog::confirm
        (this, tr("Confirm push"),
         tr("<qt><h3>Push to remote repository?</h3></qt>"),
         tr("<qt><p>You are about to push your changes to the remote repository at <code>%1</code>.</p></qt>").arg(xmlEncode(m_remoteRepoPath)),
         tr("Push"))) {

        QStringList params;
        params << "push" << "--new-branch" << m_remoteRepoPath;
        m_runner->requestAction(HgAction(ACT_PUSH, m_workFolderPath, params));
    }
}

QStringList MainWindow::listAllUpIpV4Addresses()
{
    QStringList ret;
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();

    for (int i = 0; i < ifaces.count(); i++) {
        QNetworkInterface iface = ifaces.at(i);
        if (iface.flags().testFlag(QNetworkInterface::IsUp)
            && !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            for (int j=0; j<iface.addressEntries().count(); j++) {
                QHostAddress tmp = iface.addressEntries().at(j).ip();
                if (QAbstractSocket::IPv4Protocol == tmp.protocol()) {
                    ret.push_back(tmp.toString());
                }
            }
        }
    }
    return ret;
}

void MainWindow::clearState()
{
    DEBUG << "MainWindow::clearState" << endl;
    foreach (Changeset *cs, m_currentParents) delete cs;
    m_currentParents.clear();
    foreach (Changeset *cs, m_currentHeads) delete cs;
    m_currentHeads.clear();
    m_currentBranch = "";
    m_lastStatOutput = "";
    m_lastRevertedFiles.clear();
    m_mergeTargetRevision = "";
    m_mergeCommitComment = "";
    m_stateUnknown = true;
    m_needNewLog = true;
    if (m_fsWatcher) {
        delete m_fsWatcherGeneralTimer;
        m_fsWatcherGeneralTimer = 0;
        delete m_fsWatcherRestoreTimer;
        m_fsWatcherRestoreTimer = 0;
        delete m_fsWatcher;
        m_fsWatcher = 0;
    }
}

void MainWindow::hgServe()
{
    QStringList params;
    QString msg;

    QStringList addrs = listAllUpIpV4Addresses();

    if (addrs.empty()) {
        QMessageBox::critical
            (this, tr("Serve"), tr("Failed to identify an active IPv4 address"));
        return;
    }

    //!!! should find available port as well

    QTextStream ts(&msg);
    ts << QString("<qt><p>%1</p>")
        .arg(tr("Running temporary server at %n address(es):", "", addrs.size()));
    foreach (QString addr, addrs) {
        ts << QString("<pre>&nbsp;&nbsp;http://%1:8000</pre>").arg(xmlEncode(addr));
    }
    ts << tr("<p>Press Close to stop the server and return.</p>");
    ts.flush();
             
    params << "serve";

    m_runner->requestAction(HgAction(ACT_SERVE, m_workFolderPath, params));
    
    QMessageBox::information(this, tr("Serve"), msg, QMessageBox::Close);

    m_runner->killCurrentActions();
}

void MainWindow::startupDialog()
{
    StartupDialog *dlg = new StartupDialog(this);
    if (dlg->exec()) m_firstStart = false;
}

void MainWindow::open()
{
    bool done = false;

    while (!done) {

        MultiChoiceDialog *d = new MultiChoiceDialog
                               (tr("Open Repository"),
                                tr("<qt><big>What would you like to open?</big></qt>"),
                                this);

        d->addChoice("remote",
                     tr("<qt><center><img src=\":images/browser-64.png\"><br>Remote repository</center></qt>"),
                     tr("Open a remote Mercurial repository, by cloning from its URL into a local folder."),
                     MultiChoiceDialog::UrlToDirectoryArg);

        d->addChoice("local",
                     tr("<qt><center><img src=\":images/hglogo-64.png\"><br>Local repository</center></qt>"),
                     tr("Open an existing local Mercurial repository."),
                     MultiChoiceDialog::DirectoryArg);

        d->addChoice("init",
                     tr("<qt><center><img src=\":images/hdd_unmount-64.png\"><br>File folder</center></qt>"),
                     tr("Open a local folder, by creating a Mercurial repository in it."),
                     MultiChoiceDialog::DirectoryArg);

        QSettings settings;
        settings.beginGroup("General");
        QString lastChoice = settings.value("lastopentype", "local").toString();
        if (lastChoice != "local" &&
            lastChoice != "remote" &&
            lastChoice != "init") {
            lastChoice = "local";
        }

        d->setCurrentChoice(lastChoice);

        if (d->exec() == QDialog::Accepted) {

            QString choice = d->getCurrentChoice();
            settings.setValue("lastopentype", choice);

            QString arg = d->getArgument().trimmed();

            bool result = false;

            if (choice == "local") {
                result = openLocal(arg);
            } else if (choice == "remote") {
                result = openRemote(arg, d->getAdditionalArgument().trimmed());
            } else if (choice == "init") {
                result = openInit(arg);
            }

            if (result) {
                enableDisableActions();
                clearState();
                hgQueryPaths();
                done = true;
            }

        } else {

            // cancelled
            done = true;
        }

        delete d;
    }
}

void MainWindow::changeRemoteRepo()
{
    // This will involve rewriting the local .hgrc

    QDir hgDir(m_workFolderPath + "/.hg");
    if (!hgDir.exists()) {
        //!!! visible error!
        return;
    }

    QFileInfo hgrc(m_workFolderPath + "/.hg/hgrc");
    if (hgrc.exists() && !hgrc.isWritable()) {
        //!!! visible error!
        return;
    }

    MultiChoiceDialog *d = new MultiChoiceDialog
        (tr("Change Remote Location"),
         tr("<qt><big>Change the remote location</big></qt>"),
         this);

    d->addChoice("remote",
                 tr("<qt><center><img src=\":images/browser-64.png\"><br>Remote repository</center></qt>"),
                 tr("Provide a new URL to use for push and pull actions from the current local repository."),
                 MultiChoiceDialog::UrlArg);

    if (d->exec() == QDialog::Accepted) {

        // New block to ensure QSettings is deleted before
        // hgQueryPaths called.  NB use of absoluteFilePath instead of
        // canonicalFilePath, which would fail if the file did not yet
        // exist

        {
            QSettings s(hgrc.absoluteFilePath(), QSettings::IniFormat);
            s.beginGroup("paths");
            s.setValue("default", d->getArgument());
        }

        m_stateUnknown = true;
        hgQueryPaths();
    }

    delete d;
}

void MainWindow::open(QString local)
{
    if (openLocal(local)) {
        enableDisableActions();
        clearState();
        hgQueryPaths();
    }
}

bool MainWindow::complainAboutFilePath(QString arg)
{    
    QMessageBox::critical
        (this, tr("File chosen"),
         tr("<qt><b>Folder required</b><br><br>You asked to open \"%1\".<br>This is a file; to open a repository, you need to choose a folder.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::askAboutUnknownFolder(QString arg)
{    
    bool result = (QMessageBox::question
                   (this, tr("Path does not exist"),
                    tr("<qt><b>Path does not exist: create it?</b><br><br>You asked to open a remote repository by cloning it to \"%1\". This folder does not exist, and neither does its parent.<br><br>Would you like to create the parent folder as well?</qt>").arg(xmlEncode(arg)),
                    QMessageBox::Ok | QMessageBox::Cancel,
                    QMessageBox::Cancel)
                   == QMessageBox::Ok);
    if (result) {
        QDir dir(arg);
        dir.cdUp();
        if (!dir.mkpath(dir.absolutePath())) {
            QMessageBox::critical
                (this, tr("Failed to create folder"),
                 tr("<qt><b>Failed to create folder</b><br><br>Sorry, the path for the parent folder \"%1\" could not be created.</qt>").arg(dir.absolutePath()));
            return false;
        }
        return true;
    }
    return false;
}

bool MainWindow::complainAboutUnknownFolder(QString arg)
{    
    QMessageBox::critical
        (this, tr("Folder does not exist"),
         tr("<qt><b>Folder does not exist</b><br><br>You asked to open \"%1\".<br>This folder does not exist, and it cannot be created because its parent does not exist either.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutInitInRepo(QString arg)
{
    QMessageBox::critical
        (this, tr("Path is in existing repository"),
         tr("<qt><b>Path is in an existing repository</b><br><br>You asked to initialise a repository at \"%1\".<br>This path is already inside an existing repository.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutInitFile(QString arg)
{
    QMessageBox::critical
        (this, tr("Path is a file"),
         tr("<qt><b>Path is a file</b><br><br>You asked to initialise a repository at \"%1\".<br>This is an existing file; it is only possible to initialise in folders.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutCloneToExisting(QString arg)
{
    QMessageBox::critical
        (this, tr("Path is in existing repository"),
         tr("<qt><b>Local path is in an existing repository</b><br><br>You asked to open a remote repository by cloning it to the local path \"%1\".<br>This path is already inside an existing repository.<br>Please provide a different folder name for the local repository.</qt>").arg(xmlEncode(arg)));
    return false;
}

bool MainWindow::complainAboutCloneToFile(QString arg)
{
    QMessageBox::critical
        (this, tr("Path is a file"),
         tr("<qt><b>Local path is a file</b><br><br>You asked to open a remote repository by cloning it to the local path \"%1\".<br>This path is an existing file.<br>Please provide a new folder name for the local repository.</qt>").arg(xmlEncode(arg)));
    return false;
}

QString MainWindow::complainAboutCloneToExistingFolder(QString arg, QString remote)
{
    // If the directory "arg" exists but "arg" plus the last path
    // component of "remote" does not, then offer the latter as an
    // alternative path

    QString offer;

    QDir d(arg);
    if (d.exists()) {
        if (QRegExp("^\\w+://").indexIn(remote) >= 0) {
            QString rpath = QUrl(remote).path();
            if (rpath != "") {
                rpath = QDir(rpath).dirName();
                if (rpath != "" && !d.exists(rpath)) {
                    offer = d.filePath(rpath);
                }
            }
        }
    }

    if (offer != "") {
        bool result = (QMessageBox::question
                       (this, tr("Folder exists"),
                        tr("<qt><b>Local folder already exists</b><br><br>You asked to open a remote repository by cloning it to \"%1\", but this folder already exists and so cannot be cloned to.<br><br>Would you like to create the new folder \"%2\" instead?</qt>")
                        .arg(xmlEncode(arg)).arg(xmlEncode(offer)),
                        QMessageBox::Ok | QMessageBox::Cancel,
                        QMessageBox::Cancel)
                       == QMessageBox::Ok);
        if (result) return offer;
        else return "";
    }

    QMessageBox::critical
        (this, tr("Folder exists"),
         tr("<qt><b>Local folder already exists</b><br><br>You asked to open a remote repository by cloning it to \"%1\", but this file or folder already exists and so cannot be cloned to.<br>Please provide a different folder name for the local repository.</qt>").arg(xmlEncode(arg)));
    return "";
}

bool MainWindow::askToOpenParentRepo(QString arg, QString parent)
{
    return (QMessageBox::question
            (this, tr("Path is inside a repository"),
             tr("<qt><b>Open the repository that contains this path?</b><br><br>You asked to open \"%1\".<br>This is not the root folder of a repository.<br>But it is inside a repository, whose root is at \"%2\". <br><br>Would you like to open that repository instead?</qt>")
             .arg(xmlEncode(arg)).arg(xmlEncode(parent)),
             QMessageBox::Ok | QMessageBox::Cancel,
             QMessageBox::Ok)
            == QMessageBox::Ok);
}

bool MainWindow::askToInitExisting(QString arg)
{
    return (QMessageBox::question
            (this, tr("Folder has no repository"),
             tr("<qt><b>Initialise a repository here?</b><br><br>You asked to open \"%1\".<br>This folder does not contain a Mercurial repository.<br><br>Would you like to initialise a repository here?</qt>")
             .arg(xmlEncode(arg)),
             QMessageBox::Ok | QMessageBox::Cancel,
             QMessageBox::Ok)
            == QMessageBox::Ok);
}

bool MainWindow::askToInitNew(QString arg)
{
    return (QMessageBox::question
            (this, tr("Folder does not exist"),
             tr("<qt><b>Initialise a new repository?</b><br><br>You asked to open \"%1\".<br>This folder does not yet exist.<br><br>Would you like to create the folder and initialise a new empty repository in it?</qt>")
             .arg(xmlEncode(arg)),
             QMessageBox::Ok | QMessageBox::Cancel,
             QMessageBox::Ok)
            == QMessageBox::Ok);
}

bool MainWindow::askToOpenInsteadOfInit(QString arg)
{
    return (QMessageBox::question
            (this, tr("Repository exists"),
             tr("<qt><b>Open existing repository?</b><br><br>You asked to initialise a new repository at \"%1\".<br>This folder already contains a repository.  Would you like to open it?</qt>")
             .arg(xmlEncode(arg)),
             QMessageBox::Ok | QMessageBox::Cancel,
             QMessageBox::Ok)
            == QMessageBox::Ok);
}

bool MainWindow::openLocal(QString local)
{
    DEBUG << "open " << local << endl;

    FolderStatus status = getFolderStatus(local);
    QString containing = getContainingRepoFolder(local);

    switch (status) {

    case FolderHasRepo:
        // fine
        break;

    case FolderExists:
        if (containing != "") {
            if (!askToOpenParentRepo(local, containing)) return false;
            local = containing;
        } else {
            //!!! No -- this is likely to happen far more by accident
            // than because the user actually wanted to init something.
            // Don't ask, just politely reject.
            if (!askToInitExisting(local)) return false;
            return openInit(local);
        }
        break;

    case FolderParentExists:
        if (containing != "") {
            if (!askToOpenParentRepo(local, containing)) return false;
            local = containing;
        } else {
            if (!askToInitNew(local)) return false;
            return openInit(local);
        }
        break;

    case FolderUnknown:
        if (containing != "") {
            if (!askToOpenParentRepo(local, containing)) return false;
            local = containing;
        } else {
            return complainAboutUnknownFolder(local);
        }
        break;
        
    case FolderIsFile:
        return complainAboutFilePath(local);
    }

    m_workFolderPath = local;
    m_remoteRepoPath = "";
    return true;
}    

bool MainWindow::openRemote(QString remote, QString local)
{
    DEBUG << "clone " << remote << " to " << local << endl;

    FolderStatus status = getFolderStatus(local);
    QString containing = getContainingRepoFolder(local);

    DEBUG << "status = " << status << ", containing = " << containing << endl;

    if (status == FolderHasRepo || containing != "") {
        return complainAboutCloneToExisting(local);
    }

    if (status == FolderIsFile) {
        return complainAboutCloneToFile(local);
    }

    if (status == FolderUnknown) {
        if (!askAboutUnknownFolder(local)) {
            return false;
        }
    }

    if (status == FolderExists) {
        local = complainAboutCloneToExistingFolder(local, remote);
        if (local == "") return false;
    }

    m_workFolderPath = local;
    m_remoteRepoPath = remote;
    hgCloneFromRemote();

    return true;
}

bool MainWindow::openInit(QString local)
{
    DEBUG << "openInit " << local << endl;

    FolderStatus status = getFolderStatus(local);
    QString containing = getContainingRepoFolder(local);

    DEBUG << "status = " << status << ", containing = " << containing << endl;

    if (status == FolderHasRepo) {
        if (!askToOpenInsteadOfInit(local)) return false;
    }

    if (containing != "") {
        return complainAboutInitInRepo(local);
    }

    if (status == FolderIsFile) {
        return complainAboutInitFile(local);
    }

    if (status == FolderUnknown) {
        return complainAboutUnknownFolder(local);
    }

    m_workFolderPath = local;
    m_remoteRepoPath = "";
    hgInit();
    return true;
}

void MainWindow::settings()
{
    SettingsDialog *settingsDlg = new SettingsDialog(this);
    settingsDlg->exec();

    if (settingsDlg->presentationChanged()) {
        m_hgTabs->updateFileStates();
        updateToolBarStyle();
        hgRefresh();
    }
}

#define STDOUT_NEEDS_BIG_WINDOW 512
#define SMALL_WND_W     500
#define SMALL_WND_H     300

#define BIG_WND_W       1024
#define BIG_WND_H       768


void MainWindow::presentLongStdoutToUser(QString stdo)
{
    if (!stdo.isEmpty())
    {
        QDialog dlg;

        if (stdo.length() > STDOUT_NEEDS_BIG_WINDOW)
        {
            dlg.setMinimumWidth(BIG_WND_W);
            dlg.setMinimumHeight(BIG_WND_H);
        }
        else
        {
            dlg.setMinimumWidth(SMALL_WND_W);
            dlg.setMinimumHeight(SMALL_WND_H);
        }

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
    else
    {
        QMessageBox::information(this, tr("EasyMercurial"), tr("Mercurial command did not return any output."));
    }
}

void MainWindow::updateFileSystemWatcher()
{
    bool justCreated = false;
    if (!m_fsWatcher) {
        m_fsWatcher = new QFileSystemWatcher();
        justCreated = true;
    }

    // QFileSystemWatcher will refuse to add a file or directory to
    // its watch list that it is already watching -- fine, that's what
    // we want -- but it prints a warning when this happens, which is
    // annoying because it would be the normal case for us.  So we'll
    // check for duplicates ourselves.
    QSet<QString> alreadyWatched;
    QStringList dl(m_fsWatcher->directories());
    foreach (QString d, dl) alreadyWatched.insert(d);
    
    std::deque<QString> pending;
    pending.push_back(m_workFolderPath);

    while (!pending.empty()) {

        QString path = pending.front();
        pending.pop_front();
        if (!alreadyWatched.contains(path)) {
            m_fsWatcher->addPath(path);
            DEBUG << "Added to file system watcher: " << path << endl;
        }

        QDir d(path);
        if (d.exists()) {
            d.setFilter(QDir::Dirs | QDir::NoDotAndDotDot |
                        QDir::Readable | QDir::NoSymLinks);
            foreach (QString entry, d.entryList()) {
                if (entry.startsWith('.')) continue;
                QString entryPath = d.absoluteFilePath(entry);
                pending.push_back(entryPath);
            }
        }
    }

    // The general timer isn't really related to the fs watcher
    // object, it just does something similar -- every now and then we
    // do a refresh just to update the history dates etc

    m_fsWatcherGeneralTimer = new QTimer(this);
    connect(m_fsWatcherGeneralTimer, SIGNAL(timeout()),
            this, SLOT(checkFilesystem()));
    m_fsWatcherGeneralTimer->setInterval(30 * 60 * 1000); // half an hour
    m_fsWatcherGeneralTimer->start();

    if (justCreated) {
        connect(m_fsWatcher, SIGNAL(directoryChanged(QString)),
                this, SLOT(fsDirectoryChanged(QString)));
        connect(m_fsWatcher, SIGNAL(fileChanged(QString)),
                this, SLOT(fsFileChanged(QString)));
    }
}

void MainWindow::suspendFileSystemWatcher()
{
    DEBUG << "MainWindow::suspendFileSystemWatcher" << endl;
    if (m_fsWatcher) {
        m_fsWatcherSuspended = true;
        if (m_fsWatcherRestoreTimer) {
            delete m_fsWatcherRestoreTimer;
            m_fsWatcherRestoreTimer = 0;
        }
        m_fsWatcherGeneralTimer->stop();
    }
}

void MainWindow::restoreFileSystemWatcher()
{
    DEBUG << "MainWindow::restoreFileSystemWatcher" << endl;
    if (m_fsWatcherRestoreTimer) delete m_fsWatcherRestoreTimer;
        
    // The restore timer is used to leave a polite interval between
    // being asked to restore the watcher and actually doing so.  It's
    // a single shot timer each time it's used, but we don't use
    // QTimer::singleShot because we want to stop the previous one if
    // it's running (via deleting it)

    m_fsWatcherRestoreTimer = new QTimer(this);
    connect(m_fsWatcherRestoreTimer, SIGNAL(timeout()),
            this, SLOT(actuallyRestoreFileSystemWatcher()));
    m_fsWatcherRestoreTimer->setInterval(1000);
    m_fsWatcherRestoreTimer->setSingleShot(true);
    m_fsWatcherRestoreTimer->start();
}

void MainWindow::actuallyRestoreFileSystemWatcher()
{
    DEBUG << "MainWindow::actuallyRestoreFileSystemWatcher" << endl;
    if (m_fsWatcher) {
        m_fsWatcherSuspended = false;
        m_fsWatcherGeneralTimer->start();
    }
}

void MainWindow::checkFilesystem()
{
    DEBUG << "MainWindow::checkFilesystem" << endl;
    hgRefresh();
}

void MainWindow::fsDirectoryChanged(QString d)
{
    DEBUG << "MainWindow::fsDirectoryChanged " << d << endl;
    if (!m_fsWatcherSuspended) {
        hgStat();
    }
}

void MainWindow::fsFileChanged(QString f)
{
    DEBUG << "MainWindow::fsFileChanged " << f << endl;
    if (!m_fsWatcherSuspended) {
        hgStat();
    }
}

QString MainWindow::format1(QString head)
{
    return QString("<qt><h3>%1</h3></qt>").arg(head);
}    

QString MainWindow::format3(QString head, QString intro, QString code)
{
    code = xmlEncode(code).replace("\n", "<br>")
#ifndef Q_OS_WIN32
           // The hard hyphen comes out funny on Windows
           .replace("-", "&#8209;")
#endif
           .replace(" ", "&nbsp;");
    if (intro == "") {
        return QString("<qt><h3>%1</h3><p><code>%2</code></p>")
            .arg(head).arg(code);
    } else if (code == "") {
        return QString("<qt><h3>%1</h3><p>%2</p>")
            .arg(head).arg(intro);
    } else {
        return QString("<qt><h3>%1</h3><p>%2</p><p><code>%3</code></p>")
            .arg(head).arg(intro).arg(code);
    }
}

void MainWindow::showIncoming(QString output)
{
    m_runner->hide();
    IncomingDialog *d = new IncomingDialog(this, output);
    d->exec();
    delete d;
}

int MainWindow::extractChangeCount(QString text)
{
    QRegExp re("added (\\d+) ch\\w+ with (\\d+) ch\\w+ to (\\d+) f\\w+");
    if (re.indexIn(text) >= 0) {
        return re.cap(1).toInt();
    } else if (text.contains("no changes")) {
        return 0;
    } else {
        return -1; // unknown
    }
}

void MainWindow::showPushResult(QString output)
{
    QString head;
    QString report;
    int n = extractChangeCount(output);
    if (n > 0) {
        head = tr("Pushed %n changeset(s)", "", n);
        report = tr("<qt>Successfully pushed to the remote repository at <code>%1</code>.</qt>").arg(xmlEncode(m_remoteRepoPath));
    } else if (n == 0) {
        head = tr("No changes to push");
        report = tr("The remote repository already contains all changes that have been committed locally.");
        if (m_hgTabs->canCommit()) {
            report = tr("%1<p>You do have some uncommitted changes. If you wish to push those to the remote repository, commit them locally first.").arg(report);
        }            
    } else {
        head = tr("Push complete");
    }
    m_runner->hide();

    MoreInformationDialog::information(this, tr("Push complete"),
                                       head, report, output);
}

void MainWindow::showPullResult(QString output)
{
    QString head;
    QString report;
    int n = extractChangeCount(output);
    if (n > 0) {
        head = tr("Pulled %n changeset(s)", "", n);
        report = tr("The new changes will be highlighted in the history.<br>Use Update to bring these changes into your working copy.");
    } else if (n == 0) {
        head = tr("No changes to pull");
        report = tr("Your local repository already contains all changes found in the remote repository.");
    } else {
        head = tr("Pull complete");
    }
    m_runner->hide();

    MoreInformationDialog::information(this, tr("Pull complete"),
                                       head, report, output);
}

void MainWindow::reportNewRemoteHeads(QString output)
{
    bool headsAreLocal = false;

    if (m_currentParents.size() == 1) {
        int m_currentBranchHeads = 0;
        bool parentIsHead = false;
        Changeset *parent = m_currentParents[0];
        foreach (Changeset *head, m_currentHeads) {
            if (head->isOnBranch(m_currentBranch)) {
                ++m_currentBranchHeads;
            }
            if (parent->id() == head->id()) {
                parentIsHead = true;
            }
        }
        if (m_currentBranchHeads == 2 && parentIsHead) {
            headsAreLocal = true;
        }
    }

    if (headsAreLocal) {
        MoreInformationDialog::warning
            (this,
             tr("Push failed"),
             tr("Push failed"),
             tr("Your local repository could not be pushed to the remote repository.<br><br>You may need to merge the changes locally first."),
             output);
    } else {
        MoreInformationDialog::warning
            (this,
             tr("Push failed"),
             tr("Push failed"),
             tr("Your local repository could not be pushed to the remote repository.<br><br>The remote repository may have been changed by someone else since you last pushed. Try pulling and merging their changes into your local repository first."),
             output);
    }
}

void MainWindow::commandStarting(HgAction action)
{
    // Annoyingly, hg stat actually modifies the working directory --
    // it creates files called hg-checklink and hg-checkexec to test
    // properties of the filesystem.  For safety's sake, suspend the
    // fs watcher while running commands, and restore it shortly after
    // a command has finished.

    if (action.action == ACT_STAT) {
        suspendFileSystemWatcher();
    }
}

void MainWindow::commandFailed(HgAction action, QString output)
{
    DEBUG << "MainWindow::commandFailed" << endl;
    restoreFileSystemWatcher();

    QString setstr;
#ifdef Q_OS_MAC
    setstr = tr("Preferences");
#else
    setstr = tr("Settings");
#endif

    // Some commands we just have to ignore bad return values from:

    switch(action.action) {
    case ACT_NONE:
        // uh huh
        return;
    case ACT_TEST_HG:
        MoreInformationDialog::warning
            (this,
             tr("Failed to run Mercurial"),
             tr("Failed to run Mercurial"),
             tr("The Mercurial program either could not be found or failed to run.<br>Check that the Mercurial program path is correct in %1.").arg(setstr),
             output);
        settings();
        return;
    case ACT_TEST_HG_EXT:
        QMessageBox::warning
            (this,
             tr("Failed to run Mercurial"),
             tr("Failed to run Mercurial with extension enabled"),
             tr("The Mercurial program failed to run with the EasyMercurial interaction extension enabled.<br>This may indicate an installation problem with EasyMercurial.<br><br>You may be able to continue working if you switch off &ldquo;Use EasyHg Mercurial Extension&rdquo; in %1.  Note that remote repositories that require authentication may not work if you do this.").arg(setstr),
             output);
        settings();
        return;
    case ACT_CLONEFROMREMOTE:
        // if clone fails, we have no repo
        m_workFolderPath = "";
        enableDisableActions();
        break;
    case ACT_INCOMING:
        // returns non-zero code and no output if the check was
        // successful but there are no changes pending
        if (output.replace(QRegExp("(^|\\n)warning: [^\\n]*\\n"), "").trimmed() == "") {
            showIncoming("");
            return;
        }
        break;
    case ACT_QUERY_HEADS:
        // fails if repo is empty; we don't care (if there's a genuine
        // problem, something else will fail too).  Pretend it
        // succeeded, so that any further actions that are contingent
        // on the success of the heads query get carried out properly.
        commandCompleted(action, "");
        return;
    case ACT_FOLDERDIFF:
    case ACT_CHGSETDIFF:
        // external program, unlikely to be anything useful in stderr
        // and some return with failure codes when something as basic
        // as the user closing the window via the wm happens
        return;
    case ACT_PUSH:
        if (output.contains("creates new remote heads")) {
            reportNewRemoteHeads(output);
            return;
        }
    case ACT_STAT:
        break; // go on and report
    default:
        break;
    }

    QString command = action.executable;
    if (command == "") command = "hg";
    foreach (QString arg, action.params) {
        command += " " + arg;
    }

    //!!!

    QString message = tr("<qt><h3>Command failed</h3>"
                         "<p>The following command failed:</p>"
                         "<code>%1</code>"
                         "%2</qt>")
        .arg(command)
        .arg((output.trimmed() != "") ?
             tr("<p>Its output said:</p><code>%1</code>")
             .arg(xmlEncode(output.left(800))
                  .replace("\n", "<br>"))
             : "");

    QMessageBox::warning(this, tr("Command failed"), message);
}

void MainWindow::commandCompleted(HgAction completedAction, QString output)
{
    restoreFileSystemWatcher();
    HGACTIONS action = completedAction.action;

    if (action == ACT_NONE) return;

    bool headsChanged = false;
    QStringList oldHeadIds;

    switch (action) {

    case ACT_TEST_HG:
        break;

    case ACT_TEST_HG_EXT:
        break;

    case ACT_QUERY_PATHS:
    {
        DEBUG << "stdout is " << output << endl;
        LogParser lp(output, "=");
        LogList ll = lp.parse();
        DEBUG << ll.size() << " results" << endl;
        if (!ll.empty()) {
            m_remoteRepoPath = lp.parse()[0]["default"].trimmed();
            DEBUG << "Set remote path to " << m_remoteRepoPath << endl;
        } else {
            m_remoteRepoPath = "";
        }
        MultiChoiceDialog::addRecentArgument("local", m_workFolderPath);
        MultiChoiceDialog::addRecentArgument("remote", m_remoteRepoPath);
        updateWorkFolderAndRepoNames();
        break;
    }

    case ACT_QUERY_BRANCH:
        m_currentBranch = output.trimmed();
        break;

    case ACT_STAT:
        m_lastStatOutput = output;
        updateFileSystemWatcher();
        break;

    case ACT_RESOLVE_LIST:
        if (output != "") {
            // Remove lines beginning with R (they are resolved,
            // and the file stat parser treats R as removed)
            QStringList outList = output.split('\n');
            QStringList winnowed;
            foreach (QString line, outList) {
                if (!line.startsWith("R ")) winnowed.push_back(line);
            }
            output = winnowed.join("\n");
        }
        DEBUG << "m_lastStatOutput = " << m_lastStatOutput << endl;
        DEBUG << "resolve output = " << output << endl;
        m_hgTabs->updateWorkFolderFileList(m_lastStatOutput + output);
        break;

    case ACT_RESOLVE_MARK:
        m_shouldHgStat = true;
        break;
        
    case ACT_INCOMING:
        showIncoming(output);
        break;

    case ACT_ANNOTATE:
        presentLongStdoutToUser(output);
        m_shouldHgStat = true;
        break;
        
    case ACT_PULL:
        showPullResult(output);
        m_shouldHgStat = true;
        break;
        
    case ACT_PUSH:
        showPushResult(output);
        break;
        
    case ACT_INIT:
        MultiChoiceDialog::addRecentArgument("init", m_workFolderPath);
        MultiChoiceDialog::addRecentArgument("local", m_workFolderPath);
        enableDisableActions();
        m_shouldHgStat = true;
        break;
        
    case ACT_CLONEFROMREMOTE:
        MultiChoiceDialog::addRecentArgument("local", m_workFolderPath);
        MultiChoiceDialog::addRecentArgument("remote", m_remoteRepoPath);
        MultiChoiceDialog::addRecentArgument("remote", m_workFolderPath, true);
        MoreInformationDialog::information
            (this,
             tr("Clone"),
             tr("Clone successful"),
             tr("The remote repository was successfully cloned to the local folder <code>%1</code>.").arg(xmlEncode(m_workFolderPath)),
             output);
        enableDisableActions();
        m_shouldHgStat = true;
        break;
        
    case ACT_LOG:
        m_hgTabs->setNewLog(output);
        m_needNewLog = false;
        break;
        
    case ACT_LOG_INCREMENTAL:
        m_hgTabs->addIncrementalLog(output);
        break;
        
    case ACT_QUERY_PARENTS:
    {
        foreach (Changeset *cs, m_currentParents) delete cs;
        m_currentParents = Changeset::parseChangesets(output);
        QStringList parentIds = Changeset::getIds(m_currentParents);
        m_hgTabs->setCurrent(parentIds, m_currentBranch);
    }
        break;
        
    case ACT_QUERY_HEADS:
    {
        oldHeadIds = Changeset::getIds(m_currentHeads);
        Changesets newHeads = Changeset::parseChangesets(output);
        QStringList newHeadIds = Changeset::getIds(newHeads);
        if (oldHeadIds != newHeadIds) {
            DEBUG << "Heads changed, will prompt an incremental log if appropriate" << endl;
            DEBUG << "Old heads: " << oldHeadIds.join(",") << endl;
            DEBUG << "New heads: " << newHeadIds.join(",") << endl;
            headsChanged = true;
            foreach (Changeset *cs, m_currentHeads) delete cs;
            m_currentHeads = newHeads;
        }
    }
        break;

    case ACT_COMMIT:
        m_hgTabs->clearSelections();
        m_justMerged = false;
        m_shouldHgStat = true;
        break;

    case ACT_REVERT:
        hgMarkResolved(m_lastRevertedFiles);
        m_justMerged = false;
        break;
        
    case ACT_REMOVE:
    case ACT_ADD:
        m_hgTabs->clearSelections();
        m_shouldHgStat = true;
        break;

    case ACT_TAG:
        m_needNewLog = true;
        m_shouldHgStat = true;
        break;

    case ACT_NEW_BRANCH:
        m_shouldHgStat = true;
        m_hgTabs->showWorkTab();
        break;

    case ACT_UNCOMMITTED_SUMMARY:
        QMessageBox::information(this, tr("Change summary"),
                                 format3(tr("Summary of uncommitted changes"),
                                         "",
                                         output));
        break;

    case ACT_DIFF_SUMMARY:
    {
        // Output has log info first, diff following after a blank line
        output.replace("\r\n", "\n");
        QStringList olist = output.split("\n\n", QString::SkipEmptyParts);
        if (olist.size() > 1) output = olist[1];

        Changeset *cs = (Changeset *)completedAction.extraData;
        if (cs) {
            QMessageBox::information
                (this, tr("Change summary"),
                 format3(tr("Summary of changes"),
                         cs->formatHtml(),
                         output));
        } else if (output == "") {
            // Can happen, for a merge commit (depending on parent)
            QMessageBox::information(this, tr("Change summary"),
                                     format3(tr("Summary of changes"),
                                             tr("No changes"),
                                             output));
        } else {
            QMessageBox::information(this, tr("Change summary"),
                                     format3(tr("Summary of changes"),
                                             "",
                                             output));
        }            
        break;
    }

    case ACT_FOLDERDIFF:
    case ACT_CHGSETDIFF:
    case ACT_SERVE:
    case ACT_HG_IGNORE:
        m_shouldHgStat = true;
        break;
        
    case ACT_UPDATE:
        QMessageBox::information(this, tr("Update"), tr("<qt><h3>Update successful</h3><p>%1</p>").arg(xmlEncode(output)));
        m_shouldHgStat = true;
        break;
        
    case ACT_MERGE:
        MoreInformationDialog::information
            (this, tr("Merge"), tr("Merge successful"),
             tr("Remember to test and commit the result before making any further changes."),
             output);
        m_shouldHgStat = true;
        m_justMerged = true;
        break;
        
    case ACT_RETRY_MERGE:
        QMessageBox::information(this, tr("Resolved"),
                                 tr("<qt><h3>Merge resolved</h3><p>Merge resolved successfully.<br>Remember to test and commit the result before making any further changes.</p>"));
        m_shouldHgStat = true;
        m_justMerged = true;
        break;
        
    default:
        break;
    }

    // Sequence when no full log required:
    //   paths -> branch -> stat -> resolve-list -> heads ->
    //     incremental-log (only if heads changed) -> parents
    // 
    // Sequence when full log required:
    //   paths -> branch -> stat -> resolve-list -> heads -> parents -> log
    //
    // Note we want to call enableDisableActions only once, at the end
    // of whichever sequence is in use.

    bool noMore = false;

    switch (action) {

    case ACT_TEST_HG:
    {
        QSettings settings;
        settings.beginGroup("General");
        if (settings.value("useextension", true).toBool()) {
            hgTestExtension();
        } else if (m_workFolderPath == "") {
            open();
        } else {
            hgQueryPaths();
        }
        break;
    }
        
    case ACT_TEST_HG_EXT:
        if (m_workFolderPath == "") {
            open();
        } else{
            hgQueryPaths();
        }
        break;
        
    case ACT_QUERY_PATHS:
        hgQueryBranch();
        break;

    case ACT_QUERY_BRANCH:
        hgStat();
        break;
        
    case ACT_STAT:
        hgResolveList();
        break;

    case ACT_RESOLVE_LIST:
        hgQueryHeads();
        break;

    case ACT_QUERY_HEADS:
        if (headsChanged && !m_needNewLog) {
            hgLogIncremental(oldHeadIds);
        } else {
            hgQueryParents();
        }
        break;

    case ACT_LOG_INCREMENTAL:
        hgQueryParents();
        break;

    case ACT_QUERY_PARENTS:
        if (m_needNewLog) {
            hgLog();
        } else {
            // we're done
            noMore = true;
        }
        break;

    case ACT_LOG:
        // we're done
        noMore = true;
        break;

    default:
        if (m_shouldHgStat) {
            m_shouldHgStat = false;
            hgQueryPaths();
        } else {
            noMore = true;
        }
        break;
    }

    if (noMore) {
        m_stateUnknown = false;
        enableDisableActions();
        m_hgTabs->updateHistory();
    }
}

void MainWindow::connectActions()
{
    connect(m_exitAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(m_aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    connect(m_hgRefreshAct, SIGNAL(triggered()), this, SLOT(hgRefresh()));
    connect(m_hgRemoveAct, SIGNAL(triggered()), this, SLOT(hgRemove()));
    connect(m_hgAddAct, SIGNAL(triggered()), this, SLOT(hgAdd()));
    connect(m_hgCommitAct, SIGNAL(triggered()), this, SLOT(hgCommit()));
    connect(m_hgFolderDiffAct, SIGNAL(triggered()), this, SLOT(hgFolderDiff()));
    connect(m_hgUpdateAct, SIGNAL(triggered()), this, SLOT(hgUpdate()));
    connect(m_hgRevertAct, SIGNAL(triggered()), this, SLOT(hgRevert()));
    connect(m_hgMergeAct, SIGNAL(triggered()), this, SLOT(hgMerge()));
    connect(m_hgIgnoreAct, SIGNAL(triggered()), this, SLOT(hgIgnore()));

    connect(m_settingsAct, SIGNAL(triggered()), this, SLOT(settings()));
    connect(m_openAct, SIGNAL(triggered()), this, SLOT(open()));
    connect(m_changeRemoteRepoAct, SIGNAL(triggered()), this, SLOT(changeRemoteRepo()));

    connect(m_hgIncomingAct, SIGNAL(triggered()), this, SLOT(hgIncoming()));
    connect(m_hgPullAct, SIGNAL(triggered()), this, SLOT(hgPull()));
    connect(m_hgPushAct, SIGNAL(triggered()), this, SLOT(hgPush()));

    connect(m_hgAnnotateAct, SIGNAL(triggered()), this, SLOT(hgAnnotate()));
    connect(m_hgServeAct, SIGNAL(triggered()), this, SLOT(hgServe()));
}

void MainWindow::connectTabsSignals()
{
    connect(m_hgTabs, SIGNAL(commit()),
            this, SLOT(hgCommit()));
    
    connect(m_hgTabs, SIGNAL(revert()),
            this, SLOT(hgRevert()));
    
    connect(m_hgTabs, SIGNAL(diffWorkingFolder()),
            this, SLOT(hgFolderDiff()));
    
    connect(m_hgTabs, SIGNAL(showSummary()),
            this, SLOT(hgShowSummary()));

    connect(m_hgTabs, SIGNAL(updateTo(QString)),
            this, SLOT(hgUpdateToRev(QString)));

    connect(m_hgTabs, SIGNAL(diffToCurrent(QString)),
            this, SLOT(hgDiffToCurrent(QString)));

    connect(m_hgTabs, SIGNAL(diffToParent(QString, QString)),
            this, SLOT(hgDiffToParent(QString, QString)));

    connect(m_hgTabs, SIGNAL(showSummary(Changeset *)),
            this, SLOT(hgShowSummaryFor(Changeset *)));

    connect(m_hgTabs, SIGNAL(mergeFrom(QString)),
            this, SLOT(hgMergeFrom(QString)));

    connect(m_hgTabs, SIGNAL(newBranch(QString)),
            this, SLOT(hgNewBranch(QString)));

    connect(m_hgTabs, SIGNAL(tag(QString)),
            this, SLOT(hgTag(QString)));
}    

void MainWindow::enableDisableActions()
{
    DEBUG << "MainWindow::enableDisableActions" << endl;

    QString dirname = QDir(m_workFolderPath).dirName();
    if (dirname != "") {
        setWindowTitle(tr("EasyMercurial: %1").arg(dirname));
    } else {
        setWindowTitle(tr("EasyMercurial"));
    }

    //!!! should also do things like set the status texts for the
    //!!! actions appropriately by context

    QDir localRepoDir;
    QDir workFolderDir;
    bool workFolderExist = true;
    bool localRepoExist = true;

    m_remoteRepoActionsEnabled = true;
    if (m_remoteRepoPath.isEmpty()) {
        m_remoteRepoActionsEnabled = false;
    }

    m_localRepoActionsEnabled = true;
    if (m_workFolderPath.isEmpty()) {
        m_localRepoActionsEnabled = false;
        workFolderExist = false;
    }

    if (m_workFolderPath == "" || !workFolderDir.exists(m_workFolderPath)) {
        m_localRepoActionsEnabled = false;
        workFolderExist = false;
    } else {
        workFolderExist = true;
    }

    if (!localRepoDir.exists(m_workFolderPath + "/.hg")) {
        m_localRepoActionsEnabled = false;
        localRepoExist = false;
    }

    m_hgIncomingAct -> setEnabled(m_remoteRepoActionsEnabled && m_remoteRepoActionsEnabled);
    m_hgPullAct -> setEnabled(m_remoteRepoActionsEnabled && m_remoteRepoActionsEnabled);
    m_hgPushAct -> setEnabled(m_remoteRepoActionsEnabled && m_remoteRepoActionsEnabled);

    bool haveDiff = false;
    QSettings settings;
    settings.beginGroup("Locations");
    if (settings.value("extdiffbinary", "").toString() != "") {
        haveDiff = true;
    }
    settings.endGroup();

    m_hgRefreshAct -> setEnabled(m_localRepoActionsEnabled);
    m_hgFolderDiffAct -> setEnabled(m_localRepoActionsEnabled && haveDiff);
    m_hgRevertAct -> setEnabled(m_localRepoActionsEnabled);
    m_hgAddAct -> setEnabled(m_localRepoActionsEnabled);
    m_hgRemoveAct -> setEnabled(m_localRepoActionsEnabled);
    m_hgUpdateAct -> setEnabled(m_localRepoActionsEnabled);
    m_hgCommitAct -> setEnabled(m_localRepoActionsEnabled);
    m_hgMergeAct -> setEnabled(m_localRepoActionsEnabled);
    m_hgAnnotateAct -> setEnabled(m_localRepoActionsEnabled);
    m_hgServeAct -> setEnabled(m_localRepoActionsEnabled);
    m_hgIgnoreAct -> setEnabled(m_localRepoActionsEnabled);

    DEBUG << "m_localRepoActionsEnabled = " << m_localRepoActionsEnabled << endl;
    DEBUG << "canCommit = " << m_hgTabs->canCommit() << endl;

    m_hgAddAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canAdd());
    m_hgRemoveAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canRemove());
    m_hgCommitAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canCommit());
    m_hgRevertAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canRevert());
    m_hgFolderDiffAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canDiff());

    // A default merge makes sense if:
    //  * there is only one parent (if there are two, we have an uncommitted merge) and
    //  * there are exactly two heads that have the same branch as the current branch and
    //  * our parent is one of those heads
    //
    // A default update makes sense if:
    //  * there is only one parent and
    //  * the parent is not one of the current heads

    bool canMerge = false;
    bool canUpdate = false;
    bool haveMerge = false;
    bool emptyRepo = false;
    bool noWorkingCopy = false;
    bool newBranch = false;
    int m_currentBranchHeads = 0;

    if (m_currentParents.size() == 1) {
        bool parentIsHead = false;
        Changeset *parent = m_currentParents[0];
        foreach (Changeset *head, m_currentHeads) {
            DEBUG << "head branch " << head->branch() << ", current branch " << m_currentBranch << endl;
            if (head->isOnBranch(m_currentBranch)) {
                ++m_currentBranchHeads;
            }
            if (parent->id() == head->id()) {
                parentIsHead = true;
            }
        }
        if (m_currentBranchHeads == 2 && parentIsHead) {
            canMerge = true;
        }
        if (m_currentBranchHeads == 0 && parentIsHead) {
            // Just created a new branch
            newBranch = true;
        }
        if (!parentIsHead) {
            canUpdate = true;
            DEBUG << "parent id = " << parent->id() << endl;
            DEBUG << " head ids "<<endl;
            foreach (Changeset *h, m_currentHeads) {
                DEBUG << "head id = " << h->id() << endl;
            }
        }
        m_justMerged = false;
    } else if (m_currentParents.size() == 0) {
        if (m_currentHeads.size() == 0) {
            // No heads -> empty repo
            emptyRepo = true;
        } else {
            // Heads, but no parents -> no working copy, e.g. we have
            // just converted this repo but haven't updated in it yet.
            // Uncommon but confusing; probably merits a special case
            noWorkingCopy = true;
            canUpdate = true;
        }
        m_justMerged = false;
    } else {
        haveMerge = true;
        m_justMerged = true;
    }
        
    m_hgMergeAct->setEnabled(m_localRepoActionsEnabled &&
                           (canMerge || m_hgTabs->canResolve()));
    m_hgUpdateAct->setEnabled(m_localRepoActionsEnabled &&
                            (canUpdate && !m_hgTabs->haveChangesToCommit()));

    // Set the state field on the file status widget

    QString branchText;
    if (m_currentBranch == "" || m_currentBranch == "default") {
        branchText = tr("the default branch");
    } else {
        branchText = tr("branch \"%1\"").arg(m_currentBranch);
    }

    if (m_stateUnknown) {
        if (m_workFolderPath == "") {
            m_workStatus->setState(tr("No repository open"));
        } else {
            m_workStatus->setState(tr("(Examining repository)"));
        }
    } else if (emptyRepo) {
        m_workStatus->setState(tr("Nothing committed to this repository yet"));
    } else if (noWorkingCopy) {
        m_workStatus->setState(tr("No working copy yet: consider updating"));
    } else if (canMerge) {
        m_workStatus->setState(tr("<b>Awaiting merge</b> on %1").arg(branchText));
    } else if (!m_hgTabs->getAllUnresolvedFiles().empty()) {
        m_workStatus->setState(tr("Have unresolved files following merge on %1").arg(branchText));
    } else if (haveMerge) {
        m_workStatus->setState(tr("Have merged but not yet committed on %1").arg(branchText));
    } else if (newBranch) {
        m_workStatus->setState(tr("On %1.  New branch: has not yet been committed").arg(branchText));
    } else if (canUpdate) {
        if (m_hgTabs->haveChangesToCommit()) {
            // have uncommitted changes
            m_workStatus->setState(tr("On %1. Not at the head of the branch").arg(branchText));
        } else {
            // no uncommitted changes
            m_workStatus->setState(tr("On %1. Not at the head of the branch: consider updating").arg(branchText));
        }
    } else if (m_currentBranchHeads > 1) {
        m_workStatus->setState(tr("At one of %n heads of %1", "", m_currentBranchHeads).arg(branchText));
    } else {
        m_workStatus->setState(tr("At the head of %1").arg(branchText));
    }
}

void MainWindow::createActions()
{
    //File actions
    m_openAct = new QAction(QIcon(":/images/fileopen.png"), tr("Open..."), this);
    m_openAct -> setStatusTip(tr("Open an existing repository or working folder"));

    m_changeRemoteRepoAct = new QAction(tr("Change Remote Location..."), this);
    m_changeRemoteRepoAct->setStatusTip(tr("Change the default remote repository for pull and push actions"));

    m_settingsAct = new QAction(QIcon(":/images/settings.png"), tr("Settings..."), this);
    m_settingsAct -> setStatusTip(tr("View and change application settings"));

    m_exitAct = new QAction(QIcon(":/images/exit.png"), tr("Quit"), this);
    m_exitAct->setShortcuts(QKeySequence::Quit);
    m_exitAct->setStatusTip(tr("Quit EasyMercurial"));

    //Repository actions
    m_hgRefreshAct = new QAction(QIcon(":/images/status.png"), tr("Refresh"), this);
    m_hgRefreshAct->setStatusTip(tr("Refresh the window to show the current state of the working folder"));

    m_hgIncomingAct = new QAction(QIcon(":/images/incoming.png"), tr("Preview"), this);
    m_hgIncomingAct -> setStatusTip(tr("See what changes are available in the remote repository waiting to be pulled"));

    m_hgPullAct = new QAction(QIcon(":/images/pull.png"), tr("Pull"), this);
    m_hgPullAct -> setStatusTip(tr("Pull changes from the remote repository to the local repository"));

    m_hgPushAct = new QAction(QIcon(":/images/push.png"), tr("Push"), this);
    m_hgPushAct->setStatusTip(tr("Push changes from the local repository to the remote repository"));

    //Workfolder actions
    m_hgFolderDiffAct   = new QAction(QIcon(":/images/folderdiff.png"), tr("Diff"), this);
    m_hgFolderDiffAct->setStatusTip(tr("See what has changed in the working folder compared with the last committed state"));

    m_hgRevertAct = new QAction(QIcon(":/images/undo.png"), tr("Revert"), this);
    m_hgRevertAct->setStatusTip(tr("Throw away your changes and return to the last committed state"));

    m_hgAddAct = new QAction(QIcon(":/images/add.png"), tr("Add"), this);
    m_hgAddAct -> setStatusTip(tr("Mark the selected file(s) to be added on the next commit"));

    //!!! needs to be modified for number
    m_hgRemoveAct = new QAction(QIcon(":/images/remove.png"), tr("Remove"), this);
    m_hgRemoveAct -> setStatusTip(tr("Mark the selected file(s) to be removed from version control on the next commit"));

    m_hgUpdateAct = new QAction(QIcon(":/images/update.png"), tr("Update"), this);
    m_hgUpdateAct->setStatusTip(tr("Update the working folder to the head of the current repository branch"));

    //!!! needs to be modified when files selected
    m_hgCommitAct = new QAction(QIcon(":/images/commit.png"), tr("Commit"), this);
    m_hgCommitAct->setStatusTip(tr("Commit your changes to the local repository"));

    m_hgMergeAct = new QAction(QIcon(":/images/merge.png"), tr("Merge"), this);
    m_hgMergeAct->setStatusTip(tr("Merge the two independent sets of changes in the local repository into the working folder"));

    //Advanced actions
    //!!! needs to be modified for number
    m_hgAnnotateAct = new QAction(tr("Annotate"), this);
    m_hgAnnotateAct -> setStatusTip(tr("Show line-by-line version information for selected file"));

    m_hgIgnoreAct = new QAction(tr("Edit .hgignore File"), this);
    m_hgIgnoreAct -> setStatusTip(tr("Edit the .hgignore file, containing the names of files that should be ignored by Mercurial"));

    m_hgServeAct = new QAction(tr("Serve via HTTP"), this);
    m_hgServeAct -> setStatusTip(tr("Serve local repository via http for workgroup access"));

    //Help actions
    m_aboutAct = new QAction(tr("About EasyMercurial"), this);

    // Miscellaneous
    QShortcut *clearSelectionsShortcut = new QShortcut(Qt::Key_Escape, this);
    connect(clearSelectionsShortcut, SIGNAL(activated()),
            this, SLOT(clearSelections()));
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu(tr("File"));

    m_fileMenu -> addAction(m_openAct);
    m_fileMenu -> addAction(m_changeRemoteRepoAct);
    m_fileMenu -> addSeparator();

    m_advancedMenu = m_fileMenu->addMenu(tr("Advanced"));

    m_fileMenu -> addAction(m_settingsAct);

    m_fileMenu -> addSeparator();
    m_fileMenu -> addAction(m_exitAct);

    m_advancedMenu -> addAction(m_hgIgnoreAct);
    m_advancedMenu -> addSeparator();
    m_advancedMenu -> addAction(m_hgServeAct);

    m_helpMenu = menuBar()->addMenu(tr("Help"));
    m_helpMenu->addAction(m_aboutAct);
}

void MainWindow::createToolBars()
{
    m_fileToolBar = addToolBar(tr("File"));
    m_fileToolBar -> setIconSize(QSize(MY_ICON_SIZE, MY_ICON_SIZE));
    m_fileToolBar -> addAction(m_openAct);
    m_fileToolBar -> addAction(m_hgRefreshAct);
    m_fileToolBar -> addSeparator();
    m_fileToolBar -> setMovable(false);

    m_repoToolBar = addToolBar(tr(REPOMENU_TITLE));
    m_repoToolBar -> setIconSize(QSize(MY_ICON_SIZE, MY_ICON_SIZE));
    m_repoToolBar->addAction(m_hgIncomingAct);
    m_repoToolBar->addAction(m_hgPullAct);
    m_repoToolBar->addAction(m_hgPushAct);
    m_repoToolBar -> setMovable(false);

    m_workFolderToolBar = addToolBar(tr(WORKFOLDERMENU_TITLE));
    addToolBar(Qt::LeftToolBarArea, m_workFolderToolBar);
    m_workFolderToolBar -> setIconSize(QSize(MY_ICON_SIZE, MY_ICON_SIZE));
    m_workFolderToolBar->addAction(m_hgFolderDiffAct);
    m_workFolderToolBar->addSeparator();
    m_workFolderToolBar->addAction(m_hgRevertAct);
    m_workFolderToolBar->addAction(m_hgUpdateAct);
    m_workFolderToolBar->addAction(m_hgCommitAct);
    m_workFolderToolBar->addAction(m_hgMergeAct);
    m_workFolderToolBar->addSeparator();
    m_workFolderToolBar->addAction(m_hgAddAct);
    m_workFolderToolBar->addAction(m_hgRemoveAct);
    m_workFolderToolBar -> setMovable(false);

    updateToolBarStyle();
}

void MainWindow::updateToolBarStyle()
{
    QSettings settings;
    settings.beginGroup("Presentation");
    bool showText = settings.value("showiconlabels", true).toBool();
    settings.endGroup();
    
    foreach (QToolButton *tb, findChildren<QToolButton *>()) {
        tb->setToolButtonStyle(showText ?
                               Qt::ToolButtonTextUnderIcon :
                               Qt::ToolButtonIconOnly);
    }
}    

void MainWindow::updateWorkFolderAndRepoNames()
{
    m_hgTabs->setLocalPath(m_workFolderPath);

    m_workStatus->setLocalPath(m_workFolderPath);
    m_workStatus->setRemoteURL(m_remoteRepoPath);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QDir workFolder;

    QSettings settings;

    m_remoteRepoPath = settings.value("remoterepopath", "").toString();
    m_workFolderPath = settings.value("workfolderpath", "").toString();
    if (!workFolder.exists(m_workFolderPath))
    {
        m_workFolderPath = "";
    }

    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    m_firstStart = settings.value("firststart", QVariant(true)).toBool();

    resize(size);
    move(pos);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    settings.setValue("remoterepopath", m_remoteRepoPath);
    settings.setValue("workfolderpath", m_workFolderPath);
    settings.setValue("firststart", m_firstStart);
}




