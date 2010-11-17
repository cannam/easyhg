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

#ifndef MULTICHOICEDIALOG_H
#define MULTICHOICEDIALOG_H

#include <QDialog>
#include <QString>
#include <QPushButton>
#include <QMap>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QStackedWidget>

class MultiChoiceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MultiChoiceDialog(QString title, QString heading, QWidget *parent = 0);

    enum ArgType {
        NoArg,
        FileArg,
        UrlArg,
        FileOrUrlArg
    };

    void addChoice(QString identifier, QString text,
                   QString description, ArgType arg);

    QString getSelectedIdentifier();
    QString getArgument();

signals:

private slots:


private:
    void updateArgWidgets(); // when choice changes

    QMap<QString, QString> m_texts;
    QMap<QString, QString> m_descriptions;

    QString m_currentChoice;
    QMap<QString, QPushButton *> m_choiceButtons;

    QGridLayout *m_choiceLayout;
    QLabel *m_descriptionLabel;
    QLabel *m_argLabel;
    QLineEdit *m_argEdit;
    QPushButton *m_browseButton;

};

#endif // MULTICHOICEDIALOG_H
