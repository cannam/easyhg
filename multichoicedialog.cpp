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

#include "multichoicedialog.h"

#include "selectablelabel.h"

#include "debug.h"

#include <QDialogButtonBox>
#include <QToolButton>
#include <QPushButton>
#include <QFont>
#include <QDir>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>

MultiChoiceDialog::MultiChoiceDialog(QString title, QString heading,
                                     QString helpUrl, QWidget *parent) :
    QDialog(parent),
    m_helpUrl(helpUrl)
{
    setModal(true);
    setWindowTitle(title);

    QGridLayout *outer = new QGridLayout;
    setLayout(outer);

    outer->addWidget(new QLabel(heading), 0, 0, 1, 3);

    QWidget *innerWidget = new QWidget;
    outer->addWidget(innerWidget, 1, 0, 1, 3);
    m_choiceLayout = new QHBoxLayout;
    innerWidget->setLayout(m_choiceLayout);

    m_descriptionLabel = new QLabel;
    outer->addWidget(m_descriptionLabel, 2, 0, 1, 3);

    QFont f = m_descriptionLabel->font();
    f.setPointSize(f.pointSize() * 0.95);
    m_descriptionLabel->setFont(f);

    m_urlLabel = new QLabel(tr("&URL:"));
    outer->addWidget(m_urlLabel, 3, 0);

    m_urlCombo = new QComboBox();
    m_urlCombo->setEditable(true);
    m_urlLabel->setBuddy(m_urlCombo);
    connect(m_urlCombo, SIGNAL(editTextChanged(const QString &)),
            this, SLOT(urlChanged(const QString &)));
    outer->addWidget(m_urlCombo, 3, 1, 1, 2);

    m_fileLabel = new QLabel(tr("&File:"));
    outer->addWidget(m_fileLabel, 4, 0);

    m_fileCombo = new QComboBox();
    m_fileCombo->setEditable(true);
    m_fileLabel->setBuddy(m_fileCombo);
    connect(m_fileCombo, SIGNAL(editTextChanged(const QString &)),
            this, SLOT(fileChanged(const QString &)));
    outer->addWidget(m_fileCombo, 4, 1);
    outer->setColumnStretch(1, 20);

    m_browseButton = new QPushButton(tr("Browse..."));
    outer->addWidget(m_browseButton, 4, 2);
    connect(m_browseButton, SIGNAL(clicked()), this, SLOT(browse()));

    outer->addItem(new QSpacerItem(2, 12), 5, 0);

    QDialogButtonBox *bbox;
    if (helpUrl != "") {
        bbox = new QDialogButtonBox(QDialogButtonBox::Help |
                                    QDialogButtonBox::Ok |
                                    QDialogButtonBox::Cancel);
    } else {
        bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                    QDialogButtonBox::Cancel);
    }        
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(bbox, SIGNAL(helpRequested()), this, SLOT(helpRequested()));
    outer->addWidget(bbox, 6, 0, 1, 3);

    m_okButton = bbox->button(QDialogButtonBox::Ok);
    updateOkButton();

    setMinimumWidth(480);
}

void
MultiChoiceDialog::helpRequested()
{
    QDesktopServices::openUrl(m_helpUrl);
}

QString
MultiChoiceDialog::getCurrentChoice()
{
    return m_currentChoice;
}

void
MultiChoiceDialog::setCurrentChoice(QString c)
{
    m_currentChoice = c;
    choiceChanged();
}

QString
MultiChoiceDialog::getArgument()
{
    if (m_argTypes[m_currentChoice] == UrlArg ||
        m_argTypes[m_currentChoice] == UrlToDirectoryArg) {
        return m_urlCombo->currentText();
    } else {
        return m_fileCombo->currentText();
    }
}

QString
MultiChoiceDialog::getAdditionalArgument()
{
    if (m_argTypes[m_currentChoice] == UrlToDirectoryArg) {
        return m_fileCombo->currentText();
    } else {
        return "";
    }
}

void
MultiChoiceDialog::addRecentArgument(QString id, QString arg,
                                     bool additionalArgument)
{
    if (additionalArgument) {
        RecentFiles(QString("Recent-%1-add").arg(id)).addFile(arg);
    } else {
        RecentFiles(QString("Recent-%1").arg(id)).addFile(arg);
    }
}

void
MultiChoiceDialog::addChoice(QString id, QString text,
                             QString description, ArgType arg)
{
    bool first = (m_texts.empty());

    m_texts[id] = text;
    m_descriptions[id] = description;
    m_argTypes[id] = arg;
    
    if (arg != NoArg) {
        m_recentFiles[id] = QSharedPointer<RecentFiles>
            (new RecentFiles(QString("Recent-%1").arg(id)));
    }

    SelectableLabel *cb = new SelectableLabel;
    cb->setSelectedText(text);
    cb->setUnselectedText(text);
    cb->setMaximumWidth(270);

    m_choiceLayout->addWidget(cb);
    m_choiceButtons[cb] = id;

    connect(cb, SIGNAL(selectionChanged()), this, SLOT(choiceChanged()));

    if (first) {
        m_currentChoice = id;
        choiceChanged();
    }
}

void
MultiChoiceDialog::browse()
{
    QString origin = getArgument();

    if (origin == "") {
#ifdef Q_OS_WIN32
        origin = "c:";
#else
        origin = QDir::homePath();
#endif
    }

    QString path = origin;

    if (m_argTypes[m_currentChoice] == DirectoryArg ||
        m_argTypes[m_currentChoice] == UrlToDirectoryArg) {

        path = QFileDialog::getExistingDirectory
            (this, tr("Open Directory"), origin,
             QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (path != QString()) {
            m_fileCombo->lineEdit()->setText(path + QDir::separator());
        }

    } else {

        path = QFileDialog::getOpenFileName
            (this, tr("Open File"), origin);
        if (path != QString()) {
            m_fileCombo->lineEdit()->setText(path);
        }
    }
}

void
MultiChoiceDialog::urlChanged(const QString &s)
{
    updateOkButton();
}

void
MultiChoiceDialog::fileChanged(const QString &s)
{
    updateOkButton();
}

void
MultiChoiceDialog::updateOkButton()
{
/* This doesn't work well
    if (m_argTypes[m_currentChoice] != UrlToDirectoryArg) {
        return;
    }
    QDir dirPath(m_fileCombo->currentText());
    if (!dirPath.exists()) {
        if (!dirPath.cdUp()) return;
    }
    QString url = m_urlCombo->currentText();
    if (QRegExp("^\\w+://").indexIn(url) < 0) {
        return;
    }
    QString urlDirName = url;
    urlDirName.replace(QRegExp("^.*\\//.*\\/"), "");
    if (urlDirName == "" || urlDirName == url) {
        return;
    }
    m_fileCombo->lineEdit()->setText(dirPath.filePath(urlDirName));
*/
    if (m_argTypes[m_currentChoice] == UrlToDirectoryArg) {
        m_okButton->setEnabled(getArgument() != "" &&
                               getAdditionalArgument() != "");
    } else {
        m_okButton->setEnabled(getArgument() != "");
    }
}

void
MultiChoiceDialog::choiceChanged()
{
    DEBUG << "choiceChanged" << endl;

    if (m_choiceButtons.empty()) return;

    QString id = "";

    QObject *s = sender();
    QWidget *w = qobject_cast<QWidget *>(s);
    if (w) id = m_choiceButtons[w];

    if (id == m_currentChoice) return;
    if (id == "") {
        // Happens when this is called for the very first time, when
        // m_currentChoice has been set to the intended ID but no
        // button has actually been pressed -- then we need to
        // initialise
        id = m_currentChoice;
    }

    m_currentChoice = id;

    foreach (QWidget *cw, m_choiceButtons.keys()) {
        SelectableLabel *sl = qobject_cast<SelectableLabel *>(cw);
        if (sl) {
            sl->setSelected(m_choiceButtons[cw] == id);
        }
    }

    m_descriptionLabel->setText(m_descriptions[id]);

    m_fileLabel->hide();
    m_fileCombo->hide();
    m_browseButton->hide();
    m_urlLabel->hide();
    m_urlCombo->hide();

    QSharedPointer<RecentFiles> rf = m_recentFiles[id];
    m_fileCombo->clear();
    m_urlCombo->clear();

    switch (m_argTypes[id]) {
        
    case NoArg:
        break;

    case FileArg:
        m_fileLabel->setText(tr("&File:"));
        m_fileLabel->show();
        m_fileCombo->show();
        m_fileCombo->addItems(rf->getRecent());
        m_browseButton->show();
        break;

    case DirectoryArg:
        m_fileLabel->setText(tr("&Folder:"));
        m_fileLabel->show();
        m_fileCombo->show();
        m_fileCombo->addItems(rf->getRecent());
        m_browseButton->show();
        break;

    case UrlArg:
        m_urlLabel->show();
        m_urlCombo->show();
        m_urlCombo->addItems(rf->getRecent());
        break;

    case UrlToDirectoryArg:
        m_urlLabel->show();
        m_urlCombo->show();
        m_urlCombo->addItems(rf->getRecent());
        m_fileLabel->setText(tr("&Folder:"));
        m_fileLabel->show();
        m_fileCombo->show();
        m_fileCombo->lineEdit()->setText(QDir::homePath());
        m_browseButton->show();
        break;
    }

    updateOkButton();
    adjustSize();
}


