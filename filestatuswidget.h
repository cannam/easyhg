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

class FileStatusWidget : public QWidget
{
    Q_OBJECT

public:
    FileStatusWidget(QWidget *parent = 0);

    QString localPath() const { return m_localPath; }
    void setLocalPath(QString p);

    QString remoteURL() const { return m_remoteURL; }
    void setRemoteURL(QString u);

    FileStates fileStates() const { return m_fileStates; }
    void setFileStates(FileStates sp);

    bool haveChangesToCommit() const {
        return !m_fileStates.added.empty() ||
               !m_fileStates.removed.empty() ||
               !m_fileStates.modified.empty();
    }

private:
    QString m_localPath;
    QLabel *m_localPathLabel;

    QString m_remoteURL;
    QLabel *m_remoteURLLabel;
    
    FileStates m_fileStates;

    QListWidget *m_modifiedList;
    QListWidget *m_addedList;
    QListWidget *m_unknownList;
    QListWidget *m_removedList;
    QListWidget *m_missingList;

    void updateWidgets();
};

#endif
