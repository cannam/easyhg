/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    EasyMercurial

    Based on hgExplorer by Jari Korhonen
    Copyright (c) 2010 Jari Korhonen
    Copyright (c) 2013 Chris Cannam
    Copyright (c) 2013 Queen Mary, University of London

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef MORE_INFORMATION_DIALOG_H
#define MORE_INFORMATION_DIALOG_H

#include <QString>
#include <QDialog>

class QLabel;
class QTextEdit;
class QPushButton;
class QGridLayout;

/**
 * Provide methods like the QMessageBox static methods, to call up
 * dialogs with "More information" buttons in them.  QMessageBox does
 * have an optional additional-details field, but it doesn't behave
 * quite as we'd like with regard to layout
 */

class MoreInformationDialog : public QDialog
{
    Q_OBJECT

public:
    MoreInformationDialog(QString title,
                          QString head,
                          QString text,
                          QString more,
                          QWidget *parent = 0);

    ~MoreInformationDialog();

    void setIcon(QIcon);

    static void critical(QWidget *parent, QString title, QString head, QString text, QString more);
    static void information(QWidget *parent, QString title, QString head, QString text, QString more);
    static void warning(QWidget *parent, QString title, QString head, QString text, QString more);

private slots:
    void moreClicked();

private:
    QGridLayout *m_layout;
    QLabel *m_iconLabel;
    QPushButton *m_moreButton;
    QTextEdit *m_moreText;
};

#endif
