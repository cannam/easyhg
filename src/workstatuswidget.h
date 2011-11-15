/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef WORKSTATUSWIDGET_H
#define WORKSTATUSWIDGET_H

#include <QWidget>

class SqueezedLabel;

class QLabel;
class QPushButton;
class QFileInfo;
class ClickableLabel;
class QCheckBox;

class WorkStatusWidget : public QWidget
{
    Q_OBJECT

public:
    WorkStatusWidget(QWidget *parent = 0);
    ~WorkStatusWidget();

    QString localPath() const { return m_localPath; }
    void setLocalPath(QString p);

    QString remoteURL() const { return m_remoteURL; }
    void setRemoteURL(QString u);

    QString state() const { return m_state; }
    void setState(QString b);

private slots:
    void openButtonClicked();

private:
    QString m_localPath;
    ClickableLabel *m_openButton;

    QString m_remoteURL;
    SqueezedLabel *m_remoteURLLabel;

    QString m_state;
    QLabel *m_stateLabel;

    void updateStateLabel();
};

#endif
