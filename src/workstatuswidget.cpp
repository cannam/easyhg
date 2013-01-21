/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on HgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2013 Chris Cannam
    Copyright (c) 2013 Queen Mary, University of London
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "workstatuswidget.h"
#include "debug.h"
#include "clickablelabel.h"
#include "squeezedlabel.h"

#include <QGridLayout>
#include <QSpacerItem>
#include <QLabel>
#include <QProcess>
#include <QDir>

WorkStatusWidget::WorkStatusWidget(QWidget *parent) :
    QWidget(parent)
{
    QGridLayout *layout = new QGridLayout;
    layout->setMargin(6);
    layout->setSpacing(6);
    setLayout(layout);

    int row = 0;

#ifndef Q_OS_MAC    
    layout->addItem(new QSpacerItem(1, 1), row, 0);
    ++row;
#endif

    layout->addWidget(new QLabel(tr("Local:")), row, 1);

    m_openButton = new ClickableLabel;
    QFont f(m_openButton->font());
    f.setBold(true);
    m_openButton->setFont(f);
    m_openButton->setMouseUnderline(true);
    connect(m_openButton, SIGNAL(clicked()), this, SLOT(openButtonClicked()));
    layout->addWidget(m_openButton, row, 2, 1, 2);

    ++row;
    layout->addWidget(new QLabel(tr("Remote:")), row, 1);
    m_remoteURLLabel = new SqueezedLabel;
    m_remoteURLLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_remoteURLLabel, row, 2, 1, 2);

    ++row;
    layout->addWidget(new QLabel(tr("State:")), row, 1);
    m_stateLabel = new QLabel;
    layout->addWidget(m_stateLabel, row, 2, 1, 2);

    layout->setColumnStretch(2, 20);


}

WorkStatusWidget::~WorkStatusWidget()
{
}

void
WorkStatusWidget::setLocalPath(QString p)
{
    m_localPath = p;
    m_openButton->setText(p);
    m_openButton->setEnabled(QDir(m_localPath).exists());
}

void
WorkStatusWidget::setRemoteURL(QString r)
{
    m_remoteURL = r;
    m_remoteURLLabel->setText(r);
}

void
WorkStatusWidget::setState(QString b)
{
    m_state = b;
    updateStateLabel();
}

void
WorkStatusWidget::updateStateLabel()
{
    m_stateLabel->setText(m_state);
}

void
WorkStatusWidget::openButtonClicked()
{
    QDir d(m_localPath);
    if (d.exists()) {
        QStringList args;
        QString path = d.canonicalPath();
#if defined Q_OS_WIN32
        // Although the Win32 API is quite happy to have
        // forward slashes as directory separators, Windows
        // Explorer is not
        path = path.replace('/', '\\');
        args << path;
        QProcess::execute("c:/windows/explorer.exe", args);
#else
        args << path;
        QProcess::execute(
#if defined Q_OS_MAC
            "/usr/bin/open",
#else
            "/usr/bin/xdg-open",
#endif
            args);
#endif
    }
}
