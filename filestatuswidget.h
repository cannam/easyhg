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

#ifndef FILESTATUSWIDGET_H
#define FILESTATUSWIDGET_H

#include "filestates.h"

#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;
class QFileInfo;

class FileStatusWidget : public QWidget
{
    Q_OBJECT

public:
    FileStatusWidget(QWidget *parent = 0);
    ~FileStatusWidget();

    QString localPath() const { return m_localPath; }
    void setLocalPath(QString p);

    QString remoteURL() const { return m_remoteURL; }
    void setRemoteURL(QString u);

    QString state() const { return m_state; }
    void setState(QString b);

    FileStates fileStates() const { return m_fileStates; }
    void setFileStates(FileStates sp);

    bool haveChangesToCommit() const;
    bool haveSelection() const;

    QStringList getAllSelectedFiles() const;

    QStringList getSelectedCommittableFiles() const;
    QStringList getAllCommittableFiles() const;

    QStringList getSelectedRevertableFiles() const;
    QStringList getAllRevertableFiles() const;

    QStringList getSelectedAddableFiles() const;
    QStringList getAllAddableFiles() const;

    QStringList getSelectedRemovableFiles() const;
    QStringList getAllRemovableFiles() const;

    QStringList getSelectedUnresolvedFiles() const;
    QStringList getAllUnresolvedFiles() const;
    
signals:
    void selectionChanged();

public slots:
    void clearSelections();

private slots:
    void itemSelectionChanged();

private:
    QString m_localPath;
    QLabel *m_localPathLabel;

    QString m_remoteURL;
    QLabel *m_remoteURLLabel;

    QString m_state;
    QLabel *m_stateLabel;
    
    QLabel *m_noModificationsLabel;

    FileStates m_fileStates;
    QMap<FileStates::State, QString> m_simpleLabels;
    QMap<FileStates::State, QString> m_descriptions;
    QMap<FileStates::State, QListWidget *> m_stateListMap;
    QString m_highlightExplanation;

    QFileInfo *m_dateReference;
    QStringList m_selectedFiles;

    void updateWidgets();
    void updateStateLabel();
    QString labelFor(FileStates::State, bool addHighlightExplanation = false);
    void setLabelFor(QWidget *w, FileStates::State, bool addHighlightExplanation);
};

#endif
