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

#include "findwidget.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

FindWidget::FindWidget(QWidget *parent) :
    QWidget(parent)
{
    QGridLayout *layout = new QGridLayout;
    layout->setMargin(0);
    setLayout(layout);

    QToolButton *button = new QToolButton();
    layout->addWidget(button, 0, 0);
    button->setText(tr("Find..."));
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
//    button->setAutoRaise(true);
    connect(button, SIGNAL(clicked()), this, SLOT(buttonPressed()));

    m_lineEdit = new QLineEdit();
    layout->addWidget(m_lineEdit, 0, 1);

    m_lineEdit->setFixedWidth(100);
    m_lineEdit->hide();

    int h = m_lineEdit->sizeHint().height();
    int h0 = button->sizeHint().height();
    if (h > h0) button->setFixedHeight(h);

    connect(m_lineEdit, SIGNAL(textChanged(const QString &)),
	    this, SIGNAL(findTextChanged(QString)));
}

FindWidget::~FindWidget()
{
}

void
FindWidget::buttonPressed()
{
    QAbstractButton *button = qobject_cast<QAbstractButton *>(sender());
    if (!button) return;
    if (m_lineEdit->isVisible()) {
	m_lineEdit->hide();
	button->setText(tr("Find..."));
        if (m_lineEdit->text() != "") {
            emit findTextChanged("");
        }
    } else {
	m_lineEdit->show();
        m_lineEdit->setFocus(Qt::OtherFocusReason);
	button->setText(tr("Find:"));
        if (m_lineEdit->text() != "") {
            emit findTextChanged(m_lineEdit->text());
        }
    }
}
	
