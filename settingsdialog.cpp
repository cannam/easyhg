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

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Settings"));

    QSettings settings;
    
    QGridLayout *mainLayout = new QGridLayout;
    setLayout(mainLayout);

    QGroupBox *meBox = new QGroupBox(tr("About me"));
    mainLayout->addWidget(meBox, 0, 0);
    QGridLayout *meLayout = new QGridLayout;
    meBox->setLayout(meLayout);

    settings.beginGroup("User Information");

    int row = 0;

    meLayout->addWidget(new QLabel(tr("Name:")), row, 0);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setText(settings.value("name", getUserRealName()).toString());
    connect(m_nameEdit, SIGNAL(textChanged(const QString &)),
	    this, SLOT(realNameChanged(const QString &)));
    meLayout->addWidget(m_nameEdit, row++, 1);
    
    meLayout->addWidget(new QLabel(tr("Email address:")), row, 0);

    m_emailEdit = new QLineEdit();
    m_emailEdit->setText(settings.value("email").toString());
    connect(m_emailEdit, SIGNAL(textChanged(const QString &)),
	    this, SLOT(emailChanged(const QString &)));
    meLayout->addWidget(m_emailEdit, row++, 1);

    settings.endGroup();

    QGroupBox *pathsBox = new QGroupBox(tr("System application locations"));
    mainLayout->addWidget(pathsBox, 1, 0);
    QGridLayout *pathsLayout = new QGridLayout;
    pathsBox->setLayout(pathsLayout);

    settings.beginGroup("Locations");

    row = 0;

    pathsLayout->addWidget(new QLabel(tr("Mercurial (hg) program:")), row, 0);

    m_hgPathEdit = new QLineEdit();
    m_hgPathEdit->setText(settings.value("hgbinary").toString());
    connect(m_hgPathEdit, SIGNAL(textChanged(const QString &)),
	    this, SLOT(hgPathChanged(const QString &)));
    pathsLayout->addWidget(m_hgPathEdit, row, 1);

    QPushButton *browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 2);
    connect(browse, SIGNAL(clicked()), this, SLOT(hgPathBrowse()));

    pathsLayout->addWidget(new QLabel(tr("External diff program:")), row, 0);

    m_diffPathEdit = new QLineEdit();
    m_diffPathEdit->setText(settings.value("extdiffbinary").toString());
    connect(m_diffPathEdit, SIGNAL(textChanged(const QString &)),
	    this, SLOT(diffPathChanged(const QString &)));
    pathsLayout->addWidget(m_diffPathEdit, row, 1);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 2);
    connect(browse, SIGNAL(clicked()), this, SLOT(diffPathBrowse()));
    
    pathsLayout->addWidget(new QLabel(tr("External file-merge program:")), row, 0);

    m_mergePathEdit = new QLineEdit();
    m_mergePathEdit->setText(settings.value("mergebinary").toString());
    connect(m_mergePathEdit, SIGNAL(textChanged(const QString &)),
	    this, SLOT(mergePathChanged(const QString &)));
    pathsLayout->addWidget(m_mergePathEdit, row, 1);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 2);
    connect(browse, SIGNAL(clicked()), this, SLOT(mergePathBrowse()));

    pathsLayout->addWidget(new QLabel(tr("External text editor:")), row, 0);

    m_editPathEdit = new QLineEdit();
    m_editPathEdit->setText(settings.value("editorbinary").toString());
    connect(m_editPathEdit, SIGNAL(textChanged(const QString &)),
	    this, SLOT(editPathChanged(const QString &)));
    pathsLayout->addWidget(m_editPathEdit, row, 1);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 2);
    connect(browse, SIGNAL(clicked()), this, SLOT(editPathBrowse()));

    settings.endGroup();
    
    settings.beginGroup("Locations");

    pathsLayout->addWidget(new QLabel(tr("EasyHg Mercurial extension:")), row, 0);

    m_extensionPathEdit = new QLineEdit();
    m_extensionPathEdit->setText(settings.value("extensionpath").toString());
    connect(m_extensionPathEdit, SIGNAL(textChanged(const QString &)),
	    this, SLOT(extensionPathChanged(const QString &)));
    pathsLayout->addWidget(m_extensionPathEdit, row, 1);

    browse = new QPushButton(tr("Browse..."));
    pathsLayout->addWidget(browse, row++, 2);
    connect(browse, SIGNAL(clicked()), this, SLOT(extensionPathBrowse()));

    settings.endGroup();

    settings.beginGroup("General");

    //!!! more info plz
    m_useExtension = new QCheckBox(tr("Use EasyHg Mercurial extension"));
    m_useExtension->setChecked(settings.value("useextension", true).toBool());
    pathsLayout->addWidget(m_useExtension, row++, 1);

    settings.endGroup();


    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    mainLayout->addWidget(bbox, 2, 0);
    m_ok = bbox->button(QDialogButtonBox::Ok);
//    m_ok->setEnabled(false);
    
//!!!    m_ok->setEnabled(m_name != "");
//    updateExample();
}

void
SettingsDialog::realNameChanged(const QString &s)
{
}

void
SettingsDialog::emailChanged(const QString &s)
{
}

void
SettingsDialog::hgPathChanged(const QString &s)
{
}

void
SettingsDialog::hgPathBrowse()
{
}

void
SettingsDialog::diffPathChanged(const QString &s)
{
}

void
SettingsDialog::diffPathBrowse()
{
}

void
SettingsDialog::mergePathChanged(const QString &s)
{
}

void
SettingsDialog::mergePathBrowse()
{
}

void
SettingsDialog::editPathChanged(const QString &s)
{
}

void
SettingsDialog::editPathBrowse()
{
}

void
SettingsDialog::extensionPathChanged(const QString &s)
{
}

void
SettingsDialog::extensionPathBrowse()
{
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
    settings.setValue("hgbinary", m_hgPathEdit->text());
    settings.setValue("extdiffbinary", m_diffPathEdit->text());
    settings.setValue("mergebinary", m_mergePathEdit->text());
    settings.setValue("extensionpath", m_extensionPathEdit->text());
    settings.endGroup();
    settings.beginGroup("General");
    settings.setValue("useextension", m_useExtension->isChecked());
    settings.endGroup();
    QDialog::accept();
}

    
