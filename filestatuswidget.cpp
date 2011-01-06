/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2010 Chris Cannam
    Copyright (c) 2010 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "filestatuswidget.h"
#include "debug.h"
#include "multichoicedialog.h"
#include "clickablelabel.h"

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

FileStatusWidget::FileStatusWidget(QWidget *parent) :
    QWidget(parent),
    m_dateReference(0)
{
    QGridLayout *layout = new QGridLayout;
    layout->setMargin(10);
    setLayout(layout);

    int row = 0;

#ifndef Q_OS_MAC    
    layout->addItem(new QSpacerItem(1, 1), row, 0);
    ++row;
#endif

    layout->addWidget(new QLabel(tr("Local:")), row, 0);

    m_openButton = new ClickableLabel;
    QFont f(m_openButton->font());
    f.setBold(true);
    m_openButton->setFont(f);
    m_openButton->setMouseUnderline(true);
    connect(m_openButton, SIGNAL(clicked()), this, SLOT(openButtonClicked()));
    layout->addWidget(m_openButton, row, 1, 1, 2, Qt::AlignLeft);

    ++row;
    layout->addWidget(new QLabel(tr("Remote:")), row, 0);
    m_remoteURLLabel = new QLabel;
    layout->addWidget(m_remoteURLLabel, row, 1, 1, 2);

    ++row;
    layout->addWidget(new QLabel(tr("State:")), row, 0);
    m_stateLabel = new QLabel;
    layout->addWidget(m_stateLabel, row, 1, 1, 2);

    layout->setColumnStretch(1, 20);

    layout->addWidget(new QLabel("<qt><hr></qt>"), ++row, 0, 1, 3);

    ++row;

    m_noModificationsLabel = new QLabel;
    setNoModificationsLabelText();
    layout->addWidget(m_noModificationsLabel, row, 1, 1, 2);
    m_noModificationsLabel->hide();

    m_simpleLabels[FileStates::Clean] = tr("Unmodified:");
    m_simpleLabels[FileStates::Modified] = tr("Modified:");
    m_simpleLabels[FileStates::Added] = tr("Added:");
    m_simpleLabels[FileStates::Removed] = tr("Removed:");
    m_simpleLabels[FileStates::Missing] = tr("Missing:");
    m_simpleLabels[FileStates::InConflict] = tr("In Conflict:");
    m_simpleLabels[FileStates::Unknown] = tr("Untracked:");
    m_simpleLabels[FileStates::Ignored] = tr("Ignored:");

    m_descriptions[FileStates::Clean] = tr("You have not changed these files.");
    m_descriptions[FileStates::Modified] = tr("You have changed these files since you last committed them.");
    m_descriptions[FileStates::Added] = tr("These files will be added to version control next time you commit them.");
    m_descriptions[FileStates::Removed] = tr("These files will be removed from version control next time you commit them.<br>"
                                             "They will not be deleted from the local folder.");
    m_descriptions[FileStates::Missing] = tr("These files are recorded in the version control, but absent from your working folder.<br>"
                                             "If you intended to delete them, select them and use Remove to tell the version control system about it.<br>"
                                             "If you deleted them by accident, select them and use Revert to restore their previous contents.");
    m_descriptions[FileStates::InConflict] = tr("These files are unresolved following an incomplete merge.<br>Select a file and use Merge to try to resolve the merge again.");
    m_descriptions[FileStates::Unknown] = tr("These files are in your working folder but are not under version control.<br>"
//                                             "Select a file and use Add to place it under version control or Ignore to remove it from this list.");
                                             "Select a file and use Add to place it under version control.");
    m_descriptions[FileStates::Ignored] = tr("These files have names that match entries in the working folder's .hgignore file,<br>"
                                             "and so will be ignored by the version control system.");

    m_highlightExplanation = tr("Files highlighted <font color=#d40000>in red</font> "
                                "have appeared since your most recent commit or update.");

    m_boxesParent = new QWidget(this);
    layout->addWidget(m_boxesParent, ++row, 0, 1, 3);

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

        boxlayout->addItem(new QSpacerItem(2, 2), 3, 0);

        boxesLayout->addWidget(box, ++boxRow, 0);
        m_boxes.push_back(box);
        box->hide();
    }

    m_gridlyLayout = false;

    layout->setRowStretch(++row, 20);

    layout->addItem(new QSpacerItem(8, 8), ++row, 0);

    m_showAllFiles = new QCheckBox(tr("Show all files"), this);
    layout->addWidget(m_showAllFiles, ++row, 0, 1, 3, Qt::AlignLeft);
    connect(m_showAllFiles, SIGNAL(toggled(bool)),
            this, SIGNAL(showAllChanged(bool)));
}

FileStatusWidget::~FileStatusWidget()
{
    delete m_dateReference;
}

void FileStatusWidget::openButtonClicked()
{
    QDir d(m_localPath);
    if (d.exists()) {
        QStringList args;
        QString path = d.canonicalPath();
#if defined Q_OS_WIN32
        // Although the Win32 API is quite happy to have
        // forward slashes as directory separators, Windows
        // Explorer is not
        path = path.replace('/', '\\');
        args << path;
        QProcess::execute("c:/windows/explorer.exe", args);
#else
        args << path;
        QProcess::execute(
#if defined Q_OS_MAC
            "/usr/bin/open",
#else
            "/usr/bin/xdg-open",
#endif
            args);
#endif
    }
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
    } else {
        return QString("<qt><b>%1</b></qt>")
            .arg(m_simpleLabels[s]);
    }
    settings.endGroup();
}

void FileStatusWidget::setNoModificationsLabelText()
{
    QSettings settings;
    settings.beginGroup("Presentation");
    if (settings.value("showhelpfultext", true).toBool()) {
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
    return !m_fileStates.added().empty() ||
           !m_fileStates.removed().empty() ||
           !m_fileStates.modified().empty();
}

bool FileStatusWidget::haveSelection() const
{
    return !m_selectedFiles.empty();
}

QStringList FileStatusWidget::getAllSelectedFiles() const
{
    return m_selectedFiles;
}

QStringList FileStatusWidget::getSelectedCommittableFiles() const
{
    QStringList files;
    foreach (QString f, m_selectedFiles) {
        switch (m_fileStates.getStateOfFile(f)) {
        case FileStates::Added:
        case FileStates::Modified:
        case FileStates::Removed:
            files.push_back(f);
            break;
        default: break;
        }
    }
    return files;
}

QStringList FileStatusWidget::getAllCommittableFiles() const
{
    QStringList files;
    files << m_fileStates.getFilesInState(FileStates::Modified);
    files << m_fileStates.getFilesInState(FileStates::Added);
    files << m_fileStates.getFilesInState(FileStates::Removed);
    return files;
}

QStringList FileStatusWidget::getSelectedRevertableFiles() const
{
    QStringList files;
    foreach (QString f, m_selectedFiles) {
        switch (m_fileStates.getStateOfFile(f)) {
        case FileStates::Added:
        case FileStates::Modified:
        case FileStates::Removed:
        case FileStates::Missing:
        case FileStates::InConflict:
            files.push_back(f);
            break;
        default: break;
        }
    }
    return files;
}

QStringList FileStatusWidget::getAllRevertableFiles() const
{
    QStringList files;
    files << m_fileStates.getFilesInState(FileStates::Modified);
    files << m_fileStates.getFilesInState(FileStates::Added);
    files << m_fileStates.getFilesInState(FileStates::Removed);
    files << m_fileStates.getFilesInState(FileStates::Missing);
    files << m_fileStates.getFilesInState(FileStates::InConflict);
    return files;
}

QStringList FileStatusWidget::getSelectedUnresolvedFiles() const
{
    QStringList files;
    foreach (QString f, m_selectedFiles) {
        switch (m_fileStates.getStateOfFile(f)) {
        case FileStates::InConflict:
            files.push_back(f);
            break;
        default: break;
        }
    }
    return files;
}

QStringList FileStatusWidget::getAllUnresolvedFiles() const
{
    QStringList files;
    files << m_fileStates.getFilesInState(FileStates::InConflict);
    return files;
}

QStringList FileStatusWidget::getSelectedAddableFiles() const
{
    QStringList files;
    foreach (QString f, m_selectedFiles) {
        switch (m_fileStates.getStateOfFile(f)) {
        case FileStates::Unknown:
        case FileStates::Removed:
            files.push_back(f);
            break;
        default: break;
        }
    }
    return files;
}

QStringList FileStatusWidget::getAllAddableFiles() const
{
    QStringList files;
    files << m_fileStates.getFilesInState(FileStates::Removed);
    files << m_fileStates.getFilesInState(FileStates::Unknown);
    return files;
}

QStringList FileStatusWidget::getSelectedRemovableFiles() const
{
    QStringList files;
    foreach (QString f, m_selectedFiles) {
        switch (m_fileStates.getStateOfFile(f)) {
        case FileStates::Clean:
        case FileStates::Added:
        case FileStates::Modified:
        case FileStates::Missing:
        case FileStates::InConflict:
            files.push_back(f);
            break;
        default: break;
        }
    }
    return files;
}

QStringList FileStatusWidget::getAllRemovableFiles() const
{
    QStringList files;
    files << m_fileStates.getFilesInState(FileStates::Clean);
    files << m_fileStates.getFilesInState(FileStates::Added);
    files << m_fileStates.getFilesInState(FileStates::Modified);
    files << m_fileStates.getFilesInState(FileStates::Missing);
    files << m_fileStates.getFilesInState(FileStates::InConflict);
    return files;
}

void
FileStatusWidget::setLocalPath(QString p)
{
    m_localPath = p;
    m_openButton->setText(p);
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
    }
    m_openButton->setEnabled(QDir(m_localPath).exists());
}

void
FileStatusWidget::setRemoteURL(QString r)
{
    m_remoteURL = r;
    m_remoteURLLabel->setText(r);
}

void
FileStatusWidget::setFileStates(FileStates p)
{
    m_fileStates = p;
    updateWidgets();
}

void
FileStatusWidget::setState(QString b)
{
    m_state = b;
    updateStateLabel();
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

    foreach (FileStates::State s, m_stateListMap.keys()) {

        QListWidget *w = m_stateListMap[s];
        w->clear();
        QStringList files = m_fileStates.getFilesInState(s);

        QStringList highPriority, lowPriority;

        foreach (QString file, files) {

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
            item->setForeground(QColor("#d40000")); //!!! and a nice gold star
            item->setSelected(selectedFiles.contains(file));
        }

        foreach (QString file, lowPriority) {
            QListWidgetItem *item = new QListWidgetItem(file);
            w->addItem(item);
            item->setSelected(selectedFiles.contains(file));
        }

        setLabelFor(w, s, !highPriority.empty());

        if (files.empty()) {
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

    updateStateLabel();
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

void FileStatusWidget::updateStateLabel()
{
    m_stateLabel->setText(m_state);
}
