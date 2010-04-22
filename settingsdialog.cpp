//** Copyright (C) Jari Korhonen, 2010 (under lgpl)

#include "settingsdialog.h"

#include <QHBoxLayout>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent): QDialog(parent)
{
    QPushButton *okButton;
    QPushButton *cancelButton;

    mainWnd = (MainWindow *) parent;

    userInfoLabel = new QLabel(tr("User info for commits, e.g. John Smith <john.smith@mail.com>"));
    userInfoLineEdit = new QLineEdit(mainWnd->userInfo);
    userInfoLabel -> setBuddy(userInfoLineEdit);

    remoteRepoLabel = new QLabel(tr("Remote repository path, e.g. http://192.168.1.10:8000/ or /home/mike/anotherrepo/ or c:\\anotherrepo\\"));
    remoteRepoLineEdit = new QLineEdit(mainWnd->remoteRepoPath);
    remoteRepoLabel -> setBuddy(remoteRepoLineEdit);
    remoteRepoBrowseButton = new QPushButton(tr("Browse..."));

    workFolderLabel = new QLabel(tr("Local work folder path, e.g. /home/mike/work/ or c:\\mike\\work\\"));
    workFolderLineEdit = new QLineEdit(mainWnd -> workFolderPath);
    workFolderLabel -> setBuddy(workFolderLineEdit);
    workFolderBrowseButton = new QPushButton(tr("Browse..."));

    okButton = new QPushButton(tr("Ok"));
    cancelButton = new QPushButton(tr("Cancel"));

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout -> addWidget(okButton);
    btnLayout -> addWidget(cancelButton);
    btnLayout -> addStretch();

    QHBoxLayout *workFolderLayout = new QHBoxLayout;
    workFolderLayout -> addWidget(workFolderLineEdit);
    workFolderLayout -> addWidget(workFolderBrowseButton);

    QHBoxLayout *remoteRepoLayout = new QHBoxLayout;
    remoteRepoLayout -> addWidget(remoteRepoLineEdit);
    remoteRepoLayout -> addWidget(remoteRepoBrowseButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    mainLayout -> addWidget(userInfoLabel);
    mainLayout -> addWidget(userInfoLineEdit);

    mainLayout -> addWidget(remoteRepoLabel);
    mainLayout -> addLayout(remoteRepoLayout);

    mainLayout -> addWidget(workFolderLabel);
    mainLayout -> addLayout(workFolderLayout);

    mainLayout -> addLayout(btnLayout);

    setLayout(mainLayout);

    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(workFolderBrowseButton, SIGNAL(clicked()), this, SLOT(browseWorkFolder()));
    connect(remoteRepoBrowseButton, SIGNAL(clicked()), this, SLOT(browseRemoteRepo()));
}

#define EMPTY_DIR 2

void SettingsDialog::okClicked()
{
    mainWnd -> firstStart = false;
    mainWnd -> userInfo = userInfoLineEdit->text();
    mainWnd -> remoteRepoPath = remoteRepoLineEdit->text();
    mainWnd -> workFolderPath = workFolderLineEdit -> text();
    mainWnd -> writeSettings();
    mainWnd -> enableDisableActions();
    mainWnd -> hgStat();
    mainWnd -> hgExp -> setWorkFolderAndRepoNames(mainWnd -> workFolderPath, mainWnd -> remoteRepoPath);

    QDir dir(mainWnd -> workFolderPath);
    if (dir.exists(mainWnd -> workFolderPath))
    {
        uint cnt = dir.count();
        if (cnt == EMPTY_DIR)
        {
            QMessageBox::information(this, tr("Todo"), tr("Your chosen workfolder is empty.\nChoose \"File/Clone from remote\"\nto download a remote repository.\nYou can also choose \"File/Init local repository\"\nto initialize repository and add files later."));
        }
        else
        {
            QString repoPath = mainWnd -> workFolderPath + getHgDirName();
            QDir repoDir(repoPath);
            if (!repoDir.exists())
            {
                QMessageBox::information(this, tr("Todo"), tr("Your chosen workfolder is not empty,\nbut does not yet contain a repository.\nChoose \"File/Init local repository\" \nto initialize repository."));
            }
        }
    }


    close();
}


void SettingsDialog::cancelClicked()
{
    close();
}


void SettingsDialog::browseDirAndSetLineEdit(QLineEdit *lineEdit)
{
    QString dir;
    QString startDir;
    QString system;

    system = getSystem();
    if ((system == "Linux") || (system == "Mac"))
    {
        startDir = QDir::homePath();
    }
    else
    {
        startDir = "c:\\";
    }

    dir =  QFileDialog::getExistingDirectory(this, tr("Open Directory"),
        startDir,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    lineEdit->setText(dir + QDir::separator());
}

void SettingsDialog::browseWorkFolder()
{
    browseDirAndSetLineEdit(workFolderLineEdit);
}

void SettingsDialog::browseRemoteRepo()
{
    browseDirAndSetLineEdit(remoteRepoLineEdit);
}


