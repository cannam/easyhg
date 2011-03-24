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

#ifndef MULTICHOICEDIALOG_H
#define MULTICHOICEDIALOG_H

#include "recentfiles.h"

#include <QDialog>
#include <QString>
#include <QAbstractButton>
#include <QMap>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QSharedPointer>
#include <QComboBox>

class MultiChoiceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MultiChoiceDialog(QString title, QString heading,
                               QWidget *parent = 0);

    enum ArgType {
        NoArg,
        FileArg,
        DirectoryArg,
        UrlArg,
        UrlToDirectoryArg
    };

    void addChoice(QString identifier, QString text,
                   QString description, ArgType arg);

    void setCurrentChoice(QString);
    QString getCurrentChoice();
    QString getArgument();
    QString getAdditionalArgument();

    static void addRecentArgument(QString identifier, QString name,
                                  bool additionalArgument = false);

private slots:
    void choiceChanged();
    void urlChanged(const QString &);
    void fileChanged(const QString &);
    void browse();

private:
    void updateOkButton();
    
    QMap<QString, QString> m_texts;
    QMap<QString, QString> m_descriptions;
    QMap<QString, ArgType> m_argTypes;
    QMap<QString, QSharedPointer<RecentFiles> > m_recentFiles;

    QString m_currentChoice;
    QMap<QWidget *, QString> m_choiceButtons;

    QHBoxLayout *m_choiceLayout;
    QLabel *m_descriptionLabel;
    QLabel *m_fileLabel;
    QComboBox *m_fileCombo;
    QAbstractButton *m_browseButton;
    QLabel *m_urlLabel;
    QComboBox *m_urlCombo;
    QAbstractButton *m_okButton;

    QString getDefaultPath() const;
};

#endif // MULTICHOICEDIALOG_H
