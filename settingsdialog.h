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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "mainwindow.h"

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent = 0);

private slots:
    void okClicked();
    void cancelClicked();
    void browseWorkFolder();
    void browseRemoteRepo();

private:
    QLabel      *userInfoLabel;
    QLineEdit   *userInfoLineEdit;

    QLabel      *remoteRepoLabel;
    QComboBox   *remoteRepoCombo;
    QPushButton *remoteRepoBrowseButton;
    QHBoxLayout *remoteRepoLayout;

    QLabel      *workFolderLabel;
    QComboBox   *workFolderCombo;
    QPushButton *workFolderBrowseButton;
    QHBoxLayout *workFolderLayout;

    QPushButton *okButton;
    QPushButton *cancelButton;

    MainWindow  *mainWnd;

    void browseDirAndSetCombo(QComboBox *combo);
    void insertPathToMruList(QString newPath, QString mruList[]);
};


#endif // SETTINGSDIALOG_H
