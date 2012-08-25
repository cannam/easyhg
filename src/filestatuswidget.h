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

#ifndef FILESTATUSWIDGET_H
#define FILESTATUSWIDGET_H

#include "filestates.h"

#include <QWidget>
#include <QList>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QFileInfo;
class QCheckBox;
class FindWidget;

class FileStatusWidget : public QWidget
{
    Q_OBJECT

public:
    FileStatusWidget(QWidget *parent = 0);
    ~FileStatusWidget();

    QString localPath() const;
    void setLocalPath(QString p);

    FileStates fileStates() const;
    void setFileStates(FileStates sp);

    bool haveChangesToCommit() const;
    bool haveSelection() const;

    QStringList getAllCommittableFiles() const;
    QStringList getAllRevertableFiles() const;
    QStringList getAllUnresolvedFiles() const;

    QStringList getSelectedAddableFiles() const;
    QStringList getSelectedRemovableFiles() const;

    bool shouldShowAll() const;
    bool shouldShow(FileStates::State) const;

signals:
    void selectionChanged();
    void showAllChanged();

    void annotateFiles(QStringList);
    void diffFiles(QStringList);
    void commitFiles(QStringList);
    void revertFiles(QStringList);
    void renameFiles(QStringList);
    void copyFiles(QStringList);
    void addFiles(QStringList);
    void removeFiles(QStringList);
    void redoFileMerges(QStringList);
    void markFilesResolved(QStringList);
    void ignoreFiles(QStringList);
    void unIgnoreFiles(QStringList);
    void showIn(QStringList);

public slots:
    void clearSelections();
    void updateWidgets();
    void clearWidgets(); // e.g. while cloning a new repo slowly

    void setSearchText(QString text);

private slots:
    void menuActionActivated();
    void itemSelectionChanged();
    void itemDoubleClicked(QListWidgetItem *);

private:
    QString m_localPath;
    QLabel *m_noModificationsLabel;

    FindWidget *m_findWidget;
    QCheckBox *m_showAllFiles;
    
    FileStates m_fileStates;
    QMap<FileStates::State, QString> m_simpleLabels;
    QMap<FileStates::State, QString> m_descriptions;
    QMap<FileStates::State, QListWidget *> m_stateListMap;
    QMap<FileStates::Activity, QString> m_actionLabels;
    QMap<FileStates::Activity, QString> m_shortcuts;
    QString m_highlightExplanation;

    QFileInfo *m_dateReference;
    QStringList m_selectedFiles;

    bool m_gridlyLayout;
    int m_lastGridlyCount;
    QList<QWidget *> m_boxes;
    QWidget *m_boxesParent;

    QString m_searchText;

    void layoutBoxesGridly(int count);
    void layoutBoxesLinearly();
    void setNoModificationsLabelText();
    QString labelFor(FileStates::State, bool addHighlightExplanation = false);
    void setLabelFor(QWidget *w, FileStates::State, bool addHighlightExplanation);

    QStringList getSelectedFilesInState(FileStates::State s) const;
    QStringList getSelectedFilesSupportingActivity(FileStates::Activity) const;
};

#endif
