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

#ifndef CONFIRMCOMMENTDIALOG_H
#define CONFIRMCOMMENTDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QPushButton>

class ConfirmCommentDialog : public QDialog
{
    Q_OBJECT

public:
    static bool confirmFilesAction(QWidget *parent,
                                   QString title,
                                   QString introText,
                                   QString introTextWithCount,
                                   QStringList files);

    static bool confirmDangerousFilesAction(QWidget *parent,
                                            QString title,
                                            QString introText,
                                            QString introTextWithCount,
                                            QStringList files);

    static bool confirmAndGetShortComment(QWidget *parent,
                                          QString title,
                                          QString introText,
                                          QString introTextWithCount,
                                          QStringList files,
                                          QString &comment);

    static bool confirmAndGetLongComment(QWidget *parent,
                                         QString title,
                                         QString introText,
                                         QString introTextWithCount,
                                         QStringList files,
                                         QString &comment);

    static bool confirmAndGetShortComment(QWidget *parent,
                                          QString title,
                                          QString introText,
                                          QString &comment);

    static bool confirmAndGetLongComment(QWidget *parent,
                                         QString title,
                                         QString introText,
                                         QString &comment);

private slots:
    void commentChanged();

private:
    ConfirmCommentDialog(QWidget *parent,
                         QString title,
                         QString introText,
                         QString initialComment);

    static bool confirmAndComment(QWidget *parent,
                                  QString title,
                                  QString introText,
                                  QString introTextWithCount,
                                  QStringList files,
                                  QString &comment,
                                  bool longComment);

    static bool confirmAndComment(QWidget *parent,
                                  QString title,
                                  QString introText,
                                  QString &comment,
                                  bool longComment);

    static QString buildFilesText(QString intro, QStringList files);

    QString getComment() const;

    QTextEdit *m_textEdit;
    QPushButton *m_ok;
};

#endif // CONFIRMCOMMENTDIALOG_H
