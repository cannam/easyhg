#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "mainwindow.h"


//** Copyright (C) Jari Korhonen, 2010 (under lgpl)

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
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
    QLineEdit   *remoteRepoLineEdit;
    QPushButton *remoteRepoBrowseButton;
    QHBoxLayout *remoteRepoLayout;

    QLabel      *workFolderLabel;
    QLineEdit   *workFolderLineEdit;
    QPushButton *workFolderBrowseButton;
    QHBoxLayout *workFolderLayout;

    QPushButton *okButton;
    QPushButton *cancelButton;

    MainWindow  *mainWnd;

    void browseDirAndSetLineEdit(QLineEdit *lineEdit);
};


#endif // SETTINGSDIALOG_H
