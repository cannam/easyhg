#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "mainwindow.h"

/****************************************************************************
** Copyright (C) Jari Korhonen, 2010 (under lgpl)
****************************************************************************/

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
