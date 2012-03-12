/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2012 Chris Cannam
    Copyright (c) 2012 Queen Mary, University of London

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "moreinformationdialog.h"

#include <QMessageBox>
#include <QLabel>
#include <QGridLayout>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QApplication>
#include <QStyle>

MoreInformationDialog::MoreInformationDialog(QString title,
                                             QString head,
                                             QString text,
                                             QString more,
                                             QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(title);

    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(10);
    setLayout(layout);

    m_iconLabel = new QLabel;
    layout->addWidget(m_iconLabel, 0, 0, 2, 1, Qt::AlignTop);

    QLabel *headLabel = new QLabel(QString("<qt><h3>%1</h3></qt>").arg(head));
    layout->addWidget(headLabel, 0, 1);

    QLabel *textLabel = new QLabel(text);
    textLabel->setTextFormat(Qt::RichText);
    textLabel->setWordWrap(true);
    layout->addWidget(textLabel, 1, 1, Qt::AlignTop);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
    layout->addWidget(bb, 2, 0, 1, 2);

    m_moreButton = bb->addButton(tr("More Details..."),
                                 QDialogButtonBox::ActionRole);
    m_moreButton->setAutoDefault(false);
    m_moreButton->setDefault(false);

    connect(m_moreButton, SIGNAL(clicked()), this, SLOT(moreClicked()));

    bb->button(QDialogButtonBox::Ok)->setDefault(true);

    m_moreText = new QTextEdit();
    m_moreText->setAcceptRichText(false);
    m_moreText->document()->setPlainText(more);
    m_moreText->setMinimumWidth(360);
    m_moreText->setReadOnly(true);
    m_moreText->setLineWrapMode(QTextEdit::NoWrap);

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    m_moreText->setFont(font);

    layout->addWidget(m_moreText, 3, 0, 1, 2);

    m_moreText->hide();
    if (more == "") m_moreButton->hide();

    layout->setRowStretch(1, 10);
    layout->setColumnStretch(1, 20);
    setMinimumWidth(400);
}

MoreInformationDialog::~MoreInformationDialog()
{
}

void
MoreInformationDialog::moreClicked()
{
    if (m_moreText->isVisible()) {
        m_moreText->hide();
        m_moreButton->setText(tr("Show Details..."));
    } else {
        m_moreText->show();
        m_moreButton->setText(tr("Hide Details..."));
    }
    adjustSize();
}        

void
MoreInformationDialog::setIcon(QIcon icon)
{
    QStyle *style = qApp->style();
    int iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);
    m_iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));
}

void
MoreInformationDialog::critical(QWidget *parent, QString title, QString head,
				QString text, QString more)
{
    MoreInformationDialog d(title, head, text, more, parent);
    QStyle *style = qApp->style();
    d.setIcon(style->standardIcon(QStyle::SP_MessageBoxCritical, 0, &d));
    d.exec();
}

void
MoreInformationDialog::information(QWidget *parent, QString title, QString head,
                                   QString text, QString more)
{
    MoreInformationDialog d(title, head, text, more, parent);
    QStyle *style = qApp->style();
    d.setIcon(style->standardIcon(QStyle::SP_MessageBoxInformation, 0, &d));
    d.exec();
}

void
MoreInformationDialog::warning(QWidget *parent, QString title, QString head,
                               QString text, QString more)
{
    MoreInformationDialog d(title, head, text, more, parent);
    QStyle *style = qApp->style();
    d.setIcon(style->standardIcon(QStyle::SP_MessageBoxWarning, 0, &d));
    d.exec();
}

