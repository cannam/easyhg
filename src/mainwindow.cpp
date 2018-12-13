 /* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2013 Chris Cannam
    Copyright (c) 2013 Queen Mary, University of London
    
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
#include <QWidgetAction>
#include <QRegExp>
#include <QShortcut>
#include <QUrl>
#include <QDialogButtonBox>
#include <QTimer>
#include <QTextBrowser>

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
#include "annotatedialog.h"
#include "version.h"
#include "workstatuswidget.h"
#include "hgignoredialog.h"
#include "versiontester.h"
#include "fswatcher.h"


MainWindow::MainWindow(QString myDirPath) :
    m_myDirPath(myDirPath),
    m_helpDialog(0)
{
    setWindowIcon(QIcon(":images/easyhg-icon.png"));

    QString wndTitle;

    m_showAllFiles = false;

    m_fsWatcher = new FsWatcher();
    m_fsWatcherToken = m_fsWatcher->getNewToken();
    m_commandSequenceInProgress = false;
    connect(m_fsWatcher, SIGNAL(changed()), this, SLOT(checkFilesystem()));

    m_commitsSincePush = 0;
    m_shouldHgStat = true;

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    m_runner = new HgRunner(m_myDirPath, this);
    connect(m_runner, SIGNAL(commandStarting(HgAction)),
            this, SLOT(commandStarting(HgAction)));
    connect(m_runner, SIGNAL(commandCompleted(HgAction, QString, QString)),
            this, SLOT(commandCompleted(HgAction, QString, QString)));
    connect(m_runner, SIGNAL(commandFailed(HgAction, QString, QString)),
            this, SLOT(commandFailed(HgAction, QString, QString)));
    connect(m_runner, SIGNAL(commandCancelled(HgAction)),
            this, SLOT(commandCancelled(HgAction)));
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

    cl->addWidget(m_hgTabs, row++, 0, 1, 2);

    connect(m_hgTabs, SIGNAL(selectionChanged()),
            this, SLOT(enableDisableActions()));
    connect(m_hgTabs, SIGNAL(showAllChanged()),
            this, SLOT(showAllChanged()));

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

    VersionTester *vt = new VersionTester
        ("easyhg.org", "/latest-version.txt", EASYHG_VERSION);
    connect(vt, SIGNAL(newerVersionAvailable(QString)),
            this, SLOT(newerVersionAvailable(QString)));

    hgTest();
    updateRecentMenu();
}


void MainWindow::closeEvent(QCloseEvent *)
{
    writeSettings();
    delete m_fsWatcher;
}


void MainWindow::resizeEvent(QResizeEvent *)
{
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
    QString text;
    text += "<qt>";

    text += tr("<h2>EasyMercurial v%1</h2>").arg(EASYHG_VERSION);

#ifdef Q_OS_MAC
    text += "<font size=-1>";
#endif
    text += tr("<p>EasyMercurial is a simple user interface for the "
               "Mercurial</a> version control system.</p>");
    
    text += tr("<h4>Credits and Copyright</h4>");

    text += tr("<p>Development carried out by Chris Cannam for "
               "SoundSoftware.ac.uk at the Centre for Digital Music, "
               "Queen Mary, University of London.</p>");

    text += tr("<p>EasyMercurial is based on HgExplorer by "
               "Jari Korhonen, with thanks.</p>");
    
    text += tr("<p style=\"margin-left: 2em;\">");
    text += tr("Copyright &copy; 2013-2018 Queen Mary, University of London.<br>");
    text += tr("Copyright &copy; 2010 Jari Korhonen.<br>");
    text += tr("Copyright &copy; 2013 Chris Cannam.");
    text += tr("</p>");
    
    text += tr("<p style=\"margin-left: 2em;\">");
    text += tr("This program requires Mercurial, by Matt Mackall and others.<br>");
    text += tr("This program uses Qt by The Qt Company.<br>");
    text += tr("This program uses Nuvola icons by David Vignoni.<br>");
    text += tr("This program may use KDiff3 by Joachim Eibl.<br>");
    text += tr("This program may use PyQt by River Bank Computing.<br>");
    text += tr("Packaging for Mercurial and other dependencies on Windows is derived from TortoiseHg by Steve Borho and others.");
    text += tr("</p>");
    
    text += tr("<h4>License</h4>");
    text += tr("<p>This program is free software; you can redistribute it and/or "
               "modify it under the terms of the GNU General Public License as "
               "published by the Free Software Foundation; either version 2 of the "
               "License, or (at your option) any later version.  See the file "
               "COPYING included with this distribution for more information.</p>");
#ifdef Q_OS_MAC
    text += "</font>";
#endif

    // use our own dialog so we can influence the size

    QDialog *d = new QDialog(this);
    d->setWindowTitle(tr("About %1").arg(QApplication::applicationName()));
        
    QGridLayout *layout = new QGridLayout;
    d->setLayout(layout);

    int row = 0;
    
    QLabel *iconLabel = new QLabel;
    iconLabel->setPixmap(QApplication::windowIcon().pixmap(64, 64));
    layout->addWidget(iconLabel, row, 0, Qt::AlignTop);

    QLabel *mainText = new QLabel();
    layout->addWidget(mainText, row, 1, 1, 2);

    layout->setRowStretch(row, 10);
    layout->setColumnStretch(1, 10);

    ++row;

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok);
    layout->addWidget(bb, row++, 0, 1, 3);
    connect(bb, SIGNAL(accepted()), d, SLOT(accept()));

    mainText->setWordWrap(true);
    mainText->setOpenExternalLinks(true);
    mainText->setText(text);

    d->setMinimumSize(400, 400);
    d->exec();

    delete d;    
}

void MainWindow::clearSelections()
{
    m_hgTabs->clearSelections();
}

void MainWindow::showAllChanged()
{
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
    params << "--version";
    // The path is not necessarily set here, but we need something for
    // this test or else HgRunner will (cautiously) refuse to run
    QString path = m_myDirPath;
    if (path == "") path = QDir::homePath();
    m_runner->requestAction(HgAction(ACT_TEST_HG, path, params));
}

void MainWindow::hgTestExtension()
{
    QStringList params;
    params << "--version";
    // The path is not necessarily set here, but we need something for
    // this test or else HgRunner will (cautiously) refuse to run
    QString path = m_myDirPath;
    if (path == "") path = QDir::homePath();
    m_runner->requestAction(HgAction(ACT_TEST_HG_EXT, path, params));
}

void MainWindow::hgStat()
{
    QStringList params;

    // We always stat all files, regardless of whether we're showing
    // them all, because we need them for the filesystem monitor
    params << "stat" << "-A";

    m_lastStatOutput = "";

    // We're about to do a stat, so we can silently bring ourselves
    // up-to-date on any file changes to this point
    (void)m_fsWatcher->getChangedPaths(m_fsWatcherToken);

    m_runner->requestAction(HgAction(ACT_STAT, m_workFolderPath, params));
}

void MainWindow::hgQueryPaths()
{
    m_showAllFiles = m_hgTabs->shouldShowAll();

    // Quickest is to just read the file

    QFileInfo hgrc(m_workFolderPath + "/.hg/hgrc");

    QString path;

    if (hgrc.exists()) {
        QSettings s(hgrc.canonicalFilePath(), QSettings::IniFormat);
        s.beginGroup("paths");
        path = s.value("default").toString();
    }

//    std::cerr << "hgQueryPaths: setting m_remoteRepoPath to " << m_remoteRepoPath << " from file " << hgrc.absoluteFilePath() << std::endl;
    
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

void MainWindow::hgQueryHeadsActive()
{
    QStringList params;
    params << "heads";
    m_runner->requestAction(HgAction(ACT_QUERY_HEADS_ACTIVE, m_workFolderPath, params));
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
    QSettings settings;
    settings.beginGroup("Presentation");

    QStringList params;
    params << "log";
    params << "--date";
    params << settings.value("datefrom", QDate(2000, 1, 1)).toDate().toString("yyyy-MM-dd") + " to " + QDate::currentDate().toString("yyyy-MM-dd");
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

void MainWindow::hgAnnotateFiles(QStringList files)
{
    QStringList params;
    
    if (!files.isEmpty()) {
        params << "annotate" << "-udqc" << "--" << files;
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
    // hgExplorer permitted adding "all" files -- I'm not sure
    // that one is a good idea, let's require the user to select

    hgAddFiles(m_hgTabs->getSelectedAddableFiles());
}

void MainWindow::hgAddFiles(QStringList files)
{
    QStringList params;

    if (!files.empty()) {
        params << "add" << "--" << files;
        m_runner->requestAction(HgAction(ACT_ADD, m_workFolderPath, params));
    }
}

void MainWindow::hgRemove()
{
    hgRemoveFiles(m_hgTabs->getSelectedRemovableFiles());
}

void MainWindow::hgRemoveFiles(QStringList files)
{
    QStringList params;

    if (!files.empty()) {
        params << "remove" << "--after" << "--force" << "--" << files;
        m_runner->requestAction(HgAction(ACT_REMOVE, m_workFolderPath, params));
    }
}

void MainWindow::hgCommit()
{
    hgCommitFiles(QStringList());
}

void MainWindow::hgCommitFiles(QStringList files)
{
    QStringList params;
    QString comment;

    if (m_justMerged) {
        comment = m_mergeCommitComment;
    }

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

    QString branchText;
    if (m_currentBranch == "" || m_currentBranch == "default") {
        branchText = tr("the default branch");
    } else {
        branchText = tr("branch \"%1\"").arg(m_currentBranch);
    }

    if (ConfirmCommentDialog::confirmAndGetLongComment
        (this,
         cf,
         tr("<h3>%1</h3><p>%2%3").arg(cf)
         .arg(tr("You are about to commit changes to the following files in %1:").arg(branchText))
         .arg(subsetNote),
         tr("<h3>%1</h3><p>%2%3").arg(cf)
         .arg(tr("You are about to commit changes to %n file(s) in %1.", "", reportFiles.size()).arg(branchText))
         .arg(subsetNote),
         reportFiles,
         comment,
         tr("Co&mmit"))) {

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


void MainWindow::hgNewBranch()
{
    QStringList params;
    QString branch;

    if (ConfirmCommentDialog::confirmAndGetShortComment
        (this,
         tr("New Branch"),
         tr("Enter new branch name:"),
         branch,
         tr("Start &Branch"))) {
        if (!branch.isEmpty()) {//!!! do something better if it is empty

            params << "branch" << filterTag(branch);
            m_runner->requestAction(HgAction(ACT_NEW_BRANCH, m_workFolderPath, params));
        }
    }
}

void MainWindow::hgNoBranch()
{
    if (m_currentParents.empty()) return;

    QString parentBranch = m_currentParents[0]->branch();
    if (parentBranch == "") parentBranch = "default";

    QStringList params;
    params << "branch" << parentBranch;
    m_runner->requestAction(HgAction(ACT_NEW_BRANCH, m_workFolderPath, params));
}

void MainWindow::hgCloseBranch()
{
    QStringList params;

    //!!! how to ensure this doesn't happen when uncommitted changes present?

    QString cf(tr("Close branch"));
    QString comment;

    QString defaultWarning;

    bool haveSprouts = (m_hgMergeAct->isEnabled());

    QString branchText;
    if (m_currentBranch == "" || m_currentBranch == "default") {
        branchText = tr("the default branch");
        if (!haveSprouts) {
            defaultWarning = tr("<p><b>Warning:</b> you are asking to close the default branch. This is not usually a good idea!</p>");
        }
    } else {
        branchText = tr("branch \"%1\"").arg(m_currentBranch);
    }

    if (haveSprouts) {
        branchText = tr("a sub-branch of %1").arg(branchText);
    }

    if (ConfirmCommentDialog::confirmAndGetLongComment
        (this,
         cf,
         tr("<h3>%1</h3><p>%2%3").arg(cf)
         .arg(tr("You are about to close %1.<p>This branch will be marked as closed and hidden from the history view.<p>You will still be able to see if it you select \"Show closed branches\" in the history view, and it will be reopened if you commit to it.<p>Please enter your comment for the commit log:").arg(branchText))
         .arg(defaultWarning),
         comment,
         tr("C&lose branch"))) {

        params << "commit" << "--message" << comment
               << "--user" << getUserInfo() << "--close-branch";
        
        m_runner->requestAction(HgAction(ACT_CLOSE_BRANCH, m_workFolderPath, params));
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
         tr("Add &Tag"))) {
        if (!tag.isEmpty()) {//!!! do something better if it is empty

            params << "tag" << "--user" << getUserInfo();
            params << "--rev" << Changeset::hashOf(id) << filterTag(tag);
            
            m_runner->requestAction(HgAction(ACT_TAG, m_workFolderPath, params));
        }
    }
}

void MainWindow::initHgIgnore()
{
    if (!QDir(m_workFolderPath).exists()) return;
    QString hgIgnorePath = m_workFolderPath + "/.hgignore";

    QFile f(hgIgnorePath);
    if (!f.exists()) {
        f.open(QFile::WriteOnly);
        QTextStream *ts = new QTextStream(&f);
        *ts << "syntax: glob\n";
        delete ts;
        f.close();
    }
}    

void MainWindow::hgIgnore()
{
    // hgExplorer permitted adding "all" files -- I'm not sure
    // that one is a good idea, let's require the user to select

    hgIgnoreFiles(m_hgTabs->getSelectedAddableFiles());
}

void MainWindow::hgEditIgnore()
{
    if (!QDir(m_workFolderPath).exists()) return;

    initHgIgnore();

    QString hgIgnorePath = m_workFolderPath + "/.hgignore";

    QFile f(hgIgnorePath);
    if (!f.exists()) return; // shouldn't happen (after initHgIgnore called)

    if (!f.open(QFile::ReadOnly)) return;
    QTextStream sin(&f);
    QString all = sin.readAll();
    f.close();

    QDialog d;
    QGridLayout layout;
    d.setLayout(&layout);

    int row = 0;
    layout.addWidget(new QLabel(tr("<qt><h3>Ignored File Patterns</h3></qt>")), row++, 0);//!!! todo: link to Hg docs?

    QTextEdit ed;
    ed.setAcceptRichText(false);
    ed.setLineWrapMode(QTextEdit::NoWrap);
    layout.setRowStretch(row, 10);
    layout.addWidget(&ed, row++, 0);
    
    QDialogButtonBox bb(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(bb.button(QDialogButtonBox::Save), SIGNAL(clicked()),
            &d, SLOT(accept()));
    connect(bb.button(QDialogButtonBox::Cancel), SIGNAL(clicked()),
            &d, SLOT(reject()));
    layout.addWidget(&bb, row++, 0);

    ed.document()->setPlainText(all);

    d.resize(QSize(300, 400));

    if (d.exec() == QDialog::Accepted) {
        if (!f.open(QFile::WriteOnly | QFile::Truncate)) {
            QMessageBox::critical(this, tr("Write failed"),
                                  tr("Failed to open file %1 for writing")
                                  .arg(f.fileName()));
            return;
        }
        QTextStream sout(&f);
        sout << ed.document()->toPlainText();
        f.close();
    }
}

static QString regexEscape(QString filename)
{
    return filename
        .replace(".", "\\.")
        .replace("[", "\\[")
        .replace("]", "\\]")
        .replace("(", "\\(")
        .replace(")", "\\)")
        .replace("?", "\\?");
}

void MainWindow::hgIgnoreFiles(QStringList files)
{
    if (!QDir(m_workFolderPath).exists() || files.empty()) return;

    // we should:
    //
    // * show the user the list of file names selected
    // 
    // * offer a choice (depending on the files selected?)
    // 
    //   - ignore only these files
    //
    //   - ignore files with these names, in any subdirectories?
    //
    //   - ignore all files with this extension (if they have a common
    //     extension)?
    //   
    //   - ignore all files with these extensions (if they have any
    //     extensions?)

    DEBUG << "MainWindow::hgIgnoreFiles: File names are:" << endl;
    foreach (QString file, files) DEBUG << file << endl;

    QSet<QString> suffixes;
    foreach (QString file, files) {
        QString s = QFileInfo(file).suffix();
        if (s != "") suffixes.insert(s);
    }

    QString directory;
    int dirCount = 0;
    foreach (QString file, files) {
        QString d = QFileInfo(file).path();
        if (d != directory) {
            ++dirCount;
            directory = d;
        }
    }
    if (dirCount != 1 || directory == ".") directory = "";

    HgIgnoreDialog::IgnoreType itype =
        HgIgnoreDialog::confirmIgnore
        (this, files, QStringList::fromSet(suffixes), directory);

    DEBUG << "hgIgnoreFiles: Ignore type is " << itype << endl;

    if (itype == HgIgnoreDialog::IgnoreNothing) return;

    // Now, .hgignore can be switched from regex to glob syntax
    // part-way through -- and glob is much simpler for us, so we
    // should do that if it's in regex mode at the end of the file.

    initHgIgnore();

    QString hgIgnorePath = m_workFolderPath + "/.hgignore";

    // hgignore file should now exist (initHgIgnore should have
    // created it if it didn't).  Check for glob status first

    QFile f(hgIgnorePath);
    if (!f.exists()) {
        std::cerr << "MainWindow::ignoreFiles: Internal error: .hgignore file not found (even though we were supposed to have created it)" << std::endl;
        return;
    }

    f.open(QFile::ReadOnly);
    bool glob = false;
    bool cr = false; // whether the last line examined ended with a CR
    while (!f.atEnd()) {
        QByteArray ba = f.readLine();
        QString s = QString::fromLocal8Bit(ba);
        cr = (s.endsWith('\n') || s.endsWith('\r'));
        s = s.trimmed();
        if (s.startsWith("syntax:")) {
            if (s.endsWith("glob")) {
                glob = true;
            } else {
                glob = false;
            }
        }
    }
    f.close();

    f.open(QFile::Append);
    QTextStream out(&f);

    if (!cr) {
        out << endl;
    }

    if (!glob) {
        out << "syntax: glob" << endl;
    }

    QString info = "<qt><h3>" + tr("Ignored files") + "</h3><p>";
    info += tr("The following lines have been added to the .hgignore file for this working copy:");
    info += "</p><code>";

    QStringList args;
    if (itype == HgIgnoreDialog::IgnoreAllFilesOfGivenSuffixes) {
        args = QStringList::fromSet(suffixes);
    } else if (itype == HgIgnoreDialog::IgnoreGivenFilesOnly) {
        args = files;
    } else if (itype == HgIgnoreDialog::IgnoreAllFilesOfGivenNames) {
        QSet<QString> names;
        foreach (QString f, files) {
            names << QFileInfo(f).fileName();
        }
        args = QStringList::fromSet(names);
    } else if (itype == HgIgnoreDialog::IgnoreWholeDirectory) {
        args << directory;
    }

    bool first = true;

    foreach (QString a, args) {
        QString line;
        if (itype == HgIgnoreDialog::IgnoreAllFilesOfGivenSuffixes) {
            line = "*." + a;
        } else if (itype == HgIgnoreDialog::IgnoreGivenFilesOnly) {
            // Doesn't seem to be possible to do this with a glob,
            // because the glob is always unanchored and there is no
            // equivalent of ^ to anchor it
            line = "re:^" + regexEscape(a) + "$";
        } else if (itype == HgIgnoreDialog::IgnoreAllFilesOfGivenNames) {
            line = a;
        } else if (itype == HgIgnoreDialog::IgnoreWholeDirectory) {
            line = "re:^" + regexEscape(a) + "/";
        }
        if (line != "") {
            out << line << endl;
            if (!first) info += "<br>";
            first = false;
            info += xmlEncode(line);
        }
    }

    f.close();
    
    info += "</code></qt>";

    QMessageBox::information(this, tr("Ignored files"),
                             info);

    hgRefresh();
}

void MainWindow::hgUnIgnoreFiles(QStringList)
{
    // Not implemented: edit the .hgignore instead
    hgEditIgnore();
}

void MainWindow::hgShowIn(QStringList files)
{
    foreach (QString file, files)
    {
        QStringList args;
#if defined Q_OS_WIN32
        // Although the Win32 API is quite happy to have
        // forward slashes as directory separators, Windows
        // Explorer is not
        int last = m_workFolderPath.length() - 1;
        QChar c = m_workFolderPath[last];
        if (c == '\\' || c == '/') {
            m_workFolderPath.chop(1);
        }
        file = m_workFolderPath + "\\" + file;
        file = file.replace('/', '\\');
        args << "/select," << file;
        QProcess::execute("c:/windows/explorer.exe", args);
#elif defined(Q_OS_MAC)
        file = m_workFolderPath + "/" + file;
        args << "--reveal" << file;
        QProcess::execute("/usr/bin/open", args);
#endif
    }
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

void MainWindow::hgShowSummary()
{
    QStringList params;
    
    params << "diff" << "--stat";

    m_runner->requestAction(HgAction(ACT_UNCOMMITTED_SUMMARY, m_workFolderPath, params));
}

void MainWindow::hgFolderDiff()
{
    hgDiffFiles(QStringList());
}

void MainWindow::hgDiffFiles(QStringList files)
{
    QString diff = getDiffBinaryName();
    if (diff == "") return;

    QStringList params;

    // Diff parent against working folder (folder diff)

    params << "--config" << "extensions.extdiff=" << "extdiff";
    params << "--program" << diff << "--";

    QSettings settings;
    if (settings.value("multipleDiffInstances", false).toBool()) {
        foreach (QString file, files) {
            QStringList p = params;
            p << file;
            m_runner->requestAction(HgAction(ACT_FOLDERDIFF, m_workFolderPath, p));
        }
    }
    else {
        params << files; // may be none: whole dir
        m_runner->requestAction(HgAction(ACT_FOLDERDIFF, m_workFolderPath, params));
    }
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
    hgRevertFiles(QStringList());
}

void MainWindow::hgRevertFiles(QStringList files)
{
    QStringList params;
    QString comment;
    bool all = false;

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
         tr("Re&vert"))) {

        m_lastRevertedFiles = files;
        
        m_runner->requestAction(HgAction(ACT_REVERT, m_workFolderPath, params));
    }
}


void MainWindow::hgRenameFiles(QStringList files)
{
    QString renameTo;

    QString file;
    if (files.empty()) return;
    file = files[0];

    if (ConfirmCommentDialog::confirmAndGetShortComment
        (this,
         tr("Rename"),
         tr("Rename <code>%1</code> to:").arg(xmlEncode(file)),
         renameTo,
         tr("Re&name"))) {
        
        if (renameTo != "" && renameTo != file) {

            QStringList params;

            params << "rename" << "--" << file << renameTo;

            m_runner->requestAction(HgAction(ACT_RENAME_FILE, m_workFolderPath, params));
        }
    }
}


void MainWindow::hgCopyFiles(QStringList files)
{
    QString copyTo;

    QString file;
    if (files.empty()) return;
    file = files[0];

    if (ConfirmCommentDialog::confirmAndGetShortComment
        (this,
         tr("Copy"),
         tr("Copy <code>%1</code> to:").arg(xmlEncode(file)),
         copyTo,
         tr("Co&py"))) {
        
        if (copyTo != "" && copyTo != file) {

            QStringList params;

            params << "copy" << "--" << file << copyTo;

            m_runner->requestAction(HgAction(ACT_COPY_FILE, m_workFolderPath, params));
        }
    }
}


void MainWindow::hgMarkFilesResolved(QStringList files)
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


void MainWindow::hgRedoMerge()
{
    hgRedoFileMerges(QStringList());
}


void MainWindow::hgRedoFileMerges(QStringList files)
{
    QStringList params;

    params << "resolve";

    QString merge = getMergeBinaryName();
    if (merge != "") {
        params << "--tool" << merge;
    }

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
        hgRedoMerge();
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
            QMessageBox::critical
                (this, tr("Could not create target folder"),
                 tr("<qt><b>Could not create target folder</b><br><br>The local target folder \"%1\" does not exist<br>and could not be created.</qt>").arg(xmlEncode(m_workFolderPath)));
            m_workFolderPath = "";
            return;
        }
    }

    params << "clone" << m_remoteRepoPath << m_workFolderPath;
    
    updateWorkFolderAndRepoNames();
    m_hgTabs->updateWorkFolderFileList("");
    m_hgTabs->clearAll();

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
         tr("&Pull"))) {

        QStringList params;
        params << "pull" << m_remoteRepoPath;
        m_runner->requestAction(HgAction(ACT_PULL, m_workFolderPath, params));
    }
}

void MainWindow::hgPush()
{
    if (m_remoteRepoPath.isEmpty()) {
        changeRemoteRepo(true);
        if (m_remoteRepoPath.isEmpty()) return;
    }

    QString uncommittedNote;
    if (m_hgTabs->canCommit()) {
        uncommittedNote = tr("<p><b>Note:</b> You have uncommitted changes.  If you want to push these changes to the remote repository, you need to commit them first.");
    }

    if (ConfirmCommentDialog::confirm
        (this, tr("Confirm push"),
         tr("<qt><h3>Push to remote repository?</h3></qt>"),
         tr("<qt><p>You are about to push your commits to the remote repository at <code>%1</code>.</p>%2</qt>").arg(xmlEncode(m_remoteRepoPath)).arg(uncommittedNote),
         tr("&Push"))) {

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
    m_closedHeadIds.clear();
    m_currentBranch = "";
    m_lastStatOutput = "";
    m_lastRevertedFiles.clear();
    m_mergeTargetRevision = "";
    m_mergeCommitComment = "";
    m_stateUnknown = true;
    m_needNewLog = true;
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

    // See #202. We really want to display the port that the server
    // ends up using -- but we don't get that information until after
    // it has exited!  However, we can improve the likelihood of
    // showing the right port by at least checking whether a port is
    // defined in the hgrc file.

    QFileInfo hgrc(QDir::homePath() + "/.hgrc");
    QString path;
    int port = 8000; // the default
    if (hgrc.exists()) {
        QSettings s(hgrc.canonicalFilePath(), QSettings::IniFormat);
        s.beginGroup("web");
        int p = s.value("port").toInt();
        if (p) port = p;
    }

    QTextStream ts(&msg);
    ts << QString("<qt><h3>%1</h3><p>%2</p>")
        .arg(tr("Sharing Repository"))
        .arg(tr("Your local repository is now being made temporarily available via HTTP for workgroup access."));
    if (addrs.size() > 1) {
        ts << QString("<p>%3</p>")
            .arg(tr("Users who have network access to your computer can now clone your repository, by using one of the following URLs as a remote location:"));
    } else {
        ts << QString("<p>%3</p>")
            .arg(tr("Users who have network access to your computer can now clone your repository, by using the following URL as a remote location:"));
    }
    foreach (QString addr, addrs) {
        ts << QString("<pre>&nbsp;&nbsp;http://%1:%2</pre>").arg(xmlEncode(addr)).arg(port);
    }
    ts << tr("<p>Press Close to terminate this server, end remote access, and return.</p>");
    ts.flush();
             
    params << "serve";

    m_runner->requestAction(HgAction(ACT_SERVE, m_workFolderPath, params));
    
    QMessageBox::information(this, tr("Share Repository"), msg, QMessageBox::Close);

    m_runner->killCurrentActions();
}

void MainWindow::startupDialog()
{
    StartupDialog *dlg = new StartupDialog(this);
    if (dlg->exec()) m_firstStart = false;
    else exit(0);
}

void MainWindow::open()
{
    bool done = false;

    while (!done) {

        MultiChoiceDialog *d = new MultiChoiceDialog
                               (tr("Open Repository"),
                                tr("<qt><big>What would you like to open?</big></qt>"),
                                tr("https://code.soundsoftware.ac.uk/projects/easyhg/wiki/HelpOpenDialog"),
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
        settings.beginGroup("");
        QString lastChoice = settings.value("lastopentype", "remote").toString();
        if (lastChoice != "local" &&
            lastChoice != "remote" &&
            lastChoice != "init") {
            lastChoice = "remote";
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

void MainWindow::recentMenuActivated()
{
    QAction *a = qobject_cast<QAction *>(sender());
    if (!a) return;
    QString local = a->text();
    open(local);
}

void MainWindow::changeRemoteRepo()
{
    changeRemoteRepo(false);
}

void MainWindow::changeRemoteRepo(bool initial)
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
        (tr("Set Remote Location"),
         tr("<qt><big>Set the remote location</big></qt>"),
         "",
         this);

    QString explanation;
    if (initial) {
        explanation = tr("Provide a remote URL to use when pushing from, or pulling to, the local<br>repository <code>%1</code>.<br>This will be the default for subsequent pushes and pulls.<br>You can change it using &ldquo;Set Remote Location&rdquo; on the File menu.").arg(m_workFolderPath);
    } else {
        explanation = tr("Provide a new remote URL to use when pushing from, or pulling to, the local<br>repository <code>%1</code>.").arg(m_workFolderPath);
    }

    d->addChoice("remote",
                 tr("<qt><center><img src=\":images/browser-64.png\"><br>Remote repository</center></qt>"),
                 explanation,
                 MultiChoiceDialog::UrlArg,
                 true); // default empty

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
    // If the directory "arg" exists but is empty, then we accept it.

    // If the directory "arg" exists and is non-empty, but "arg" plus
    // the last path component of "remote" does not exist, then offer
    // the latter as an alternative path.

    QString offer;

    QDir d(arg);

    if (d.exists()) {

        if (d.entryList(QDir::Dirs | QDir::Files |
                        QDir::NoDotAndDotDot |
                        QDir::Hidden | QDir::System).empty()) {
            // directory is empty; accept it
            return arg;
        }

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
             tr("<qt><b>Initialise a repository here?</b><br><br>You asked to open \"%1\".<br>This folder is not a Mercurial working copy.<br><br>Would you like to initialise a repository here?</qt>")
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
        return openLocal(local);
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
    settings(SettingsDialog::PersonalDetailsTab);
}

void MainWindow::settings(SettingsDialog::Tab tab)
{
    SettingsDialog *settingsDlg = new SettingsDialog(this);
    settingsDlg->setCurrentTab(tab);
    settingsDlg->exec();

    if (settingsDlg->presentationChanged()) {
        m_hgTabs->updateFileStates();
        updateToolBarStyle();
        hgRefresh();
    }
}

void MainWindow::updateFsWatcher()
{
    m_fsWatcher->setWorkDirPath(m_workFolderPath);
    m_fsWatcher->setTrackedFilePaths(m_hgTabs->getFileStates().trackedFiles());
}

void MainWindow::checkFilesystem()
{
    DEBUG << "MainWindow::checkFilesystem" << endl;
    if (!m_commandSequenceInProgress) {
        if (!m_fsWatcher->getChangedPaths(m_fsWatcherToken).empty()) {
            hgRefresh();
            return;
        }
        updateFsWatcher();
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
        report = tr("New changes will be highlighted in yellow in the history.");
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
        int currentBranchActiveHeads = 0;
        bool parentIsHead = false;
        Changeset *parent = m_currentParents[0];
        foreach (Changeset *head, m_activeHeads) {
            if (head->isOnBranch(m_currentBranch)) {
                ++currentBranchActiveHeads;
            }
            if (parent->id() == head->id()) {
                parentIsHead = true;
            }
        }
        if (currentBranchActiveHeads == 2 && parentIsHead) {
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
    } else if (m_hgTabs->canCommit() && m_currentParents.size() > 1) {
        MoreInformationDialog::warning
            (this,
             tr("Push failed"),
             tr("Push failed"),
             tr("Your local repository could not be pushed to the remote repository.<br><br>You have an uncommitted merge in your local folder.  You probably need to commit it before you push."),
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

void MainWindow::reportAuthFailed(QString output)
{
    MoreInformationDialog::warning
        (this,
         tr("Authorization failed"),
         tr("Authorization failed"),
         tr("You may have entered an incorrect user name or password, or the remote URL may be wrong.<br><br>Or you may lack the necessary permissions on the remote repository.<br><br>Check with the administrator of your remote repository if necessary."),
         output);
}

void MainWindow::commandStarting(HgAction)
{
    m_commandSequenceInProgress = true;
}

void MainWindow::commandFailed(HgAction action, QString stdOut, QString stdErr)
{
    DEBUG << "MainWindow::commandFailed" << endl;

    m_commandSequenceInProgress = false;

    QString setstr;
#ifdef Q_OS_MAC
    setstr = tr("Preferences");
#else
    setstr = tr("Settings");
#endif

    // Some commands we just have to ignore bad return values from,
    // and some output gets special treatment.

    // Note our fallback case should always be to report a
    // non-specific error and show the text -- in case output scraping
    // fails (as it surely will).  Note also that we must force the
    // locale in order to ensure the output is scrapable; this happens
    // in HgRunner and may break some system encodings.

    switch(action.action) {
    case ACT_NONE:
        // uh huh
        return;
    case ACT_TEST_HG:
        MoreInformationDialog::warning
            (this,
             tr("Failed to run Mercurial"),
             tr("Failed to run Mercurial"),
             tr("The Mercurial program either could not be found or failed to run.<br><br>Check that the Mercurial program path is correct in %1.").arg(setstr),
             stdErr);
        settings(SettingsDialog::PathsTab);
        return;
    case ACT_TEST_HG_EXT:
        MoreInformationDialog::warning
            (this,
             tr("Failed to run Mercurial"),
             tr("Failed to run Mercurial with extension enabled"),
             tr("The Mercurial program failed to run with the EasyMercurial interaction extension enabled.<br>This may indicate an installation problem.<br><br>You may be able to continue working if you switch off &ldquo;Use EasyHg Mercurial Extension&rdquo; in %1.  Note that remote repositories that require authentication might not work if you do this.").arg(setstr),
             stdErr);
        settings(SettingsDialog::ExtensionsTab);
        return;
    case ACT_CLONEFROMREMOTE:
        // if clone fails, we have no repo
        m_workFolderPath = "";
        enableDisableActions();
        break; // go on to default report
    case ACT_INCOMING:
        if (stdErr.contains("authorization failed")) {
            reportAuthFailed(stdErr);
            return;
        } else if (stdErr.contains("entry cancelled")) {
            // ignore this, user cancelled username or password dialog
            return;
        } else {
            // Incoming returns non-zero code and no stdErr if the
            // check was successful but there are no changes
            // pending. This is the only case where we need to remove
            // warning messages, because it's the only case where a
            // non-zero code can be returned even though the command
            // has for our purposes succeeded
            QStringList lines = stdErr.split(QRegExp("[\\r\\n]+"));
            QString replaced;
            foreach (QString line, lines) {
                line.replace(QRegExp("^.*warning: [^\\n]*"), "");
                if (line != "") {
                    replaced += line + "\n";
                }
            }
            if (replaced == "") {
                showIncoming("");
                return;
            }
        }
        break; // go on to default report
    case ACT_PULL:
        if (stdErr.contains("authorization failed")) {
            reportAuthFailed(stdErr);
            return;
        } else if (stdErr.contains("entry cancelled")) {
            // ignore this, user cancelled username or password dialog
            return;
        } else if (stdErr.contains("no changes found") || stdOut.contains("no changes found")) {
            // success: hg 2.1 starts returning failure code for empty pull/push
            m_commandSequenceInProgress = true; // there may be further commands
            commandCompleted(action, stdOut, stdErr);
            return;
        }
        break; // go on to default report
    case ACT_PUSH:
        if (stdErr.contains("creates new remote head")) {
            reportNewRemoteHeads(stdErr);
            return;
        } else if (stdErr.contains("authorization failed")) {
            reportAuthFailed(stdErr);
            return;
        } else if (stdErr.contains("entry cancelled")) {
            // ignore this, user cancelled username or password dialog
            return;
        } else if (stdErr.contains("no changes found") || stdOut.contains("no changes found")) {
            // success: hg 2.1 starts returning failure code for empty pull/push
            m_commandSequenceInProgress = true; // there may be further commands
            commandCompleted(action, stdOut, stdErr);
            return;
        }
        break; // go on to default report
    case ACT_QUERY_HEADS_ACTIVE:
    case ACT_QUERY_HEADS:
        // fails if repo is empty; we don't care (if there's a genuine
        // problem, something else will fail too).  Pretend it
        // succeeded, so that any further actions that are contingent
        // on the success of the heads query get carried out properly.
        m_commandSequenceInProgress = true; // there may be further commands
        commandCompleted(action, "", "");
        return;
    case ACT_FOLDERDIFF:
    case ACT_CHGSETDIFF:
        // external program, unlikely to be anything useful in stdErr
        // and some return with failure codes when something as basic
        // as the user closing the window via the wm happens
        return;
    case ACT_MERGE:
        if (stdErr.contains("working directory ancestor")) {
            // arguably we should prevent this upfront, but that's
            // trickier!
            MoreInformationDialog::information
                (this, tr("Merge"), tr("Merge has no effect"),
                 tr("You asked to merge a revision with one of its ancestors.<p>This has no effect, because the ancestor's changes already exist in both revisions."),
                 stdErr);
            return;
        }
        // else fall through
    case ACT_RETRY_MERGE:
        MoreInformationDialog::information
            (this, tr("Merge"), tr("Merge failed"),
             tr("Some files were not merged successfully.<p>You can Merge again to repeat the interactive merge; use Revert to abandon the merge entirely; or edit the files that are in conflict in an editor and, when you are happy with them, choose Mark Resolved in each file's right-button menu."),
             stdErr);
        m_mergeCommitComment = "";
        hgQueryPaths();
        return;
    case ACT_STAT:
        break; // go on to default report
    default:
        break;
    }

    QString command = action.executable;
    if (command == "") command = "hg";
    foreach (QString arg, action.params) {
        command += " " + arg;
    }

    MoreInformationDialog::warning
        (this,
         tr("Command failed"),
         tr("Command failed"),
         (stdErr == "" ?
          tr("A Mercurial command failed to run correctly.  This may indicate an installation problem or some other problem with EasyMercurial.") :
          tr("A Mercurial command failed to run correctly.  This may indicate an installation problem or some other problem with EasyMercurial.<br><br>See &ldquo;More Details&rdquo; for the command output.")),
         stdErr);
}

void MainWindow::commandCompleted(HgAction completedAction, QString output, QString stdErr)
{
//    std::cerr << "commandCompleted: " << completedAction.action << std::endl;

    HGACTIONS action = completedAction.action;

    if (action == ACT_NONE) return;

    output.replace("\r\n", "\n");

    bool headsChanged = false;
    QStringList oldHeadIds;

    switch (action) {

    case ACT_TEST_HG:
    {
        QRegExp versionRE("^Mercurial.*version ([\\d+])\\.([\\d+])");
        int pos = versionRE.indexIn(output);
        if (pos >= 0) {
            int major = versionRE.cap(1).toInt();
            int minor = versionRE.cap(2).toInt();
            // We need v1.7 or newer
            if (major < 1 || (major == 1 && minor < 7)) {
                MoreInformationDialog::warning
                    (this,
                     tr("Newer Mercurial version required"),
                     tr("Newer Mercurial version required"),
                     tr("To use EasyMercurial, you should have at least Mercurial v1.7 installed.<br><br>The version found on this system (v%1.%2) does not support all of the features required by EasyMercurial.").arg(major).arg(minor),
                     output);
            }
        }
        break;
    }

    case ACT_TEST_HG_EXT:
        if (stdErr.contains("Failed to load PyQt")) {
            commandFailed(completedAction, output, stdErr);
            return;
        }
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
        break;

    case ACT_RESOLVE_LIST:
        // This happens on every update, after the stat (above)
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
        /*
        DEBUG << "m_lastStatOutput = " << m_lastStatOutput << endl;
        DEBUG << "resolve output = " << output << endl;
        */
        m_hgTabs->updateWorkFolderFileList(m_lastStatOutput + output);
        break;

    case ACT_RESOLVE_MARK:
        m_shouldHgStat = true;
        break;
        
    case ACT_INCOMING:
        showIncoming(output);
        break;

    case ACT_ANNOTATE:
    {
        AnnotateDialog dialog(this, output);
        dialog.exec();
        m_shouldHgStat = true;
        break;
    }
        
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
             tr("Open successful"),
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
        
    case ACT_QUERY_HEADS_ACTIVE:
        foreach (Changeset *cs, m_activeHeads) delete cs;
        m_activeHeads = Changeset::parseChangesets(output);
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
            updateClosedHeads();
        }
    }
        break;

    case ACT_COMMIT:
        if (m_currentParents.empty()) {
            // first commit to empty repo
            m_needNewLog = true;
        }
        m_hgTabs->clearSelections();
        m_justMerged = false;
        m_shouldHgStat = true;
        break;

    case ACT_CLOSE_BRANCH:
        m_hgTabs->clearSelections();
        m_justMerged = false;
        m_shouldHgStat = true;
        break;

    case ACT_REVERT:
        hgMarkFilesResolved(m_lastRevertedFiles);
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
    //   paths -> branch -> stat -> resolve-list -> heads ->
    //     parents -> log
    //
    // Note we want to call enableDisableActions only once, at the end
    // of whichever sequence is in use.

    bool noMore = false;

    switch (action) {

    case ACT_TEST_HG:
    {
        QSettings settings;
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
        // NB this call is duplicated in hgQueryPaths
        hgQueryBranch();
        break;

    case ACT_QUERY_BRANCH:
        // NB this call is duplicated in hgQueryBranch
        hgStat();
        break;
        
    case ACT_STAT:
        hgResolveList();
        break;
        
    case ACT_RESOLVE_LIST:
        hgQueryHeadsActive();
        break;

    case ACT_QUERY_HEADS_ACTIVE:
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
        m_commandSequenceInProgress = false;
        m_stateUnknown = false;
        enableDisableActions();
        m_hgTabs->updateHistory();
        updateRecentMenu();
        checkFilesystem();
    }
}

void MainWindow::commandCancelled(HgAction)
{
    // Originally I had this checking whether the cancelled action was
    // a network one and, if so, calling hgQueryPaths to update the
    // local view in case it had changed anything. But that doesn't
    // work properly -- because at this point, although the command
    // has been cancelled and a kill signal sent, it hasn't actually
    // exited yet. If we request another command now, it will go on
    // the stack and be associated with the failed exit forthcoming
    // from the cancelled command -- giving the user a disturbing
    // command-failed dialog
}

void MainWindow::connectActions()
{
    connect(m_exitAct, SIGNAL(triggered()), this, SLOT(close()));
    connect(m_aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    connect(m_helpAct, SIGNAL(triggered()), this, SLOT(help()));

    connect(m_hgRefreshAct, SIGNAL(triggered()), this, SLOT(hgRefresh()));
    connect(m_hgRemoveAct, SIGNAL(triggered()), this, SLOT(hgRemove()));
    connect(m_hgAddAct, SIGNAL(triggered()), this, SLOT(hgAdd()));
    connect(m_hgCommitAct, SIGNAL(triggered()), this, SLOT(hgCommit()));
    connect(m_hgIgnoreAct, SIGNAL(triggered()), this, SLOT(hgIgnore()));
    connect(m_hgEditIgnoreAct, SIGNAL(triggered()), this, SLOT(hgEditIgnore()));
    connect(m_hgFolderDiffAct, SIGNAL(triggered()), this, SLOT(hgFolderDiff()));
    connect(m_hgUpdateAct, SIGNAL(triggered()), this, SLOT(hgUpdate()));
    connect(m_hgRevertAct, SIGNAL(triggered()), this, SLOT(hgRevert()));
    connect(m_hgMergeAct, SIGNAL(triggered()), this, SLOT(hgMerge()));

    connect(m_settingsAct, SIGNAL(triggered()), this, SLOT(settings()));
    connect(m_openAct, SIGNAL(triggered()), this, SLOT(open()));
    connect(m_changeRemoteRepoAct, SIGNAL(triggered()), this, SLOT(changeRemoteRepo()));

    connect(m_hgIncomingAct, SIGNAL(triggered()), this, SLOT(hgIncoming()));
    connect(m_hgPullAct, SIGNAL(triggered()), this, SLOT(hgPull()));
    connect(m_hgPushAct, SIGNAL(triggered()), this, SLOT(hgPush()));

    connect(m_hgServeAct, SIGNAL(triggered()), this, SLOT(hgServe()));
}

void MainWindow::connectTabsSignals()
{
    connect(m_hgTabs, SIGNAL(currentChanged(int)),
            this, SLOT(enableDisableActions()));

    connect(m_hgTabs, SIGNAL(commit()),
            this, SLOT(hgCommit()));
    
    connect(m_hgTabs, SIGNAL(revert()),
            this, SLOT(hgRevert()));
    
    connect(m_hgTabs, SIGNAL(diffWorkingFolder()),
            this, SLOT(hgFolderDiff()));
    
    connect(m_hgTabs, SIGNAL(showSummary()),
            this, SLOT(hgShowSummary()));
    
    connect(m_hgTabs, SIGNAL(newBranch()),
            this, SLOT(hgNewBranch()));
    
    connect(m_hgTabs, SIGNAL(noBranch()),
            this, SLOT(hgNoBranch()));

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
            this, SLOT(hgNewBranch()));

    connect(m_hgTabs, SIGNAL(closeBranch(QString)),
            this, SLOT(hgCloseBranch()));

    connect(m_hgTabs, SIGNAL(tag(QString)),
            this, SLOT(hgTag(QString)));

    connect(m_hgTabs, SIGNAL(annotateFiles(QStringList)),
            this, SLOT(hgAnnotateFiles(QStringList)));

    connect(m_hgTabs, SIGNAL(diffFiles(QStringList)),
            this, SLOT(hgDiffFiles(QStringList)));

    connect(m_hgTabs, SIGNAL(commitFiles(QStringList)),
            this, SLOT(hgCommitFiles(QStringList)));

    connect(m_hgTabs, SIGNAL(revertFiles(QStringList)),
            this, SLOT(hgRevertFiles(QStringList)));

    connect(m_hgTabs, SIGNAL(renameFiles(QStringList)),
            this, SLOT(hgRenameFiles(QStringList)));

    connect(m_hgTabs, SIGNAL(copyFiles(QStringList)),
            this, SLOT(hgCopyFiles(QStringList)));

    connect(m_hgTabs, SIGNAL(addFiles(QStringList)),
            this, SLOT(hgAddFiles(QStringList)));

    connect(m_hgTabs, SIGNAL(removeFiles(QStringList)),
            this, SLOT(hgRemoveFiles(QStringList)));

    connect(m_hgTabs, SIGNAL(redoFileMerges(QStringList)),
            this, SLOT(hgRedoFileMerges(QStringList)));

    connect(m_hgTabs, SIGNAL(markFilesResolved(QStringList)),
            this, SLOT(hgMarkFilesResolved(QStringList)));

    connect(m_hgTabs, SIGNAL(ignoreFiles(QStringList)),
            this, SLOT(hgIgnoreFiles(QStringList)));

    connect(m_hgTabs, SIGNAL(unIgnoreFiles(QStringList)),
            this, SLOT(hgUnIgnoreFiles(QStringList)));

    connect(m_hgTabs, SIGNAL(showIn(QStringList)),
            this, SLOT(hgShowIn(QStringList)));
}    

void MainWindow::enableDisableActions()
{
    DEBUG << "MainWindow::enableDisableActions" << endl;

    QString dirname = QDir(m_workFolderPath).dirName();

    if (m_workFolderPath != "") { // dirname of "" is ".", so test path instead
        setWindowTitle(tr("EasyMercurial: %1").arg(dirname));
    } else {
        setWindowTitle(tr("EasyMercurial"));
    }

    //!!! should also do things like set the status texts for the
    //!!! actions appropriately by context

    QDir localRepoDir;
    QDir workFolderDir;

    m_remoteRepoActionsEnabled = true;
    if (m_remoteRepoPath.isEmpty()) {
        m_remoteRepoActionsEnabled = false;
    }

    m_localRepoActionsEnabled = true;
    if (m_workFolderPath.isEmpty()) {
        m_localRepoActionsEnabled = false;
    }

    if (m_workFolderPath == "" || !workFolderDir.exists(m_workFolderPath)) {
        m_localRepoActionsEnabled = false;
    }

    if (!localRepoDir.exists(m_workFolderPath + "/.hg")) {
        m_localRepoActionsEnabled = false;
    }

    bool haveDiff = false;
    QSettings settings;
    settings.beginGroup("Locations");
    if (settings.value("extdiffbinary", "").toString() != "") {
        haveDiff = true;
    }
    settings.endGroup();

    m_hgTabs->setHaveMerge(m_currentParents.size() == 2);

    m_hgRefreshAct->setEnabled(m_localRepoActionsEnabled);
    m_hgFolderDiffAct->setEnabled(m_localRepoActionsEnabled && haveDiff);
    m_hgRevertAct->setEnabled(m_localRepoActionsEnabled);
    m_hgAddAct->setEnabled(m_localRepoActionsEnabled);
    m_hgRemoveAct->setEnabled(m_localRepoActionsEnabled);
    m_hgIgnoreAct->setEnabled(m_localRepoActionsEnabled);
    m_hgUpdateAct->setEnabled(m_localRepoActionsEnabled);
    m_hgCommitAct->setEnabled(m_localRepoActionsEnabled);
    m_hgMergeAct->setEnabled(m_localRepoActionsEnabled);
    m_hgServeAct->setEnabled(m_localRepoActionsEnabled);
    m_hgEditIgnoreAct->setEnabled(m_localRepoActionsEnabled);

    DEBUG << "m_localRepoActionsEnabled = " << m_localRepoActionsEnabled << endl;
    DEBUG << "canCommit = " << m_hgTabs->canCommit() << endl;

    m_hgAddAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canAdd());
    m_hgRemoveAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canRemove());
    m_hgCommitAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canCommit());
    m_hgRevertAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canRevert());
    m_hgFolderDiffAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canDiff());
    m_hgIgnoreAct->setEnabled(m_localRepoActionsEnabled && m_hgTabs->canIgnore());

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
    bool closedBranch = false;
    int currentBranchActiveHeads = 0;

    if (m_currentParents.size() == 1) {
        bool parentIsHead = false;
        bool parentIsActiveHead = false;
        Changeset *parent = m_currentParents[0];
        foreach (Changeset *head, m_activeHeads) {
            if (head->isOnBranch(m_currentBranch)) {
                ++currentBranchActiveHeads;
            }
            if (parent->id() == head->id()) {
                parentIsActiveHead = parentIsHead = true;
            }
        }
        if (!parentIsActiveHead) {
            foreach (Changeset *head, m_currentHeads) {
                if (parent->id() == head->id()) {
                    parentIsHead = true;
                }
            }
        }
        if (currentBranchActiveHeads == 2 && parentIsActiveHead) {
            canMerge = true;
        }
        if (currentBranchActiveHeads == 0 && parentIsActiveHead) {
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
        } else if (!parentIsActiveHead) {
            closedBranch = true;
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

    m_hgIncomingAct->setEnabled(m_remoteRepoActionsEnabled);
    m_hgPullAct->setEnabled(m_remoteRepoActionsEnabled);
    // permit push even if no remote yet; we'll ask for one
    m_hgPushAct->setEnabled(m_localRepoActionsEnabled && !emptyRepo);

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
    } else if (closedBranch) {
        if (canUpdate) {
            m_workStatus->setState(tr("On a closed branch. Not at the head of the branch"));
        } else {
            m_workStatus->setState(tr("At the head of a closed branch"));
        }
    } else if (canUpdate) {
        if (m_hgTabs->haveChangesToCommit()) {
            // have uncommitted changes
            m_workStatus->setState(tr("On %1. Not at the head of the branch").arg(branchText));
        } else {
            // no uncommitted changes
            m_workStatus->setState(tr("On %1. Not at the head of the branch: consider updating").arg(branchText));
        }
    } else if (currentBranchActiveHeads > 1) {
        m_workStatus->setState(tr("At one of %n heads of %1", "", currentBranchActiveHeads).arg(branchText));
    } else {
        m_workStatus->setState(tr("At the head of %1").arg(branchText));
    }
}


void MainWindow::updateClosedHeads()
{
    m_closedHeadIds.clear();
    QSet<QString> activeIds;
    foreach (Changeset *cs, m_activeHeads) {
        activeIds.insert(cs->id());
    }
    foreach (Changeset *cs, m_currentHeads) {
        if (!activeIds.contains(cs->id())) {
            m_closedHeadIds.insert(cs->id());
        }
    }
    m_hgTabs->setClosedHeadIds(m_closedHeadIds);
}

void MainWindow::updateRecentMenu()
{
    m_recentMenu->clear();
    RecentFiles rf("Recent-local");
    QStringList recent = rf.getRecent();
    if (recent.empty()) {
        QLabel *label = new QLabel(tr("No recent local repositories"));
        QWidgetAction *wa = new QWidgetAction(m_recentMenu);
        wa->setDefaultWidget(label);
        return;
    }
    foreach (QString r, recent) {
        QAction *a = m_recentMenu->addAction(r);
        connect(a, SIGNAL(triggered()), this, SLOT(recentMenuActivated()));
    }
}

void MainWindow::createActions()
{
    //File actions
    m_openAct = new QAction(QIcon(":/images/fileopen.png"), tr("&Open..."), this);
    m_openAct->setStatusTip(tr("Open a remote repository or an existing local folder"));
    m_openAct->setShortcut(tr("Ctrl+O"));

    m_changeRemoteRepoAct = new QAction(tr("Set Push and Pull &Location..."), this);
    m_changeRemoteRepoAct->setStatusTip(tr("Set or change the default URL for pull and push actions from this repository"));

    m_settingsAct = new QAction(QIcon(":/images/settings.png"), tr("&Settings..."), this);
    m_settingsAct->setStatusTip(tr("View and change application settings"));

#ifdef Q_OS_WIN32
    m_exitAct = new QAction(QIcon(":/images/exit.png"), tr("E&xit"), this);
#else 
    m_exitAct = new QAction(QIcon(":/images/exit.png"), tr("&Quit"), this);
#endif
    m_exitAct->setShortcuts(QKeySequence::Quit);
    m_exitAct->setStatusTip(tr("Exit EasyMercurial"));

    //Repository actions
    m_hgRefreshAct = new QAction(QIcon(":/images/status.png"), tr("&Re-Read Working Folder"), this);
    m_hgRefreshAct->setShortcut(tr("Ctrl+R"));
    m_hgRefreshAct->setStatusTip(tr("Refresh the window to show the current state of the working folder"));

    m_hgIncomingAct = new QAction(QIcon(":/images/incoming.png"), tr("Pre&view Incoming Changes"), this);
    m_hgIncomingAct->setIconText(tr("Preview"));
    m_hgIncomingAct->setStatusTip(tr("See what changes are available in the remote repository waiting to be pulled"));

    m_hgPullAct = new QAction(QIcon(":/images/pull.png"), tr("Pu&ll from Remote Repository"), this);
    m_hgPullAct->setIconText(tr("Pull"));
    m_hgPullAct->setShortcut(tr("Ctrl+L"));
    m_hgPullAct->setStatusTip(tr("Pull changes from the remote repository to the local repository"));

    m_hgPushAct = new QAction(QIcon(":/images/push.png"), tr("Pus&h to Remote Repository"), this);
    m_hgPushAct->setIconText(tr("Push"));
    m_hgPushAct->setShortcut(tr("Ctrl+H"));
    m_hgPushAct->setStatusTip(tr("Push changes from the local repository to the remote repository"));

    //Workfolder actions
    m_hgFolderDiffAct   = new QAction(QIcon(":/images/folderdiff.png"), tr("&Diff"), this);
    m_hgFolderDiffAct->setIconText(tr("Diff"));
    m_hgFolderDiffAct->setShortcut(tr("Ctrl+D"));
    m_hgFolderDiffAct->setStatusTip(tr("See what has changed in the working folder compared with the last committed state"));

    m_hgRevertAct = new QAction(QIcon(":/images/undo.png"), tr("Re&vert"), this);
    m_hgRevertAct->setStatusTip(tr("Throw away your changes and return to the last committed state"));

    m_hgAddAct = new QAction(QIcon(":/images/add.png"), tr("&Add Files"), this);
    m_hgAddAct->setIconText(tr("Add"));
    m_hgAddAct->setShortcut(tr("+"));
    m_hgAddAct->setStatusTip(tr("Mark the selected files to be added on the next commit"));

    m_hgRemoveAct = new QAction(QIcon(":/images/remove.png"), tr("&Remove Files"), this);
    m_hgRemoveAct->setIconText(tr("Remove"));
    m_hgRemoveAct->setShortcut(tr("Del"));
    m_hgRemoveAct->setStatusTip(tr("Mark the selected files to be removed from version control on the next commit"));

    m_hgIgnoreAct = new QAction(tr("&Ignore Files..."), this);
    m_hgIgnoreAct->setStatusTip(tr("Add the selected filenames to the ignored list, of files that should never be tracked in this repository"));

    m_hgEditIgnoreAct = new QAction(tr("Edit Ignored List"), this);
    m_hgEditIgnoreAct->setStatusTip(tr("Edit the .hgignore file, containing the names of files that should be ignored by Mercurial"));

    m_hgUpdateAct = new QAction(QIcon(":/images/update.png"), tr("&Update to Branch Head"), this);
    m_hgUpdateAct->setIconText(tr("Update"));
    m_hgUpdateAct->setShortcut(tr("Ctrl+U"));
    m_hgUpdateAct->setStatusTip(tr("Update the working folder to the head of the current repository branch"));

    m_hgCommitAct = new QAction(QIcon(":/images/commit.png"), tr("&Commit..."), this);
    m_hgCommitAct->setShortcut(tr("Ctrl+Return"));
    m_hgCommitAct->setStatusTip(tr("Commit your changes to the local repository"));

    m_hgMergeAct = new QAction(QIcon(":/images/merge.png"), tr("&Merge"), this);
    m_hgMergeAct->setShortcut(tr("Ctrl+M"));
    m_hgMergeAct->setStatusTip(tr("Merge the two independent sets of changes in the local repository into the working folder"));

    m_hgServeAct = new QAction(tr("Share Repository"), this);
    m_hgServeAct->setStatusTip(tr("Serve local repository temporarily via HTTP for workgroup access"));

    //Help actions
#ifdef Q_OS_MAC
    m_helpAct = new QAction(tr("EasyMercurial Help"), this);
#else
    m_helpAct = new QAction(tr("Help Topics"), this);
#endif
    m_helpAct->setShortcuts(QKeySequence::HelpContents);
    m_aboutAct = new QAction(tr("About EasyMercurial"), this);

    // Miscellaneous
    QShortcut *clearSelectionsShortcut = new QShortcut(Qt::Key_Escape, this);
    connect(clearSelectionsShortcut, SIGNAL(activated()),
            this, SLOT(clearSelections()));
}

void MainWindow::createMenus()
{
#ifdef Q_OS_LINUX
    // In Ubuntu 14.04 the window's menu bar goes missing entirely if
    // the user is running any desktop environment other than Unity
    // (in which the faux single-menubar appears). The user has a
    // workaround, to remove the appmenu-qt5 package, but that is
    // awkward and the problem is so severe that it merits disabling
    // the system menubar integration altogether. Like this:
    menuBar()->setNativeMenuBar(false);
#endif

    m_fileMenu = menuBar()->addMenu(tr("&File"));

    m_fileMenu->addAction(m_openAct);
    m_recentMenu = m_fileMenu->addMenu(tr("Open Re&cent"));
    m_fileMenu->addAction(m_hgRefreshAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_hgServeAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_settingsAct);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAct);

    QMenu *workMenu;
    workMenu = menuBar()->addMenu(tr("&Work"));
    workMenu->addAction(m_hgFolderDiffAct);
    workMenu->addSeparator();
    workMenu->addAction(m_hgUpdateAct);
    workMenu->addAction(m_hgCommitAct);
    workMenu->addAction(m_hgMergeAct);
    workMenu->addSeparator();
    workMenu->addAction(m_hgAddAct);
    workMenu->addAction(m_hgRemoveAct);
    workMenu->addSeparator();
    workMenu->addAction(m_hgIgnoreAct);
    workMenu->addAction(m_hgEditIgnoreAct);
    workMenu->addSeparator();
    workMenu->addAction(m_hgRevertAct);

    QMenu *remoteMenu;
    remoteMenu = menuBar()->addMenu(tr("&Remote"));
    remoteMenu->addAction(m_hgIncomingAct);
    remoteMenu->addSeparator();
    remoteMenu->addAction(m_changeRemoteRepoAct);
    remoteMenu->addAction(m_hgPullAct);
    remoteMenu->addAction(m_hgPushAct);

    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_helpAct);
    m_helpMenu->addAction(m_aboutAct);
}

void MainWindow::createToolBars()
{
    int sz = 32;

    QString spacerBefore, spacerAfter;

    spacerBefore = spacerAfter = " ";

#ifdef Q_OS_MAC
    spacerAfter = "";
#endif

#ifdef Q_OS_WIN32
    spacerBefore = spacerAfter = "  ";
#endif

    m_repoToolBar = addToolBar(tr("Remote"));
    m_repoToolBar->setIconSize(QSize(sz, sz));
    if (spacerBefore != "") {
        m_repoToolBar->addWidget(new QLabel(spacerBefore));
    }
    m_repoToolBar->addAction(m_openAct);
    if (spacerAfter != "") {
        m_repoToolBar->addWidget(new QLabel(spacerAfter));
    }
    m_repoToolBar->addSeparator();
    m_repoToolBar->addAction(m_hgIncomingAct);
    m_repoToolBar->addAction(m_hgPullAct);
    m_repoToolBar->addAction(m_hgPushAct);
    m_repoToolBar->setMovable(false);

    m_workFolderToolBar = new QToolBar(tr("Work"));
    addToolBar(Qt::LeftToolBarArea, m_workFolderToolBar);
    m_workFolderToolBar->setIconSize(QSize(sz, sz));

    QWidget *w = new QWidget;
    w->setFixedHeight(6);
    m_workFolderToolBar->addWidget(w);

    m_workFolderToolBar->addAction(m_hgFolderDiffAct);
    m_workFolderToolBar->addSeparator();
    m_workFolderToolBar->addAction(m_hgRevertAct);
    m_workFolderToolBar->addAction(m_hgUpdateAct);
    m_workFolderToolBar->addAction(m_hgCommitAct);
    m_workFolderToolBar->addAction(m_hgMergeAct);
    m_workFolderToolBar->addSeparator();
    m_workFolderToolBar->addAction(m_hgAddAct);
    m_workFolderToolBar->addAction(m_hgRemoveAct);
    m_workFolderToolBar->setMovable(false);

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

    m_workFolderPath = settings.value("workfolderpath", "").toString();
    if (!workFolder.exists(m_workFolderPath)) {
        m_workFolderPath = "";
    }

    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(550, 550)).toSize();
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

void MainWindow::newerVersionAvailable(QString version)
{
    QSettings settings;
    settings.beginGroup("NewerVersionWarning");
    QString tag = QString("version-%1-available-show").arg(version);
    if (settings.value(tag, true).toBool()) {
        QString title(tr("Newer version available"));
        QString text(tr("<h3>Newer version available</h3><p>You are using version %1 of EasyMercurial, but version %3 is now available.</p><p>Please see the <a href=\"http://easyhg.org/\">EasyMercurial website</a> for more information.</p>").arg(EASYHG_VERSION).arg(version));
        QMessageBox::information(this, title, text);
        settings.setValue(tag, false);
    }
    settings.endGroup();
}

void MainWindow::help()
{
    if (!m_helpDialog) {
        m_helpDialog = new QDialog;
        QGridLayout *layout = new QGridLayout;
        m_helpDialog->setLayout(layout);
        QPushButton *home = new QPushButton;
        home->setIcon(QIcon(":images/home.png"));
        layout->addWidget(home, 0, 0);
        QPushButton *back = new QPushButton;
        back->setIcon(QIcon(":images/back.png"));
        layout->addWidget(back, 0, 1);
        QPushButton *fwd = new QPushButton;
        fwd->setIcon(QIcon(":images/forward.png"));
        layout->addWidget(fwd, 0, 2);
        QTextBrowser *text = new QTextBrowser;
        text->setOpenExternalLinks(true);
        layout->addWidget(text, 1, 0, 1, 4);
        text->setSource(QUrl("qrc:help/topics.html"));
        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Close);
        connect(bb, SIGNAL(rejected()), m_helpDialog, SLOT(hide()));
        connect(text, SIGNAL(backwardAvailable(bool)),
                back, SLOT(setEnabled(bool)));
        connect(text, SIGNAL(forwardAvailable(bool)),
                fwd, SLOT(setEnabled(bool)));
        connect(home, SIGNAL(clicked()), text, SLOT(home()));
        connect(back, SIGNAL(clicked()), text, SLOT(backward()));
        connect(fwd, SIGNAL(clicked()), text, SLOT(forward()));
        back->setEnabled(false);
        fwd->setEnabled(false);
        layout->addWidget(bb, 2, 0, 1, 4);
        layout->setColumnStretch(3, 20);
        double baseEm;
#ifdef Q_OS_MAC
        baseEm = 17.0;
#else
        baseEm = 15.0;
#endif
        double em = QFontMetrics(QFont()).height();
        double ratio = em / baseEm;
        m_helpDialog->setMinimumSize(450 * ratio, 500 * ratio);
    }
    QTextBrowser *tb = m_helpDialog->findChild<QTextBrowser *>();
    if (tb) tb->home();
    m_helpDialog->show();
    m_helpDialog->raise();
}

