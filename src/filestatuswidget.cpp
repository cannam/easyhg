/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2012 Chris Cannam
    Copyright (c) 2012 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "filestatuswidget.h"
#include "debug.h"
#include "multichoicedialog.h"
#include "findwidget.h"

#include <QLabel>
#include <QListWidget>
#include <QGridLayout>
#include <QFileInfo>
#include <QApplication>
#include <QDateTime>
#include <QPushButton>
#include <QToolButton>
#include <QDir>
#include <QProcess>
#include <QCheckBox>
#include <QSettings>
#include <QAction>

FileStatusWidget::FileStatusWidget(QWidget *parent) :
    QWidget(parent),
    m_dateReference(0)
{
    QGridLayout *layout = new QGridLayout;
    layout->setMargin(10);
    setLayout(layout);

    int row = 0;

    m_noModificationsLabel = new QLabel;
    setNoModificationsLabelText();
    layout->addWidget(m_noModificationsLabel, row, 0);
    m_noModificationsLabel->hide();

    m_simpleLabels[FileStates::Clean] = tr("Unmodified:");
    m_simpleLabels[FileStates::Modified] = tr("Modified:");
    m_simpleLabels[FileStates::Added] = tr("Added:");
    m_simpleLabels[FileStates::Removed] = tr("Removed:");
    m_simpleLabels[FileStates::Missing] = tr("Missing:");
    m_simpleLabels[FileStates::InConflict] = tr("In Conflict:");
    m_simpleLabels[FileStates::Unknown] = tr("Untracked:");
    m_simpleLabels[FileStates::Ignored] = tr("Ignored:");

    m_actionLabels[FileStates::Annotate] = tr("Show annotated version");
    m_actionLabels[FileStates::Diff] = tr("Diff to parent");
    m_actionLabels[FileStates::Commit] = tr("Commit...");
    m_actionLabels[FileStates::Revert] = tr("Revert to last committed state");
    m_actionLabels[FileStates::Rename] = tr("Rename...");
    m_actionLabels[FileStates::Copy] = tr("Copy...");
    m_actionLabels[FileStates::Add] = tr("Add to version control");
    m_actionLabels[FileStates::Remove] = tr("Remove from version control");
    m_actionLabels[FileStates::RedoMerge] = tr("Redo merge");
    m_actionLabels[FileStates::MarkResolved] = tr("Mark conflict as resolved");
    m_actionLabels[FileStates::Ignore] = tr("Ignore...");
    // Unignore is too difficult in fact, so we just offer to edit the hgignore
    m_actionLabels[FileStates::UnIgnore] = tr("Edit .hgignore File");

    m_descriptions[FileStates::Clean] = tr("You have not changed these files.");
    m_descriptions[FileStates::Modified] = tr("You have changed these files since you last committed them.");
    m_descriptions[FileStates::Added] = tr("These files will be added to version control next time you commit them.");
    m_descriptions[FileStates::Removed] = tr("These files will be removed from version control next time you commit them.<br>"
                                             "They will not be deleted from the local folder.");
    m_descriptions[FileStates::Missing] = tr("These files are recorded in the version control, but absent from your working folder.<br>"
                                             "If you intended to delete them, select them and use Remove to tell the version control system about it.<br>"
                                             "If you deleted them by accident, select them and use Revert to restore their previous contents.");
    m_descriptions[FileStates::InConflict] = tr("These files are unresolved following an incomplete merge.<br>Use Merge to try to resolve the merge again.");
    m_descriptions[FileStates::Unknown] = tr("These files are in your working folder but are not under version control.<br>"
//                                             "Select a file and use Add to place it under version control or Ignore to remove it from this list.");
                                             "Select a file and use Add to place it under version control.");
    m_descriptions[FileStates::Ignored] = tr("These files have names that match entries in the working folder's .hgignore file,<br>"
                                             "and so will be ignored by the version control system.");

    m_highlightExplanation = tr("Files highlighted <font color=#d40000>in red</font> "
                                "have appeared since your most recent commit or update.");

    m_boxesParent = new QWidget(this);
    layout->addWidget(m_boxesParent, ++row, 0);

    QGridLayout *boxesLayout = new QGridLayout;
    boxesLayout->setMargin(0);
    m_boxesParent->setLayout(boxesLayout);
    int boxRow = 0;

    for (int i = int(FileStates::FirstState);
         i <= int(FileStates::LastState); ++i) {

        FileStates::State s = FileStates::State(i);

        QWidget *box = new QWidget(m_boxesParent);
        QGridLayout *boxlayout = new QGridLayout;
        boxlayout->setMargin(0);
        box->setLayout(boxlayout);

        boxlayout->addItem(new QSpacerItem(3, 3), 0, 0);

        QLabel *label = new QLabel(labelFor(s));
        label->setWordWrap(true);
        boxlayout->addWidget(label, 1, 0);

        QListWidget *w = new QListWidget;
        m_stateListMap[s] = w;
        w->setSelectionMode(QListWidget::ExtendedSelection);
        boxlayout->addWidget(w, 2, 0);

        connect(w, SIGNAL(itemSelectionChanged()),
                this, SLOT(itemSelectionChanged()));
        connect(w, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
                this, SLOT(itemDoubleClicked(QListWidgetItem *)));

        FileStates::Activities activities = m_fileStates.activitiesSupportedBy(s);
        int prevGroup = -1;
        foreach (FileStates::Activity a, activities) {
            int group = FileStates::activityGroup(a);
            if (group != prevGroup && prevGroup != -1) {
                QAction *sep = new QAction("", w);
                sep->setSeparator(true);
                w->insertAction(0, sep);
            }
            prevGroup = group;
            QAction *act = new QAction(m_actionLabels[a], w);
            act->setProperty("state", s);
            act->setProperty("activity", a);
            connect(act, SIGNAL(triggered()), this, SLOT(menuActionActivated()));
            w->insertAction(0, act);
        }
        w->setContextMenuPolicy(Qt::ActionsContextMenu);

        boxlayout->addItem(new QSpacerItem(2, 2), 3, 0);

        boxesLayout->addWidget(box, ++boxRow, 0);
        m_boxes.push_back(box);
        box->hide();
    }

    m_gridlyLayout = false;

    layout->setRowStretch(++row, 20);

    layout->addItem(new QSpacerItem(8, 8), ++row, 0);

    QWidget *opts = new QWidget;
    QGridLayout *optLayout = new QGridLayout(opts);
    optLayout->setMargin(0);
    layout->addWidget(opts, ++row, 0);

    m_findWidget = new FindWidget(this);
    optLayout->addWidget(m_findWidget, 0, 0, Qt::AlignLeft);
    connect(m_findWidget, SIGNAL(findTextChanged(QString)),
            this, SLOT(setSearchText(QString)));

    m_showAllFiles = new QCheckBox(tr("Show all file states"), this);
    m_showAllFiles->setEnabled(false);
    optLayout->addWidget(m_showAllFiles, 0, 1, Qt::AlignRight);

    QSettings settings;
    m_showAllFiles->setChecked(settings.value("showall", false).toBool());

    connect(m_showAllFiles, SIGNAL(toggled(bool)),
            this, SIGNAL(showAllChanged()));
}

FileStatusWidget::~FileStatusWidget()
{
    QSettings settings;
    settings.setValue("showall", m_showAllFiles->isChecked());

    delete m_dateReference;
}

bool FileStatusWidget::shouldShowAll() const
{
    return m_showAllFiles->isChecked();
}

bool FileStatusWidget::shouldShow(FileStates::State s) const
{
    if (shouldShowAll()) return true;
    else return (s != FileStates::Clean &&
                 s != FileStates::Ignored);
}

QString FileStatusWidget::labelFor(FileStates::State s, bool addHighlightExplanation)
{
    QSettings settings;
    settings.beginGroup("Presentation");
    if (settings.value("showhelpfultext", true).toBool()) {
        if (addHighlightExplanation) {
            return QString("<qt><b>%1</b><br>%2<br>%3</qt>")
                .arg(m_simpleLabels[s])
                .arg(m_descriptions[s])
                .arg(m_highlightExplanation);
        } else {
            return QString("<qt><b>%1</b><br>%2</qt>")
                .arg(m_simpleLabels[s])
                .arg(m_descriptions[s]);
        }
    }
    return QString("<qt><b>%1</b></qt>")
        .arg(m_simpleLabels[s]);
}

void FileStatusWidget::setNoModificationsLabelText()
{
    QSettings settings;
    settings.beginGroup("Presentation");

    if (m_searchText != "") {
        if (!m_showAllFiles->isChecked()) {
            m_noModificationsLabel->setText
                (tr("<qt><b>Nothing found</b><br>None of the modified files have matching filenames.<br>Select <b>Show all file states</b> to find matches among unmodified and untracked files as well.</qt>"));
        } else {
            m_noModificationsLabel->setText
                (tr("<qt><b>Nothing found</b><br>No files have matching filenames.</qt>"));
        }
    } else if (settings.value("showhelpfultext", true).toBool()) {
        m_noModificationsLabel->setText
            (tr("<qt>This area will list files in your working folder that you have changed.<br><br>At the moment you have no uncommitted changes.<br><br>To see changes previously made to the repository,<br>switch to the History tab.<br><br>%1</qt>")
#if defined Q_OS_MAC
             .arg(tr("To open the working folder in Finder,<br>click on the &ldquo;Local&rdquo; folder path shown above."))
#elif defined Q_OS_WIN32
             .arg(tr("To open the working folder in Windows Explorer,<br>click on the &ldquo;Local&rdquo; folder path shown above."))
#else
             .arg(tr("To open the working folder in your system file manager,<br>click the &ldquo;Local&rdquo; folder path shown above."))
#endif
                );
    } else {
        m_noModificationsLabel->setText
            (tr("<qt>You have no uncommitted changes.</qt>"));
    }
}


void FileStatusWidget::menuActionActivated()
{
    QAction *act = qobject_cast<QAction *>(sender());
    if (!act) return;
    
    FileStates::State state = (FileStates::State)
        act->property("state").toUInt();
    FileStates::Activity activity = (FileStates::Activity)
        act->property("activity").toUInt();

    DEBUG << "menuActionActivated: state = " << state << ", activity = "
          << activity << endl;

    if (!FileStates::supportsActivity(state, activity)) {
        std::cerr << "WARNING: FileStatusWidget::menuActionActivated: "
                  << "Action state " << state << " does not support activity "
                  << activity << std::endl;
        return;
    }

    QStringList files = getSelectedFilesInState(state);

    switch (activity) {
    case FileStates::Annotate: emit annotateFiles(files); break;
    case FileStates::Diff: emit diffFiles(files); break;
    case FileStates::Commit: emit commitFiles(files); break;
    case FileStates::Revert: emit revertFiles(files); break;
    case FileStates::Rename: emit renameFiles(files); break;
    case FileStates::Copy: emit copyFiles(files); break;
    case FileStates::Add: emit addFiles(files); break;
    case FileStates::Remove: emit removeFiles(files); break;
    case FileStates::RedoMerge: emit redoFileMerges(files); break;
    case FileStates::MarkResolved: emit markFilesResolved(files); break;
    case FileStates::Ignore: emit ignoreFiles(files); break;
    case FileStates::UnIgnore: emit unIgnoreFiles(files); break;
    }
}

void FileStatusWidget::itemDoubleClicked(QListWidgetItem *item)
{
    QStringList files;
    QString file = item->text();
    files << file;

    switch (m_fileStates.stateOf(file)) {

    case FileStates::Modified:
    case FileStates::InConflict:
        emit diffFiles(files);
        break;

    case FileStates::Clean:
    case FileStates::Missing:
        emit annotateFiles(files);
        break;
       
    default:
        break;
    }
}

void FileStatusWidget::itemSelectionChanged()
{
    DEBUG << "FileStatusWidget::itemSelectionChanged" << endl;

    QListWidget *list = qobject_cast<QListWidget *>(sender());

    if (list) {
        foreach (QListWidget *w, m_stateListMap) {
            if (w != list) {
                w->blockSignals(true);
                w->clearSelection();
                w->blockSignals(false);
            }
        }
    }

    m_selectedFiles.clear();

    foreach (QListWidget *w, m_stateListMap) {
        QList<QListWidgetItem *> sel = w->selectedItems();
        foreach (QListWidgetItem *i, sel) {
            m_selectedFiles.push_back(i->text());
            DEBUG << "file " << i->text() << " is selected" << endl;
        }
    }

    emit selectionChanged();
}

void FileStatusWidget::clearSelections()
{
    m_selectedFiles.clear();
    foreach (QListWidget *w, m_stateListMap) {
        w->clearSelection();
    }
}

bool FileStatusWidget::haveChangesToCommit() const
{
    return !getAllCommittableFiles().empty();
}

bool FileStatusWidget::haveSelection() const
{
    return !m_selectedFiles.empty();
}

QStringList FileStatusWidget::getSelectedFilesInState(FileStates::State s) const
{
    QStringList files;
    foreach (QString f, m_selectedFiles) {
        if (m_fileStates.stateOf(f) == s) files.push_back(f);
    }
    return files;
}    

QStringList FileStatusWidget::getSelectedFilesSupportingActivity(FileStates::Activity a) const
{
    QStringList files;
    foreach (QString f, m_selectedFiles) {
        if (m_fileStates.supportsActivity(f, a)) files.push_back(f);
    }
    return files;
}    

QStringList FileStatusWidget::getAllCommittableFiles() const
{
    return m_fileStates.filesSupportingActivity(FileStates::Commit);
}

QStringList FileStatusWidget::getAllRevertableFiles() const
{
    return m_fileStates.filesSupportingActivity(FileStates::Revert);
}

QStringList FileStatusWidget::getAllUnresolvedFiles() const
{
    return m_fileStates.filesInState(FileStates::InConflict);
}

QStringList FileStatusWidget::getSelectedAddableFiles() const
{
    return getSelectedFilesSupportingActivity(FileStates::Add);
}

QStringList FileStatusWidget::getSelectedRemovableFiles() const
{
    return getSelectedFilesSupportingActivity(FileStates::Remove);
}

QString
FileStatusWidget::localPath() const
{
    return m_localPath;
}

void
FileStatusWidget::setLocalPath(QString p)
{
    m_localPath = p;
    delete m_dateReference;
    m_dateReference = new QFileInfo(p + "/.hg/dirstate");
    if (!m_dateReference->exists() ||
        !m_dateReference->isFile() ||
        !m_dateReference->isReadable()) {
        DEBUG << "FileStatusWidget::setLocalPath: date reference file "
                << m_dateReference->absoluteFilePath()
                << " does not exist, is not a file, or cannot be read"
                << endl;
        delete m_dateReference;
        m_dateReference = 0;
        m_showAllFiles->setEnabled(false);
    } else {
        m_showAllFiles->setEnabled(true);
    }
}

void
FileStatusWidget::setFileStates(FileStates p)
{
    m_fileStates = p;
    updateWidgets();
}

void
FileStatusWidget::setSearchText(QString text)
{
    if (m_searchText == text) return;
    m_searchText = text;
    updateWidgets();
}

void
FileStatusWidget::updateWidgets()
{
    QDateTime lastInteractionTime;
    if (m_dateReference) {
        lastInteractionTime = m_dateReference->lastModified();
        DEBUG << "reference time: " << lastInteractionTime << endl;
    }

    QSet<QString> selectedFiles;
    foreach (QString f, m_selectedFiles) selectedFiles.insert(f);

    int visibleCount = 0;
    bool finding = (m_searchText != "");

    foreach (FileStates::State s, m_stateListMap.keys()) {

        QListWidget *w = m_stateListMap[s];
        w->clear();

        if (!shouldShow(s)) {
            w->parentWidget()->hide();
            continue;
        }

        QStringList files = m_fileStates.filesInState(s);
        bool foundSomething = false;

        QStringList highPriority, lowPriority;

        foreach (QString file, files) {

            if (finding) {
                if (file.contains(m_searchText, Qt::CaseInsensitive)) {
                    highPriority.push_back(file);
                    foundSomething = true;
                }
                continue;
            } else {
                foundSomething = true;
            }

            bool highlighted = false;

            if (s == FileStates::Unknown) {
                // We want to highlight untracked files that have appeared
                // since the last interaction with the repo
                QString fn(m_localPath + "/" + file);
                DEBUG << "comparing with " << fn << endl;
                QFileInfo fi(fn);
                if (fi.exists() && fi.created() > lastInteractionTime) {
                    DEBUG << "file " << fn << " is newer (" << fi.lastModified()
                            << ") than reference" << endl;
                    highlighted = true;
                }
            }

            if (highlighted) {
                highPriority.push_back(file);
            } else {
                lowPriority.push_back(file);
            }
        }

        foreach (QString file, highPriority) {
            QListWidgetItem *item = new QListWidgetItem(file);
            w->addItem(item);
            if (finding) {
                item->setForeground(QColor("#008400"));
            } else {
                item->setForeground(QColor("#d40000"));
            }                
            item->setSelected(selectedFiles.contains(file));
        }

        foreach (QString file, lowPriority) {
            QListWidgetItem *item = new QListWidgetItem(file);
            w->addItem(item);
            item->setSelected(selectedFiles.contains(file));
        }

        setLabelFor(w, s, !finding && !highPriority.empty());

        if (!foundSomething) {
            w->parentWidget()->hide();
        } else {
            w->parentWidget()->show();
            ++visibleCount;
        }
    }

    m_noModificationsLabel->setVisible(visibleCount == 0);

    if (visibleCount > 3) {
        layoutBoxesGridly(visibleCount);
    } else {
        layoutBoxesLinearly();
    }

    setNoModificationsLabelText();
}

void FileStatusWidget::layoutBoxesGridly(int visibleCount)
{
    if (m_gridlyLayout && m_lastGridlyCount == visibleCount) return;

    delete m_boxesParent->layout();
    
    QGridLayout *layout = new QGridLayout;
    layout->setMargin(0);
    m_boxesParent->setLayout(layout);

    int row = 0;
    int col = 0;

    DEBUG << "FileStatusWidget::layoutBoxesGridly: visibleCount = "
          << visibleCount << endl;

    for (int i = 0; i < m_boxes.size(); ++i) {

        if (!m_boxes[i]->isVisible()) continue;

        if (col == 0 && row >= (visibleCount+1)/2) {
            layout->addItem(new QSpacerItem(10, 5), 0, 1);
            col = 2;
            row = 0;
        }

        layout->addWidget(m_boxes[i], row, col);

        ++row;
    }

    m_gridlyLayout = true;
    m_lastGridlyCount = visibleCount;
}

void FileStatusWidget::layoutBoxesLinearly()
{
    if (!m_gridlyLayout) return;

    delete m_boxesParent->layout();
    
    QGridLayout *layout = new QGridLayout;
    layout->setMargin(0);
    m_boxesParent->setLayout(layout);

    for (int i = 0; i < m_boxes.size(); ++i) {
        layout->addWidget(m_boxes[i], i, 0);
    }

    m_gridlyLayout = false;
}

void FileStatusWidget::setLabelFor(QWidget *w, FileStates::State s, bool addHighlight)
{
    QString text = labelFor(s, addHighlight);
    QWidget *p = w->parentWidget();
    QList<QLabel *> ql = p->findChildren<QLabel *>();
    if (!ql.empty()) ql[0]->setText(text);
}

