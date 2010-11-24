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
                                             QString &comment)
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
    return confirmAndComment(parent, title, text, comment);
}

bool ConfirmCommentDialog::confirmAndComment(QWidget *parent,
                                             QString title,
                                             QString introText,
                                             QString &comment)
{
    bool ok = false;
    //!!! ok, for comments need more than one line
    comment = QInputDialog::getText(parent, title, introText,
                                    QLineEdit::Normal, comment, &ok);
    return ok;
}
