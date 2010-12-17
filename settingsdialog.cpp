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

#include "settingsdialog.h"
#include "common.h"
#include "debug.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QSettings>
#include <QDir>
#include <QFileDialog>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Settings"));

    QSettings settings;
    
    QGridLayout *mainLayout = new QGridLayout;
    setLayout(mainLayout);

    QGroupBox *meBox = new QGroupBox(tr("User details"));
    mainLayout->addWidget(meBox, 0, 0);
    QGridLayout *meLayout = new QGridLayout;
    meBox->setLayout(meLayout);

    settings.beginGroup("User Information");

    int row = 0;

    meLayout->addWidget(new QLabel(tr("Name:")), row, 0);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setText(settings.value("name", getUserRealName()).toString());
    meLayout->addWidget(m_nameEdit, row++, 1);
    
    meLayout->addWidget(new QLabel(tr("Email address:")), row, 0);

    m_emailEdit = new QLineEdit();
    m_emailEdit->setText(settings.value("email").toString());
    meLayout->addWidget(m_emailEdit, row++, 1);

    settings.endGroup();

    QGroupBox *pathsBox = new QGroupBox(tr("System application locations"));
    mainLayout->addWidget(pathsBox, 1, 0);
    QGridLayout *pathsLayout = new QGridLayout;
    pathsBox->setLayout(pathsLayout);

    settings.beginGroup("Locations");

    row = 0;

    pathsLayout->addWidget(new QLabel(tr("Mercurial (hg) program:")), row, 0);

    m_hgPathLabel = new QLabel();
    m_hgPathLabel->setText(settings.value("hgbinary").toString());
    pathsLayout->addWidget(m_hgPathLabel, row, 2);

    QPushButton *browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 1);
    connect(browse, SIGNAL(clicked()), this, SLOT(hgPathBrowse()));

    pathsLayout->addWidget(new QLabel(tr("External diff program:")), row, 0);

    m_diffPathLabel = new QLabel();
    m_diffPathLabel->setText(settings.value("extdiffbinary").toString());
    pathsLayout->addWidget(m_diffPathLabel, row, 2);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 1);
    connect(browse, SIGNAL(clicked()), this, SLOT(diffPathBrowse()));
    
    pathsLayout->addWidget(new QLabel(tr("External file-merge program:")), row, 0);

    m_mergePathLabel = new QLabel();
    m_mergePathLabel->setText(settings.value("mergebinary").toString());
    pathsLayout->addWidget(m_mergePathLabel, row, 2);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 1);
    connect(browse, SIGNAL(clicked()), this, SLOT(mergePathBrowse()));

    pathsLayout->addWidget(new QLabel(tr("External text editor:")), row, 0);

    m_editPathLabel = new QLabel();
    m_editPathLabel->setText(settings.value("editorbinary").toString());
    pathsLayout->addWidget(m_editPathLabel, row, 2);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 1);
    connect(browse, SIGNAL(clicked()), this, SLOT(editPathBrowse()));

    settings.endGroup();
    
    settings.beginGroup("Locations");

    pathsLayout->addWidget(new QLabel(tr("EasyHg Mercurial extension:")), row, 0);

    m_extensionPathLabel = new QLabel();
    m_extensionPathLabel->setText(settings.value("extensionpath").toString());
    pathsLayout->addWidget(m_extensionPathLabel, row, 2);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 1);
    connect(browse, SIGNAL(clicked()), this, SLOT(extensionPathBrowse()));

    settings.endGroup();

    settings.beginGroup("General");

    //!!! more info plz
    m_useExtension = new QCheckBox(tr("Use EasyHg Mercurial extension"));
    m_useExtension->setChecked(settings.value("useextension", true).toBool());
    pathsLayout->addWidget(m_useExtension, row++, 2);

    settings.endGroup();


    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    mainLayout->addWidget(bbox, 2, 0);
    m_ok = bbox->button(QDialogButtonBox::Ok);
}

void
SettingsDialog::hgPathBrowse()
{
    browseFor(tr("Mercurial program"), m_hgPathLabel);
}

void
SettingsDialog::diffPathBrowse()
{
    browseFor(tr("External diff program"), m_diffPathLabel);
}

void
SettingsDialog::mergePathBrowse()
{
    browseFor(tr("External file-merge program"), m_mergePathLabel);
}

void
SettingsDialog::editPathBrowse()
{
    browseFor(tr("External text editor"), m_editPathLabel);
}

void
SettingsDialog::extensionPathBrowse()
{
    browseFor(tr("EasyHg Mercurial extension"), m_extensionPathLabel);
}

void
SettingsDialog::browseFor(QString title, QLabel *edit)
{
    QString origin = edit->text();

    if (origin == "") {
#ifdef Q_OS_WIN32
        origin = "c:";
#else
        origin = QDir::homePath();
#endif
    }
    
    QString path = QFileDialog::getOpenFileName(this, title, origin);
    if (path != QString()) {
        edit->setText(path);
    }
}

void
SettingsDialog::accept()
{
    DEBUG << "SettingsDialog::accept" << endl;
    QSettings settings;
    settings.beginGroup("User Information");
    settings.setValue("name", m_nameEdit->text());
    settings.setValue("email", m_emailEdit->text());
    settings.endGroup();
    settings.beginGroup("Locations");
    settings.setValue("hgbinary", m_hgPathLabel->text());
    settings.setValue("extdiffbinary", m_diffPathLabel->text());
    settings.setValue("mergebinary", m_mergePathLabel->text());
    settings.setValue("extensionpath", m_extensionPathLabel->text());
    settings.endGroup();
    settings.beginGroup("General");
    settings.setValue("useextension", m_useExtension->isChecked());
    settings.endGroup();
    QDialog::accept();
}

    
