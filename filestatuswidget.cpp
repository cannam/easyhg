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

#include <QLabel>
#include <QListWidget>
#include <QGridLayout>
#include <QFileInfo>
#include <QApplication>
#include <QDateTime>

#include "debug.h"

FileStatusWidget::FileStatusWidget(QWidget *parent) :
    QWidget(parent),
    m_dateReference(0)
{
    QGridLayout *layout = new QGridLayout;
    setLayout(layout);

    int row = 0;
    
    layout->addWidget(new QLabel(tr("Local:")), row, 0);
    m_localPathLabel = new QLabel;
    QFont f(m_localPathLabel->font());
    f.setBold(true);
    m_localPathLabel->setFont(f);
    layout->addWidget(m_localPathLabel, row, 1);

    ++row;
    layout->addWidget(new QLabel(tr("Remote:")), row, 0);
    m_remoteURLLabel = new QLabel;
    layout->addWidget(m_remoteURLLabel, row, 1);

    layout->setColumnStretch(1, 20);

    m_simpleLabels[FileStates::Clean] = tr("Unmodified:");
    m_simpleLabels[FileStates::Modified] = tr("Modified:");
    m_simpleLabels[FileStates::Added] = tr("Added:");
    m_simpleLabels[FileStates::Removed] = tr("Removed:");
    m_simpleLabels[FileStates::Missing] = tr("Missing:");
    m_simpleLabels[FileStates::Unknown] = tr("Untracked:");

    m_descriptions[FileStates::Clean] = tr("You have not changed these files.");
    m_descriptions[FileStates::Modified] = tr("You have changed these files since you last committed them.");
    m_descriptions[FileStates::Added] = tr("These files will be added to version control next time you commit.");
    m_descriptions[FileStates::Removed] = tr("These files will be removed from version control next time you commit.<br>"
                                             "They will not be deleted from the local folder.");
    m_descriptions[FileStates::Missing] = tr("These files are recorded in the version control but absent from your working folder.<br>"
                                             "If you deleted them intentionally, select them here and use <b>Remove</b> to tell the version control system about it.");
    m_descriptions[FileStates::Unknown] = tr("These files are in your working folder but are not under version control.<br>"
                                             "Select a file and use Add to place it under version control or Ignore to remove it from this list.");

    m_highlightExplanation = tr("Files highlighted <font color=red>in red</font> "
                                "have appeared since your most recent commit or update.");

    for (int i = int(FileStates::FirstState);
             i <= int(FileStates::LastState); ++i) {

        FileStates::State s = FileStates::State(i);

        QWidget *box = new QWidget;
        QGridLayout *boxlayout = new QGridLayout;
        boxlayout->setMargin(0);
        box->setLayout(boxlayout);

        boxlayout->addItem(new QSpacerItem(5, 8), 0, 0);

        boxlayout->addWidget(new QLabel(labelFor(s)), 1, 0);

        QListWidget *w = new QListWidget;
        m_stateListMap[s] = w;
        w->setSelectionMode(QListWidget::ExtendedSelection);
        boxlayout->addWidget(w, 2, 0);

        connect(w, SIGNAL(itemSelectionChanged()),
                this, SLOT(itemSelectionChanged()));

        layout->addWidget(box, ++row, 0, 1, 2);
        box->hide();
    }

    layout->setRowStretch(++row, 20);
}

FileStatusWidget::~FileStatusWidget()
{
    delete m_dateReference;
}

QString FileStatusWidget::labelFor(FileStates::State s, bool addHighlightExplanation)
{
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

void FileStatusWidget::itemSelectionChanged()
{
    m_selectedFiles.clear();

    DEBUG << "FileStatusWidget::itemSelectionChanged" << endl;

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
    return files;
}

void
FileStatusWidget::setLocalPath(QString p)
{
    m_localPath = p;
    m_localPathLabel->setText(p);
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
FileStatusWidget::updateWidgets()
{
    QDateTime lastInteractionTime;
    if (m_dateReference) {
        lastInteractionTime = m_dateReference->lastModified();
        DEBUG << "reference time: " << lastInteractionTime << endl;
    }

    QSet<QString> selectedFiles;
    foreach (QString f, m_selectedFiles) selectedFiles.insert(f);

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
            item->setForeground(Qt::red); //!!! and a nice gold star
            item->setSelected(selectedFiles.contains(file));
        }

        foreach (QString file, lowPriority) {
            QListWidgetItem *item = new QListWidgetItem(file);
            w->addItem(item);
            item->setSelected(selectedFiles.contains(file));
        }

        setLabelFor(w, s, !highPriority.empty());

        w->parentWidget()->setVisible(!files.empty());
    }
}

void FileStatusWidget::setLabelFor(QWidget *w, FileStates::State s, bool addHighlight)
{
    QString text = labelFor(s, addHighlight);
    QWidget *p = w->parentWidget();
    QList<QLabel *> ql = p->findChildren<QLabel *>();
    if (!ql.empty()) ql[0]->setText(text);
}