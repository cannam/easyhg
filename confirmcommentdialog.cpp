/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2010 Chris Cannam
    Copyright (c) 2010 Queen Mary, University of London

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "confirmcommentdialog.h"
#include "common.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QDialogButtonBox>

ConfirmCommentDialog::ConfirmCommentDialog(QWidget *parent,
                                           QString title,
                                           QString introText,
                                           QString initialComment) :
    QDialog(parent)
{
    setWindowTitle(title);

    QGridLayout *layout = new QGridLayout;
    setLayout(layout);
    QLabel *label = new QLabel(introText);
    layout->addWidget(label, 0, 0);

    m_textEdit = new QTextEdit;
    m_textEdit->setAcceptRichText(false);
    m_textEdit->document()->setPlainText(initialComment);
    m_textEdit->setMinimumWidth(360);
    connect(m_textEdit, SIGNAL(textChanged()), this, SLOT(commentChanged()));
    layout->addWidget(m_textEdit, 1, 0);

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                  QDialogButtonBox::Cancel);
    layout->addWidget(bbox, 2, 0);
    m_ok = bbox->button(QDialogButtonBox::Ok);
    m_ok->setEnabled(initialComment != "");

    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
}

void ConfirmCommentDialog::commentChanged()
{
    m_ok->setEnabled(getComment() != "");
}

QString ConfirmCommentDialog::getComment() const
{
    return m_textEdit->document()->toPlainText();
}

QString ConfirmCommentDialog::buildFilesText(QString intro, QStringList files)
{
    QString text;
    text = "<qt>" + intro;
    text += "<p><code>";
    foreach (QString file, files) {
        text += "&nbsp;&nbsp;&nbsp;" + xmlEncode(file) + "<br>";
    }
    text += "</code></qt>";
    return text;
}

bool ConfirmCommentDialog::confirmFilesAction(QWidget *parent,
                                              QString title,
                                              QString introText,
                                              QString introTextWithCount,
                                              QStringList files)
{
    QString text;
    if (files.size() <= 10) {
        text = buildFilesText(introText, files);
    } else {
        text = "<qt>" + introTextWithCount.arg(files.size()) + "</qt>";
    }
    return (QMessageBox::information(parent,
                                     title,
                                     text,
                                     QMessageBox::Ok | QMessageBox::Cancel,
                                     QMessageBox::Ok)
            == QMessageBox::Ok);
}

bool ConfirmCommentDialog::confirmDangerousFilesAction(QWidget *parent,
                                                       QString title,
                                                       QString introText,
                                                       QString introTextWithCount,
                                                       QStringList files)
{
    QString text;
    if (files.size() <= 10) {
        text = buildFilesText(introText, files);
    } else {
        text = "<qt>" + introTextWithCount.arg(files.size()) + "</qt>";
    }
    return (QMessageBox::warning(parent,
                                 title,
                                 text,
                                 QMessageBox::Ok | QMessageBox::Cancel,
                                 QMessageBox::Cancel)
            == QMessageBox::Ok);
}

bool ConfirmCommentDialog::confirmAndGetShortComment(QWidget *parent,
                                                     QString title,
                                                     QString introText,
                                                     QString introTextWithCount,
                                                     QStringList files,
                                                     QString &comment)
{
    return confirmAndComment(parent, title, introText,
                             introTextWithCount, files, comment, false);
}

bool ConfirmCommentDialog::confirmAndGetLongComment(QWidget *parent,
                                                    QString title,
                                                    QString introText,
                                                    QString introTextWithCount,
                                                    QStringList files,
                                                    QString &comment)
{
    return confirmAndComment(parent, title, introText,
                             introTextWithCount, files, comment, true);
}

bool ConfirmCommentDialog::confirmAndComment(QWidget *parent,
                                             QString title,
                                             QString introText,
                                             QString introTextWithCount,
                                             QStringList files,
                                             QString &comment,
                                             bool longComment)
{
    QString text;
    if (files.size() <= 10) {
        text = buildFilesText(introText, files);
    } else {
        text = "<qt>" + introTextWithCount.arg(files.size());
    }
    text += tr("<p>Please enter your comment:</qt>");
    return confirmAndComment(parent, title, text, comment, longComment);
}

bool ConfirmCommentDialog::confirmAndGetShortComment(QWidget *parent,
                                                     QString title,
                                                     QString introText,
                                                     QString &comment)
{
    return confirmAndComment(parent, title, introText, comment, false);
}

bool ConfirmCommentDialog::confirmAndGetLongComment(QWidget *parent,
                                                    QString title,
                                                    QString introText,
                                                    QString &comment)
{
    return confirmAndComment(parent, title, introText, comment, true);
}

bool ConfirmCommentDialog::confirmAndComment(QWidget *parent,
                                             QString title,
                                             QString introText,
                                             QString &comment,
                                             bool longComment)
{
    bool ok = false;
    if (!longComment) {
        comment = QInputDialog::getText(parent, title, introText,
                                        QLineEdit::Normal, comment, &ok);
    } else {
        ConfirmCommentDialog *d = new ConfirmCommentDialog(parent,
                                                           title,
                                                           introText,
                                                           comment);
        if (d->exec() == QDialog::Accepted) {
            comment = d->getComment();
            ok = true;
        }
    }
    return ok;
}
