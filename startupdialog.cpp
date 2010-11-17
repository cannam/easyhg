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

#include "startupdialog.h"
#include "common.h"

#include <QGridLayout>
#include <QDialogButtonBox>
#include <QSettings>

StartupDialog::StartupDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("About me"));

    QSettings settings;
    settings.beginGroup("User Information");
    m_name = settings.value("name", getUserRealName()).toString();
    m_email = settings.value("email").toString();
    
    QGridLayout *layout = new QGridLayout;
    int row = 0;

    layout->addWidget(new QLabel(tr("<qt><big><bold>Welcome to EasyMercurial!</qt></bold></big><br>How would you like to be identified in commit messages?")),
		      row++, 0, 1, 2);

    layout->addWidget(new QLabel(tr("Name:")), row, 0);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setText(m_name);
    connect(m_nameEdit, SIGNAL(textChanged(const QString &)),
	    this, SLOT(realNameChanged(const QString &)));
    layout->addWidget(m_nameEdit, row++, 1);
    
    layout->addWidget(new QLabel(tr("Email address:")), row, 0);

    m_emailEdit = new QLineEdit();
    m_emailEdit->setText(m_email);
    connect(m_emailEdit, SIGNAL(textChanged(const QString &)),
	    this, SLOT(emailChanged(const QString &)));
    layout->addWidget(m_emailEdit, row++, 1);

    layout->addWidget(new QLabel(tr("<br>You will appear as:")), row, 0, Qt::AlignBottom);
    m_example = new QLabel();
    layout->addWidget(m_example, row++, 1, Qt::AlignBottom);

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    layout->addWidget(bbox, row++, 0, 1, 2);
    m_ok = bbox->button(QDialogButtonBox::Ok);
    m_ok->setEnabled(false);
    
    setLayout(layout);

    m_ok->setEnabled(m_name != "");
    updateExample();
}

void
StartupDialog::realNameChanged(const QString &s)
{
    m_name = s.trimmed();
    m_ok->setEnabled(m_name != "");
    updateExample();
}

void
StartupDialog::emailChanged(const QString &s)
{
    m_email = s.trimmed();
    updateExample();
}

void
StartupDialog::accept()
{
    QSettings settings;
    settings.beginGroup("User Information");
    settings.setValue("name", m_name);
    settings.setValue("email", m_email);
    QDialog::accept();
}

void
StartupDialog::updateExample()
{
    QString identifier;

    if (m_email != "") {
	identifier = QString("%1 <%2>").arg(m_name).arg(m_email);
    } else {
	identifier = m_name;
    }

    m_example->setText(identifier);
}

    
