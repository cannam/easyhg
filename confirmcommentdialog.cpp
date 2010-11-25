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

#include <QMessageBox>
#include <QInputDialog>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QDialogButtonBox>

bool ConfirmCommentDialog::confirmFilesAction(QWidget *parent,
                                              QString title,
                                              QString introText,
                                              QString introTextWithCount,
                                              QStringList files)
{
    QString text;
    if (files.size() <= 10) {
        text = "<qt>" + introText;
        text += "<code>";
        foreach (QString file, files) {
            text += file + "<br>";
        }
        text += "</code></qt>";
    } else {
        text = "<qt>" + introText.arg(files.size());
    }
    return (QMessageBox::information(parent,
                                     title,
                                     text,
                                     QMessageBox::Ok | QMessageBox::Cancel,
                                     QMessageBox::Ok)
            == QMessageBox::Ok);
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
        text = "<qt>" + introText;
        text += "<p><ul>";
        foreach (QString file, files) {
            text += "<li>" + file + "</li>";
        }
        text += "</ul><p>Please enter your comment:</qt>";
    } else {
        text = "<qt>" + introText.arg(files.size());
    }
    return confirmAndComment(parent, title, text, comment, longComment);
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
        QDialog *d = new QDialog(parent);
        d->setWindowTitle(title);
        QGridLayout *layout = new QGridLayout;
        d->setLayout(layout);
        QLabel *label = new QLabel(introText);
        layout->addWidget(label, 0, 0);
        QTextEdit *textEdit = new QTextEdit;
        textEdit->setAcceptRichText(false);
        layout->addWidget(textEdit, 1, 0);
        QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                      QDialogButtonBox::Cancel);
        layout->addWidget(bbox, 2, 0);
        QObject::connect(bbox, SIGNAL(accepted()), d, SLOT(accept()));
        QObject::connect(bbox, SIGNAL(rejected()), d, SLOT(reject()));
        if (d->exec() == QDialog::Accepted) {
            comment = textEdit->document()->toPlainText();
            ok = true;
        }
    }
    return ok;
}
