/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2011 Chris Cannam
    Copyright (c) 2011 Queen Mary, University of London

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "confirmcommentdialog.h"
#include "common.h"
#include "debug.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QGridLayout>
#include <QLabel>
#include <QTextEdit>
#include <QDialogButtonBox>

ConfirmCommentDialog::ConfirmCommentDialog(QWidget *parent,
                                           QString title,
                                           QString introText,
                                           QString initialComment,
                                           QString okButtonText) :
    QDialog(parent)
{
    setWindowTitle(title);

    QGridLayout *layout = new QGridLayout;
    setLayout(layout);
    QLabel *label = new QLabel(introText);
    label->setWordWrap(true);
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
    m_ok->setDefault(true);
    m_ok->setEnabled(initialComment != "");
    m_ok->setText(okButtonText);
    bbox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);

    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
}

void ConfirmCommentDialog::commentChanged()
{
    m_ok->setEnabled(getComment() != "");
}

QString ConfirmCommentDialog::getComment() const
{
/*
    DEBUG << "ConfirmCommentDialog: as html is:" << endl;
    DEBUG << m_textEdit->document()->toHtml() << endl;
    DEBUG << "ConfirmCommentDialog: as plain text is:" << endl;
    QString t = m_textEdit->document()->toPlainText();
    DEBUG << t << endl;
    DEBUG << "Characters: " << endl;
    for (int i = 0; i < t.length(); ++i) DEBUG << t[i].unicode();
    DEBUG << endl;
    return t;
*/
}

QString ConfirmCommentDialog::buildFilesText(QString intro, QStringList files)
{
    QString text;

    if (intro == "") text = "<qt>";
    else text = "<qt>" + intro + "<p>";

    text += "<code>";
    foreach (QString file, files) {
        text += "&nbsp;&nbsp;&nbsp;" + xmlEncode(file) + "<br>";
    }
    text += "</code></qt>";

    return text;
}

bool ConfirmCommentDialog::confirm(QWidget *parent,
                                   QString title,
                                   QString head,
                                   QString text,
                                   QString okButtonText)
{
    QMessageBox box(QMessageBox::Question,
                    title,
                    head,
                    QMessageBox::Cancel,
                    parent);

    box.setInformativeText(text);

    QPushButton *ok = box.addButton(QMessageBox::Ok);
    ok->setText(okButtonText);
    box.setDefaultButton(QMessageBox::Ok);
    if (box.exec() == -1) return false;
    return box.standardButton(box.clickedButton()) == QMessageBox::Ok;
}

bool ConfirmCommentDialog::confirmDangerous(QWidget *parent,
                                            QString title,
                                            QString head,
                                            QString text,
                                            QString okButtonText)
{
    QMessageBox box(QMessageBox::Warning,
                    title,
                    head,
                    QMessageBox::Cancel,
                    parent);

    box.setInformativeText(text);

    QPushButton *ok = box.addButton(QMessageBox::Ok);
    ok->setText(okButtonText);
    box.setDefaultButton(QMessageBox::Cancel);
    if (box.exec() == -1) return false;
    return box.standardButton(box.clickedButton()) == QMessageBox::Ok;
}

bool ConfirmCommentDialog::confirmFilesAction(QWidget *parent,
                                              QString title,
                                              QString introText,
                                              QString introTextWithCount,
                                              QStringList files,
                                              QString okButtonText)
{
    QString text;
    if (files.size() <= 10) {
        text = buildFilesText(introText, files);
    } else {
        text = "<qt>" + introTextWithCount + "</qt>";
    }
    return confirm(parent, title, text, "", okButtonText);
}

bool ConfirmCommentDialog::confirmDangerousFilesAction(QWidget *parent,
                                                       QString title,
                                                       QString introText,
                                                       QString introTextWithCount,
                                                       QStringList files,
                                                       QString okButtonText)
{
    QString text;
    if (files.size() <= 10) {
        text = buildFilesText(introText, files);
    } else {
        text = "<qt>" + introTextWithCount + "</qt>";
    }
    return confirmDangerous(parent, title, text, "", okButtonText);
}

bool ConfirmCommentDialog::confirmAndGetShortComment(QWidget *parent,
                                                     QString title,
                                                     QString introText,
                                                     QString introTextWithCount,
                                                     QStringList files,
                                                     QString &comment,
                                                     QString okButtonText)
{
    return confirmAndComment(parent, title, introText,
                             introTextWithCount, files, comment, false,
                             okButtonText);
}

bool ConfirmCommentDialog::confirmAndGetLongComment(QWidget *parent,
                                                    QString title,
                                                    QString introText,
                                                    QString introTextWithCount,
                                                    QStringList files,
                                                    QString &comment,
                                                    QString okButtonText)
{
    return confirmAndComment(parent, title, introText,
                             introTextWithCount, files, comment, true,
                             okButtonText);
}

bool ConfirmCommentDialog::confirmAndComment(QWidget *parent,
                                             QString title,
                                             QString introText,
                                             QString introTextWithCount,
                                             QStringList files,
                                             QString &comment,
                                             bool longComment, 
                                             QString okButtonText)
{
    QString text;
    if (files.size() <= 10) {
        text = buildFilesText(introText, files);
    } else {
        text = "<qt>" + introTextWithCount;
    }
    text += tr("<p>Please enter your comment:</qt>");
    return confirmAndComment(parent, title, text, comment, longComment,
                             okButtonText);
}

bool ConfirmCommentDialog::confirmAndGetShortComment(QWidget *parent,
                                                     QString title,
                                                     QString introText,
                                                     QString &comment,
                                                     QString okButtonText)
{
    return confirmAndComment(parent, title, introText, comment, false,
                             okButtonText);
}

bool ConfirmCommentDialog::confirmAndGetLongComment(QWidget *parent,
                                                    QString title,
                                                    QString introText,
                                                    QString &comment,
                                                    QString okButtonText)
{
    return confirmAndComment(parent, title, introText, comment, true,
                             okButtonText);
}

bool ConfirmCommentDialog::confirmAndComment(QWidget *parent,
                                             QString title,
                                             QString introText,
                                             QString &comment,
                                             bool longComment,
                                             QString okButtonText)
{
    bool ok = false;
    if (!longComment) {
        QInputDialog d(parent);
        d.setWindowTitle(title);
        d.setLabelText(introText);
        d.setTextValue(comment);
        d.setOkButtonText(okButtonText);
        d.setTextEchoMode(QLineEdit::Normal);
        if (d.exec() == QDialog::Accepted) {
            comment = d.textValue();
            ok = true;
        }
    } else {
        ConfirmCommentDialog d(parent, title, introText, comment, okButtonText);
        if (d.exec() == QDialog::Accepted) {
            comment = d.getComment();
            ok = true;
        }
    }

    return ok;
}
