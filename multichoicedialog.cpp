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

#include "multichoicedialog.h"

#include "selectablelabel.h"

#include "debug.h"

#include <QDialogButtonBox>
#include <QToolButton>
#include <QPushButton>
#include <QFont>
#include <QDir>
#include <QFileDialog>

MultiChoiceDialog::MultiChoiceDialog(QString title, QString heading, QWidget *parent) :
    QDialog(parent)
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
    f.setPointSize(f.pointSize() * 0.9);
    m_descriptionLabel->setFont(f);

    m_argLabel = new QLabel();
    outer->addWidget(m_argLabel, 3, 0);

    m_argEdit = new QComboBox();
    m_argEdit->setEditable(true);
    outer->addWidget(m_argEdit, 3, 1);
    outer->setColumnStretch(1, 20);

    m_browseButton = new QPushButton(tr("Browse..."));
    outer->addWidget(m_browseButton, 3, 2);
    connect(m_browseButton, SIGNAL(clicked()), this, SLOT(browse()));

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                  QDialogButtonBox::Cancel);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
    outer->addWidget(bbox, 4, 0, 1, 3);
    
    setMinimumWidth(480);
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
    return m_argEdit->currentText();
}

void
MultiChoiceDialog::addRecentArgument(QString id, QString arg)
{
    RecentFiles(QString("Recent-%1").arg(id)).addFile(arg);
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

    if (m_argTypes[m_currentChoice] == DirectoryArg) {

        path = QFileDialog::getExistingDirectory
            (this, tr("Open Directory"), origin,
             QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (path != QString()) {
            m_argEdit->lineEdit()->setText(path + QDir::separator());
        }

    } else {

        path = QFileDialog::getOpenFileName
            (this, tr("Open File"), origin);
        if (path != QString()) {
            m_argEdit->lineEdit()->setText(path);
        }
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

    switch (m_argTypes[id]) {
        
    case NoArg:
        m_argLabel->hide();
        m_argEdit->hide();
        m_browseButton->hide();
        break;

    case FileArg:
        m_argLabel->setText(tr("File:"));
        m_argLabel->show();
        m_argEdit->show();
        m_browseButton->show();
        break;

    case DirectoryArg:
        m_argLabel->setText(tr("Folder:"));
        m_argLabel->show();
        m_argEdit->show();
        m_browseButton->show();
        break;

    case UrlArg:
        m_argLabel->setText(tr("URL:"));
        m_argLabel->show();
        m_argEdit->show();
        m_browseButton->hide();
        break;

    case FileOrUrlArg:
        m_argLabel->setText(tr("File or URL:"));
        m_argLabel->show();
        m_argEdit->show();
        m_browseButton->show();
        break;
    }

    if (m_argTypes[id] != NoArg) {
        QSharedPointer<RecentFiles> rf = m_recentFiles[id];
        m_argEdit->clear();
        m_argEdit->addItems(rf->getRecent());
    }

    adjustSize();
}


